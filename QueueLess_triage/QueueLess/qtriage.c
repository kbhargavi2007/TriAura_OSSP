/* qtriage.c - AUTHORIZED TRIAGE / RECEPTIONIST SIMULATION (a separate process)
 *
 * This program stands for a trained member of staff at the triage desk.  A PERSON decides the priority and types
 * it with -p.  The computer does NOT diagnose anybody and qserver never chooses a priority: it only schedules
 * according to the value the triage desk sent.
 *
 *   qtriage -> named pipe (FIFO) -> qserver (scheduler) -> doctor process        (NOT  patient -> priority -> qserver)
 *
 * Two uses:
 *   1) register a patient AT the desk and assign the priority in one step
 *        ./qtriage -n Ravi -a 66 -d 1 -v new -c 5 -r 1 -p N [-w]
 *   2) assign / change the priority of a patient who already registered at the kiosk (./qclient) - by token
 *        ./qtriage -t 7 -p E
 *
 * CO-3: same FIFO mechanism as qclient: request on /tmp/queueless.reg, private reply FIFO, and with -w the process
 *       waits for SIGUSR1 (your turn + room, sigqueue payload) / SIGUSR2 (done) using sigsuspend().
 */
#include "queueless.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <getopt.h>
#include <sys/stat.h>

static volatile sig_atomic_t called = 0, finished = 0, room = -1;
static void on_usr1(int s, siginfo_t *si, void *u) { (void)s; (void)u; room = si->si_value.sival_int; called = 1; }
static void on_usr2(int s) { (void)s; finished = 1; }

static void usage(void)
{
    fprintf(stderr,
        "QueueLess triage desk - AUTHORIZED TRIAGE / RECEPTIONIST SIMULATION (a person chooses the priority)\n"
        "usage: qtriage -n NAME -a AGE -d DEPT -v new|fu -c COMPLAINTS -r 0|1 -p E|U|N [-w]   register + assign priority\n"
        "       qtriage -t TOKEN -p E|U|N                                                     assign priority to a registered patient\n"
        "  -p  E emergency, U urgent, N normal  -  the triage decision (the computer does not diagnose)\n"
        "  DEPT: 0 General 1 Cardiology 2 Orthopedics 3 Pediatrics 4 Dermatology 5 ENT\n");
    exit(2);
}

int main(int argc, char **argv)
{
    RegRequest rq; memset(&rq, 0, sizeof rq);
    strcpy(rq.name, "patient"); rq.age = 30; rq.complaints = 1; rq.source = SRC_TRIAGE;
    int wait_mode = 0, have_p = 0, update = 0, opt;
    while ((opt = getopt(argc, argv, "n:a:d:v:c:r:p:t:wh")) != -1) {
        switch (opt) {
        case 'n': strncpy(rq.name, optarg, sizeof rq.name - 1); break;
        case 'a': rq.age = (uint8_t)atoi(optarg); break;
        case 'd': rq.dept = (uint8_t)atoi(optarg); break;
        case 'v': rq.visit = (optarg[0] == 'f' || optarg[0] == 'F'); break;
        case 'c': rq.complaints = (uint8_t)atoi(optarg); break;
        case 'r': rq.has_reports = (uint8_t)atoi(optarg); break;
        case 'p': have_p = 1;
                  switch (optarg[0]) { case 'E': case 'e': rq.triage_priority = PRIO_EMERGENCY; break;
                                       case 'U': case 'u': rq.triage_priority = PRIO_URGENT; break;
                                       case 'N': case 'n': rq.triage_priority = PRIO_NORMAL; break;
                                       default: usage(); }
                  break;
        case 't': rq.token = atoi(optarg); update = 1; break;
        case 'w': wait_mode = 1; break;
        default: usage();
        }
    }
    if (!have_p) { fprintf(stderr, "qtriage: the triage priority (-p E|U|N) is required\n"); usage(); }
    if (update) { rq.source = SRC_TRIAGE_UPDATE; wait_mode = 0; if (rq.token < 1) usage(); }
    rq.client_pid = getpid(); rq.notify = (uint8_t)wait_mode;
    snprintf(rq.reply_fifo, sizeof rq.reply_fifo, "/tmp/queueless.reply.%d", getpid());

    printf("[triage desk - authorized triage/receptionist simulation]\n");
    printf("TRIAGE: assigned %s priority", PRIO_NAME[rq.triage_priority]);
    if (update) printf(" to token #%d", rq.token);
    printf("\n"); fflush(stdout);

    /* handlers first, signals blocked until we deliberately sigsuspend() */
    struct sigaction sa; memset(&sa, 0, sizeof sa); sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = on_usr1; sa.sa_flags = SA_SIGINFO; sigaction(SIGUSR1, &sa, NULL);
    memset(&sa, 0, sizeof sa); sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_usr2; sigaction(SIGUSR2, &sa, NULL);
    sigset_t block, old; sigemptyset(&block); sigaddset(&block, SIGUSR1); sigaddset(&block, SIGUSR2);
    sigprocmask(SIG_BLOCK, &block, &old);

    unlink(rq.reply_fifo);
    if (mkfifo(rq.reply_fifo, 0600) < 0) { perror("mkfifo reply"); return 1; }
    int rfd = open(rq.reply_fifo, O_RDONLY | O_NONBLOCK);     /* reader first => server can open writer */
    int sfd = open(REG_FIFO, O_WRONLY | O_NONBLOCK);
    if (sfd < 0) { fprintf(stderr, "Reception is closed (is ./qserver running?): %s\n", strerror(errno));
                   unlink(rq.reply_fifo); return 1; }
    if (write(sfd, &rq, sizeof rq) != (ssize_t)sizeof rq) { perror("write"); unlink(rq.reply_fifo); return 1; }
    close(sfd);

    struct pollfd pf = { rfd, POLLIN, 0 };
    RegReply rep; int ok = 0;
    if (poll(&pf, 1, 4000) > 0 && read(rfd, &rep, sizeof rep) == (ssize_t)sizeof rep) ok = 1;
    close(rfd); unlink(rq.reply_fifo);
    if (!ok) { fprintf(stderr, "No reply from reception (timeout)\n"); return 1; }
    if (rep.status != 0) { printf("Triage refused: %s\n", rep.msg); return 3; }

    if (update) { printf("  token #%d: %s\n  patients ahead: %d, expected wait %.1f min\n", rep.token, rep.msg, rep.ahead, rep.eta); return 0; }
    printf("Token #%d for %s  (%s, %s)\n", rep.token, rq.name, DEPT_NAME[rq.dept], PRIO_NAME[rq.triage_priority]);
    printf("  patients ahead of you : %d\n", rep.ahead);
    printf("  your consultation     : about %.1f min\n", rep.est);
    printf("  expected wait         : %.1f min  (likely between %.1f and %.1f)\n", rep.eta, rep.lo, rep.hi);
    printf("  %s\n", rep.msg);
    if (!wait_mode) return 0;

    printf("Waiting for the call... (Ctrl-C to leave)\n"); fflush(stdout);
    while (!finished) {
        sigsuspend(&old);                    /* atomically unblock + sleep until a signal */
        if (called) { called = 0; printf(">>> Token #%d: please go to Doctor room %d now!\n", rep.token, (int)room); fflush(stdout); }
    }
    printf(">>> Consultation finished. Take care!\n");
    return 0;
}
