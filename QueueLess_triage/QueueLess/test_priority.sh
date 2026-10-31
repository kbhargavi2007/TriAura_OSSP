#!/bin/bash
# test_priority.sh - deterministic test of triage-assigned priority scheduling.
#
# Nothing is faked: it starts the REAL ./qserver (1 doctor process) and drives it with REAL ./qtriage / ./qclient
# processes.  The order of doctor calls is then read from qserver's own log and compared with what the scheduler
# (Emergency > Urgent > Normal, shortest-predicted-first, aging, emergency preemption) must produce.
# One simulated minute = 400 ms, so a consultation lasts ~3.6 s: plenty of time for the queue to fill.
#
# Runs in a temporary directory (your qdata/ is not touched).  Needs a free /tmp/queueless.reg, i.e. no other qserver.
cd "$(dirname "$0")" || exit 1
ROOT=$PWD
[ -x ./qserver ] && [ -x ./qtriage ] && [ -x ./qclient ] || make >/dev/null || exit 1
if pgrep -x qserver >/dev/null; then echo "A qserver is already running - stop it first (quit in its prompt)."; exit 1; fi

WORK=$(mktemp -d); cd "$WORK" || exit 1
SP=""; FAILS=0
cleanup() { [ -n "$SP" ] && kill -TERM "$SP" 2>/dev/null; pkill -x qclient 2>/dev/null; pkill -x qtriage 2>/dev/null; rm -rf "$WORK"; }
trap cleanup EXIT

start_server() {
    "$ROOT/qserver" -n 1 -s 400 < /dev/null > "$WORK/server.out" 2>&1 &
    SP=$!; sleep 1.2
}
stop_server() { kill -TERM "$SP" 2>/dev/null; wait "$SP" 2>/dev/null; SP=""; }
show_log() { sed -E 's/^queueless> //' "$WORK/server.out" | grep -E "REGISTER|TRIAGE|ADMIT|CALL|PREEMPT|BACK|DONE" | sed 's/^/    /'; }
call_order() { sed -E -n 's/.*CALL +token #[0-9]+ +([^ ]+).*/\1/p' "$WORK/server.out" | tr '\n' ' ' | sed 's/ $//'; }
lineno() { grep -n -m1 -E "$1" "$WORK/server.out" | cut -d: -f1; }
wait_pids() {                                    # wait (max ~60 s) for background client processes
    for _ in $(seq 1 120); do
        alive=0; for p in "$@"; do kill -0 "$p" 2>/dev/null && alive=1; done
        [ $alive -eq 0 ] && return 0; sleep 0.5
    done
    echo "    (timeout waiting for clients)"; return 1
}
check() {                                        # check "description" "expected" "actual"
    if [ "$2" == "$3" ]; then echo "  PASS: $1  ->  $3"; else echo "  FAIL: $1"; echo "        expected: $2"; echo "        actual  : $3"; FAILS=$((FAILS+1)); fi
}

echo "=================================================================================="
echo " SCENARIO 1: emergency PREEMPTS a normal patient; urgent is served before normal resumes"
echo "   1 doctor. Ravi (NORMAL) is with the doctor -> Arjun (URGENT) waits -> Zoya (EMERGENCY) arrives"
echo "=================================================================================="
start_server
"$ROOT/qtriage" -n Ravi  -a 40 -d 0 -v new -c 1 -r 0 -p N -w > "$WORK/c1.out" 2>&1 & P1=$!; sleep 1.0
"$ROOT/qtriage" -n Arjun -a 45 -d 0 -v new -c 1 -r 0 -p U -w > "$WORK/c2.out" 2>&1 & P2=$!; sleep 0.5
"$ROOT/qtriage" -n Zoya  -a 30 -d 0 -v new -c 1 -r 0 -p E -w > "$WORK/c3.out" 2>&1 & P3=$!
wait_pids $P1 $P2 $P3; sleep 0.5
show_log
check "doctor call order (Ravi again = resumed after preemption)" "Ravi Zoya Arjun Ravi" "$(call_order)"
check "a PREEMPT was logged" "1" "$(grep -c 'PREEMPT' "$WORK/server.out")"
echo "  --- what Zoya's triage terminal printed:"; sed 's/^/    /' "$WORK/c3.out"
stop_server

echo
echo "=================================================================================="
echo " SCENARIO 2: several patients WAIT; when the doctor frees up the scheduler picks by priority"
echo "   1 doctor. Occ (EMERGENCY) is with the doctor (not preemptable) -> Nora (NORMAL), Uma (URGENT),"
echo "   Eli (EMERGENCY) all enter the waiting queue, in that order"
echo "=================================================================================="
start_server
"$ROOT/qtriage" -n Occ  -a 50 -d 0 -v new -c 1 -r 0 -p E -w > "$WORK/d1.out" 2>&1 & P1=$!; sleep 0.8
"$ROOT/qtriage" -n Nora -a 30 -d 0 -v new -c 1 -r 0 -p N -w > "$WORK/d2.out" 2>&1 & P2=$!; sleep 0.3
"$ROOT/qtriage" -n Uma  -a 61 -d 0 -v new -c 1 -r 0 -p U -w > "$WORK/d3.out" 2>&1 & P3=$!; sleep 0.3
"$ROOT/qtriage" -n Eli  -a 22 -d 0 -v new -c 1 -r 0 -p E -w > "$WORK/d4.out" 2>&1 & P4=$!
wait_pids $P1 $P2 $P3 $P4; sleep 0.5
show_log
check "doctor call order" "Occ Eli Uma Nora" "$(call_order)"
check "no preemption (an emergency never preempts another emergency)" "0" "$(grep -c 'PREEMPT' "$WORK/server.out")"
stop_server

echo
echo "=================================================================================="
echo " SCENARIO 3: patients cannot choose their priority; only the triage desk can"
echo "=================================================================================="
start_server
"$ROOT/qclient" -n X -p E > /dev/null 2>&1; check "qclient rejects the removed -p option (exit code)" "2" "$?"
python3 - <<'EOF'
# a forged kiosk-style request that claims priority 0 (= EMERGENCY) but has source=SRC_KIOSK (0)
import os, struct
req = struct.pack('<ii32s8B64s', os.getpid(), 0, b'Forged', 30, 0, 0, 1, 0, 0, 0, 0, b'')
fd = os.open('/tmp/queueless.reg', os.O_WRONLY | os.O_NONBLOCK); os.write(fd, req); os.close(fd)
EOF
sleep 1.5
check "forged 'emergency' kiosk request is NOT scheduled although the doctor is idle (CALL count)" "0" "$(grep -c 'CALL' "$WORK/server.out")"
"$ROOT/qtriage" -t 1 -p N > "$WORK/e1.out" 2>&1; sleep 0.8       # triage desk assigns NORMAL to token #1
"$ROOT/qclient" -n Kiosk -a 35 -d 0 -v new -c 1 -r 0 -w > "$WORK/e2.out" 2>&1 & P2=$!; sleep 0.8
"$ROOT/qtriage" -t 2 -p E > "$WORK/e3.out" 2>&1                  # triage desk assigns EMERGENCY to token #2
wait_pids $P2; sleep 3.5
show_log
check "token #1: REGISTER precedes TRIAGE precedes CALL" "1" "$( a=$(lineno 'REGISTER token #1 '); b=$(lineno 'TRIAGE token #1 '); c=$(lineno 'CALL +token #1 '); [ "$a" -lt "$b" ] && [ "$b" -lt "$c" ] && echo 1 || echo 0)"
check "token #2 (kiosk patient): REGISTER precedes TRIAGE precedes CALL" "1" "$( a=$(lineno 'REGISTER token #2 '); b=$(lineno 'TRIAGE token #2 '); c=$(lineno 'CALL +token #2 '); [ "$a" -lt "$b" ] && [ "$b" -lt "$c" ] && echo 1 || echo 0)"
check "kiosk patient (triaged EMERGENCY) preempted the NORMAL one" "1" "$(grep -c 'PREEMPT' "$WORK/server.out")"
echo "  --- what the kiosk patient's terminal printed:"; sed 's/^/    /' "$WORK/e2.out"
stop_server

echo
if [ $FAILS -eq 0 ]; then echo "ALL CHECKS PASSED"; else echo "$FAILS CHECK(S) FAILED"; fi
exit $FAILS
