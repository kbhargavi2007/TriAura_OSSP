# QueueLess – Smart Hospital/Clinic Waiting System (Operating Systems project)

**Big idea:** a clinic *is* an operating system.

| Clinic | Operating system |
|---|---|
| Patient | Process |
| Doctor | CPU core (one `qdoctor` process each) |
| Waiting room | Ready queue (linked list on the heap) |
| Consultation | CPU burst |
| Reception | Scheduler / kernel service (`qserver`) |
| Emergency arrives | High-priority process → **preemption** |
| Waiting-room chairs | Counting semaphore |
| Lab/printer/ECG machine | Shared resources → deadlock risk |

## Architecture: who decides what

```
Patient  ->  Reception  ->  Triage desk  ->  Priority queue  ->  OS-style scheduler  ->  Doctor process
(qclient)    (qserver       (qtriage:        (linked list,        (sched_thread:          (qdoctor:
             registers)      assigns          triage_priority)     priority classes,       fork + exec,
                             priority)                             SJF, aging, preempt)    one per doctor)
```

**Priority is assigned by the triage/reception component. The scheduler does not diagnose patients. It uses the
assigned priority to schedule waiting patients.**

* `qclient` (patient kiosk) collects only registration details: name, age, department, new/follow-up, number of
  complaints, old reports. It has **no priority option**. The server queues such a patient as *awaiting triage*; he is
  invisible to the scheduler until triage assigns a priority. Any priority a kiosk request might carry is ignored.
* `qtriage` is an **authorized triage / receptionist simulation**: a person decides E/U/N and types it with `-p`.
  It sends the decision to `qserver` through the existing named pipe. (The computer does not diagnose anyone.)
  `./qtriage -t TOKEN -p E|U|N` assigns (or changes) the priority of a patient who already registered at the kiosk.
* `qserver` never chooses a priority. The field is called `triage_priority` end to end
  (`RegRequest` -> `Patient` -> `Assign` -> `Record`), and the scheduler only sorts by it.
* The `add ...` / `emergency ...` / `demo` commands of the reception shell act as the desk too (source = triage).
* The server log shows who did what: `REGISTER` (kiosk, awaiting) -> `TRIAGE token #N name -> PRIORITY` -> `ADMIT` -> `CALL`.

## Build and run (Ubuntu)
```bash
sudo apt update && sudo apt install build-essential      # gcc, make, libc headers
make clean && make                    # builds qserver qdoctor qclient qtriage qstatus
./qserver -n 3 -s 250                 # 3 doctors, 1 simulated minute = 250 ms
```
Other terminals:
```bash
./qtriage -n Ravi  -a 66 -d 1 -v new -c 5 -r 1 -p N -w   # triage desk: register + assign NORMAL, -w waits for the call
./qtriage -n Zoya  -a 30 -d 1 -v new -c 1 -r 0 -p E      # triage desk: EMERGENCY
./qtriage -n Arjun -a 45 -d 1 -v new -c 3 -r 0 -p U      # triage desk: URGENT
./qclient -n Meena -a 40 -d 0 -v new -c 1 -r 0 -w        # patient kiosk: no priority; awaits triage (prints its token)
./qtriage -t 4 -p U                                      # triage desk assigns a priority to kiosk token #4
./qstatus -w                                             # live display (memory-mapped board)
```
Deterministic priority test (real `qserver` + real `qtriage`/`qclient`, ~50 s, prints PASS/FAIL): `make test`
(or `./test_priority.sh`). It has three scenarios: emergency preempts normal; a queue of Normal/Urgent/Emergency is
served Emergency -> Urgent -> Normal; and the kiosk cannot self-declare a priority.
In the reception prompt type `help`. Quick tour: `demo 20`, `status`, `estimate`, `emergency Zoya 30 0`,
`break`, `resume`, `kill 1`, `ps`, `sys`, `mem`, `demand`, `cow`, `vtop`, `memerr`, `fs`, `fds`, `iobench`, `race`, `deadlock`, `zombie`, `sh ls | wc -l`.

## Can the receptionist know the consultation time? (the estimation problem)
No – and a real OS has exactly the same problem: **SJF needs the CPU-burst length, which the scheduler never knows.**
The OS answer is *prediction from history*, and that is what QueueLess does:

1. **Cheap non-medical features** the receptionist can collect: department, new vs follow-up, age band, number of separate
   complaints, whether the patient brings old reports.
2. **Exponential averaging** per (department, visit-type): `tau(n+1) = a*t(n) + (1-a)*tau(n)` – the classic OS burst predictor.
3. **Online linear correction** (normalised LMS) learns how much senior age, children, extra complaints and reports add.
4. **Per-doctor speed factor** learned automatically (Dr.2 is x1.2 slower, Dr.1 is x0.85 faster, ...).
5. **Uncertainty**: variance is tracked, so the patient sees a **range** (80 % interval), not a fake exact number.
6. **Live correction**: if a consultation overruns, the remaining time is re-estimated (like a feedback scheduler), so every waiting
   patient's ETA is recomputed every second by simulating the whole queue over all doctors.
7. **Learning survives restarts**: every finished consultation is appended to `qdata/records.dat` and replayed at startup.
8. In a real clinic the doctor presses "done" (or a nurse triage adds a level); here consultation time is simulated by a hidden
   "reality" model so you can *watch the estimator converge* (`estimate` prints learned MAE vs the receptionist's flat guess).

Scheduling policy (priority = `triage_priority`, set by the triage desk): priority classes (emergency > urgent > normal); inside a class, shortest *predicted remaining* time first with
**aging** (waiting time reduces the sort key → no starvation). Emergencies **preempt** the doctor with the most remaining work via
`sigqueue(SIGUSR1, patient_id)`; progress is saved and the interrupted patient re-enters the queue.

## Syllabus coverage – where each topic lives
| CO | Topic | Where / how to see it |
|---|---|---|
| **1** | Layered service abstraction, user/kernel space, syscalls | `sys` (vDSO vs real syscall cost, `syscall(SYS_getpid)`), logger uses raw `write()` |
| 1 | Shell as user program, command journey | `sh cmd`, `sh a \| b`, `sh cmd > file`, `cd` built-in vs external; `!cmd` shortcut |
| 1 | Linux stack / systems programming | `readlink /proc/self/exe`, `/proc` parsing, `getopt`, `strace -f -p $(pgrep qserver)` |
| **2** | Process abstraction/lifecycle | doctors are separate processes: `fork` → `execl` → run → `waitpid`; `ps` reads `/proc/PID/stat` states R/S/T/Z |
| 2 | Synchronisation & termination, pitfalls | signal thread `waitpid()`s only its own doctors (no zombies), respawn on crash (`kill 0`), `zombie` demo, fork-in-threaded-program avoided by exec |
| 2 | User-level scheduling | priority + SJF-by-prediction + aging + preemption (`sched_thread`) |
| **3** | Anonymous pipes | reception ↔ each doctor (2 pipes/doctor, `poll()` in the collector thread), shell pipelines |
| 3 | Named pipes | `/tmp/queueless.reg` (registrations from `qclient` and triage decisions from `qtriage`) and per-patient reply FIFOs |
| 3 | Signals & handlers | doctor: `sigaction` for SIGUSR1/SIGTERM/SIGCONT; client: SIGUSR1 (your turn + room number), SIGUSR2 (done) with `sigsuspend`; server: `sigwait` thread, SIGCHLD |
| 3 | Groups, sessions, job control | doctors share one process group; `break`/`resume` = `killpg(SIGSTOP/SIGCONT)`; `ps` shows PGID/SID; try Ctrl-Z / `fg` on `qserver` |
| **4** | Virtual memory, address space layout | `mem` (text/data/bss/heap/mmap/stack, `/proc/self/maps`) |
| 4 | Address translation, page tables | `vtop` (VPN, offset, PTE bits, PFN via `/proc/self/pagemap`; PFN visible with `sudo`) |
| 4 | Page faults, demand paging | `demand` (64 MiB mmap: 0 → 16384 minor faults) |
| 4 | Dynamic allocation | malloc'd patient list; `mem` shows brk-heap vs mmap threshold |
| 4 | Copy-on-write | `cow` (child read pass 0 faults, write pass one fault per page) |
| 4 | Memory errors, OS response, tools | `memerr` (SIGSEGV, guard page, RLIMIT_AS); `make asan`, valgrind |
| **5** | Unix file abstraction, inodes, dir entries, VFS, ext4 | `fs` (stat, statfs, readdir with inode numbers, hard vs symbolic link, /proc via same `read()`) |
| 5 | File descriptors, open file management | `fds` (`/proc/self/fd`, `dup` shares offset, second `open` doesn't) |
| 5 | File I/O syscalls, buffered vs unbuffered | `iobench`; logger thread uses `write()` for the terminal and `stdio` for the log file |
| 5 | Memory-mapped file I/O | `/tmp/queueless.board` is `mmap`'d by server, doctors and `qstatus`; `fs` walks `records.dat` via `mmap` + `mincore` |
| **6** | Threads vs processes, pthreads | 6 threads in the server, 4 doctor processes |
| 6 | Race conditions, mutexes | `race` (lost updates vs mutex vs atomic); `qlock` protects the queue |
| 6 | Condition variables | scheduler sleeps on `qcond`, woken by admissions/completions |
| 6 | Counting semaphores | waiting-room chairs; logger bounded buffer (`empty`/`full`); 2 printers (process-shared, in the mmap) |
| 6 | Deadlocks | `deadlock` demo; global lock order `qlock → est_lock → board lock`, `records_lock → printer`; timeouts everywhere |
| 6 | Advanced synchronisation | `pthread_rwlock` (estimator), process-shared **robust** mutexes (survive a doctor being killed), barrier in `race` |

## Files
`queueless.h` shared structs · `qserver.c` reception, scheduler, estimator, admin shell · `qdoctor.c` doctor process ·
`qclient.c` patient kiosk (no priority) · `qtriage.c` authorized triage/receptionist simulation · `qstatus.c` display screen ·
`oslab.c` live OS demos · `test_priority.sh` deterministic scheduling test · `Makefile`

## Tools for the viva
`make asan` (AddressSanitizer+UBSan) · `make tsan` (ThreadSanitizer) · `valgrind --leak-check=full ./qserver -n 1` ·
`strace -f -e trace=process,signal -p $(pgrep qserver)` · `ps -eo pid,ppid,pgid,sid,stat,comm | grep q` · `ls -l /proc/$(pgrep qserver)/fd`

## Honest limitations
* Consultation times are simulated (the doctor "presses done" in real life). The estimator never sees the hidden truth.
* A doctor killed mid-consultation restarts that patient from zero. * Runs on one machine (IPC, not networking).
* `qtriage` and `qclient` share one FIFO (mode 0666); the server ignores a priority in a kiosk request, but any local user could still
  write a "triage" request. Real role separation would use a second FIFO with mode 0660 and a dedicated group.
* A kiosk patient stays *awaiting triage* until someone runs `qtriage -t TOKEN -p ...`; he is not shown in the qstatus queue (only counted).
