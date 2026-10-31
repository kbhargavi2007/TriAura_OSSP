/* qdoctor.c - one doctor = one process (the "CPU core" of the clinic)
 *
 * Started by the reception with fork() + exec().  Talks to reception over two
 * anonymous pipes.  Shares the board (mmap) with everybody.
 *
 * CO-2: process lifecycle (READY -> RUNNING(consulting) -> STOPPED by job control -> EXIT)
 * CO-3: sigaction handlers: SIGUSR1 (preempt, payload via sigqueue), SIGTERM, SIGCONT
 * CO-5: open(O_APPEND) + write() of records, mmap of the board
 * CO-6: process-shared robust mutex + counting semaphore (2 printers)
 *       lock order is always  records_lock -> printers  (prevents deadlock)
 */
#include "queueless.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

static volatile sig_atomic_t g_quit = 0, g_cont = 0, g_preempt_id = 0;

static void on_usr1(int s, siginfo_t *si, void *u)
{   /* preempt request: payload = the patient id that must be preempted */
    (void)s; (void)u;
    g_preempt_id = si->si_value.sival_int;
}
static void on_term(int s) { (void)s; g_quit = 1; }
static void on_cont(int s) { (void)s; g_cont = 1; }

static int id, rfd, wfd, recfd;
static double scale;
static Board *bd;

static void send_msg(int type, int pid_, long work_ms)
{
    DocMsg m = { type, id, pid_, work_ms };
    write_full(wfd, &m, sizeof m);
}

static void set_elapsed(double minutes)
{
    rlock(&bd->lock);
    bd->docs[id].elapsed_min = minutes;
    pthread_mutex_unlock(&bd->lock);
}

static void store_record(const Assign *a, long done_ms)
{
    Record r; memset(&r, 0, sizeof r);
    r.patient_id = a->patient_id; memcpy(r.name, a->name, sizeof r.name);
    r.age = a->age; r.dept = a->dept; r.visit = a->visit; r.complaints = a->complaints;
    r.has_reports = a->has_reports; r.triage_priority = a->triage_priority; r.doctor = id;
    r.est_min = a->est_total_min;
    r.actual_min = (float)((a->work_prev_ms + done_ms) / scale);
    r.ts = time(NULL);

    rlock(&bd->records_lock);                       /* 1st resource */
    struct timespec abs; clock_gettime(CLOCK_REALTIME, &abs); abs.tv_sec += 2;
    int got = 0;                                    /* 2nd resource (counting semaphore) */
    for (;;) {
        if (sem_timedwait(&bd->printers, &abs) == 0) { got = 1; break; }
        if (errno != EINTR) break;                  /* timeout: print later, don't hang */
    }
    write_full(recfd, &r, sizeof r);                /* O_APPEND => atomic append */
    if (got) {
        struct timespec ts = { 0, 30 * 1000000L };  /* "printing prescription" */
        nanosleep(&ts, NULL);
        sem_post(&bd->printers);
    }
    pthread_mutex_unlock(&bd->records_lock);
}

/* returns 1 if the consultation was preempted/aborted, 0 if it finished */
static int consult(const Assign *a)
{
    long done = 0; int tick = 0;
    set_elapsed(0);
    while (done < a->work_ms) {
        if (g_preempt_id == a->patient_id || g_quit) {
            g_preempt_id = 0;
            set_elapsed(0);
            send_msg(MSG_PREEMPTED, a->patient_id, done);   /* "context switch": save progress */
            return 1;
        }
        struct timespec ts = { 0, TICK_MS * 1000000L };
        if (nanosleep(&ts, NULL) == 0) done += TICK_MS;      /* stopped time is NOT counted */
        if (g_cont) g_cont = 0;                              /* resumed after SIGSTOP/SIGCONT */
        if (++tick % 2 == 0) set_elapsed(done / scale);
    }
    if (done > a->work_ms) done = a->work_ms;
    set_elapsed(done / scale);
    store_record(a, done);
    send_msg(MSG_DONE, a->patient_id, done);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 6) { fprintf(stderr, "qdoctor is started by qserver\n"); return 2; }
    id = atoi(argv[1]); rfd = atoi(argv[2]); wfd = atoi(argv[3]);
    scale = atof(argv[4]);

    sigset_t empty; sigemptyset(&empty); sigprocmask(SIG_SETMASK, &empty, NULL);
    struct sigaction sa; memset(&sa, 0, sizeof sa); sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = on_usr1; sa.sa_flags = SA_SIGINFO;     /* no SA_RESTART: we want EINTR */
    sigaction(SIGUSR1, &sa, NULL);
    memset(&sa, 0, sizeof sa); sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_term; sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = on_cont; sigaction(SIGCONT, &sa, NULL);
    signal(SIGINT, SIG_IGN);            /* Ctrl-C at the terminal must not kill doctors */
    signal(SIGPIPE, SIG_DFL);

    int bfd = open(argv[5], O_RDWR);
    if (bfd < 0) { perror("qdoctor: open board"); return 1; }
    bd = mmap(NULL, sizeof(Board), PROT_READ | PROT_WRITE, MAP_SHARED, bfd, 0);
    if (bd == MAP_FAILED) { perror("qdoctor: mmap"); return 1; }
    close(bfd);
    recfd = open(RECORDS_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (recfd < 0) { perror("qdoctor: records"); return 1; }

    send_msg(MSG_READY, 0, 0);
    for (;;) {
        Assign a;
        ssize_t r = read(rfd, &a, sizeof a);
        if (r < 0) { if (errno == EINTR) { if (g_quit) break; continue; } break; }
        if (r == 0 || (size_t)r < sizeof a) break;           /* reception closed the pipe */
        if (a.cmd == CMD_QUIT) break;
        consult(&a);
        if (g_quit) break;
    }
    close(recfd);
    munmap(bd, sizeof(Board));
    return 0;
}
