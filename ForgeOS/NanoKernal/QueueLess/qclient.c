/* qclient.c - patient kiosk (a separate process)
 *
 * PATIENT KIOSK: collects registration details only.  There is NO priority option here: priority is assigned by the
 * triage desk (./qtriage).  A kiosk registration is queued as "awaiting triage" by the server.
 *
 * CO-3: sends the registration through a NAMED PIPE (FIFO), gets its reply through a
 *       private FIFO, and (with -w) waits ASYNCHRONOUSLY for signals from reception:
 *          SIGUSR1 (sigqueue payload = room number)  -> "your turn"
 *          SIGUSR2                                    -> "consultation finished"
 *       sigsuspend() is used to avoid the classic lost-wakeup race.
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
        "usage: qclient -n NAME -a AGE -d DEPT -v new|fu -c COMPLAINTS -r 0|1 [-w]\n"
        "  DEPT: 0 General 1 Cardiology 2 Orthopedics 3 Pediatrics 4 Dermatology 5 ENT\n"
        "  -c  number of separate complaints (1-5)   -r  bringing old reports/tests (0/1)\n"
        "  Priority is NOT chosen by the patient: the triage desk assigns it (./qtriage -t TOKEN -p E|U|N).\n"
        "  -w  stay and wait for the call (signals) instead of just registering\n");
    exit(2);
}

int main(int argc, char **argv)
{
    RegRequest rq; memset(&rq, 0, sizeof rq);
    strcpy(rq.name, "patient"); rq.age = 30; rq.complaints = 1; rq.source = SRC_KIOSK;
    int wait_mode = 0, opt;
    while ((opt = getopt(argc, argv, "n:a:d:v:c:r:wh")) != -1) {
        switch (opt) {
        case 'n': strncpy(rq.name, optarg, sizeof rq.name - 1); break;
        case 'a': rq.age = (uint8_t)atoi(optarg); break;
        case 'd': rq.dept = (uint8_t)atoi(optarg); break;
        case 'v': rq.visit = (optarg[0] == 'f' || optarg[0] == 'F'); break;
        case 'c': rq.complaints = (uint8_t)atoi(optarg); break;
        case 'r': rq.has_reports = (uint8_t)atoi(optarg); break;
        case 'w': wait_mode = 1; break;
        default: usage();
        }
    }
    rq.client_pid = getpid(); rq.notify = (uint8_t)wait_mode;
    snprintf(rq.reply_fifo, sizeof rq.reply_fifo, "/tmp/queueless.reply.%d", getpid());

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
    if (rep.status != 0) { printf("Registration refused: %s\n", rep.msg); return 3; }

    printf("Token #%d for %s  (%s) - registered, awaiting triage\n", rep.token, rq.name, DEPT_NAME[rq.dept]);
    printf("  your consultation     : about %.1f min\n", rep.est);
    printf("  queue position and waiting time are shown after triage assigns your priority\n");
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
