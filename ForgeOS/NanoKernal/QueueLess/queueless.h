/* queueless.h - shared definitions for QueueLess (server, doctors, client, status)
 *
 * OS concepts used here:
 *   CO-3  named pipes (REG_FIFO), sigqueue payloads
 *   CO-5  memory-mapped file (BOARD_PATH) shared by every process
 *   CO-6  process-shared robust mutex + counting semaphore living inside the mmap
 */
#ifndef QUEUELESS_H
#define QUEUELESS_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>

#define MAX_DOCTORS  8
#define MAX_BOARD_Q  48
#define NDEPT        6
#define NFEAT        4
#define TICK_MS      50

#define REG_FIFO     "/tmp/queueless.reg"
#define BOARD_PATH   "/tmp/queueless.board"
#define DATA_DIR     "qdata"
#define RECORDS_PATH DATA_DIR "/records.dat"
#define LOG_PATH     DATA_DIR "/queueless.log"
#define BOARD_MAGIC  0x514C4553u

enum { PRIO_EMERGENCY = 0, PRIO_URGENT = 1, PRIO_NORMAL = 2 };
enum { DOC_STARTING = 0, DOC_IDLE, DOC_BUSY, DOC_PAUSED, DOC_DOWN };
enum { MSG_READY = 1, MSG_DONE, MSG_PREEMPTED };
/* WHO sent a registration.  SRC_KIOSK = 0 on purpose: a zero-filled request is a kiosk request,
 * which can NEVER carry a priority.  Only the triage desk (qtriage / reception shell) may. */
enum { SRC_KIOSK = 0, SRC_TRIAGE = 1, SRC_TRIAGE_UPDATE = 2 };
enum { CMD_ASSIGN = 1, CMD_QUIT };

static const char *DEPT_NAME[NDEPT] = {"General", "Cardiology", "Orthopedics",
                                        "Pediatrics", "Dermatology", "ENT"};
static const char *PRIO_NAME[3]     = {"EMERGENCY", "URGENT", "NORMAL"};
static const char *DOC_STATE_NAME[] = {"STARTING", "IDLE", "BUSY", "PAUSED", "DOWN"};

/* ---- registration -> reception (written to the named pipe; < PIPE_BUF => atomic) ----
 * triage_priority is meaningful ONLY when source is SRC_TRIAGE / SRC_TRIAGE_UPDATE: it is the
 * decision of authorised triage staff.  The server ignores it in a SRC_KIOSK request. */
typedef struct {
    pid_t   client_pid;
    int     token;               /* SRC_TRIAGE_UPDATE: which registered patient */
    char    name[32];
    uint8_t age, dept, visit /*0 new,1 follow-up*/, complaints /*1..5*/;
    uint8_t has_reports, triage_priority, notify, source;
    char    reply_fifo[64];
} RegRequest;

typedef struct {
    int    status;               /* 0 ok, 1 waiting room full, 2 bad request */
    int    token, ahead;
    double est, eta, lo, hi;     /* minutes */
    char   msg[96];
} RegReply;

/* ---- reception -> doctor (anonymous pipe) ---- */
typedef struct {
    int     cmd, patient_id;
    char    name[32];
    uint8_t age, dept, visit, complaints, has_reports, triage_priority, _pad[2];
    float   est_total_min;       /* estimate given at registration */
    long    work_ms;             /* remaining simulated consultation work */
    long    work_prev_ms;        /* work already done before a preemption */
} Assign;

/* ---- doctor -> reception (anonymous pipe) ---- */
typedef struct { int type, doctor_id, patient_id; long work_ms; } DocMsg;

/* ---- persistent consultation history (fixed-size records, pread/pwrite/mmap) ---- */
typedef struct {
    int     patient_id;
    char    name[32];
    uint8_t age, dept, visit, complaints, has_reports, triage_priority, _pad[2];
    int     doctor;
    float   est_min, actual_min;
    int64_t ts;
} Record;

/* ---- the shared board: lives in a MAP_SHARED file mapping ---- */
typedef struct {
    int    id; pid_t pid; int state, patient_id;
    char   patient_name[32];
    double est_min, elapsed_min, speed;
    int    served;
} DocSlot;

typedef struct {
    int    id; char name[32]; int dept, prio;
    double est_min, eta, lo, hi, waited;
} QSlot;

typedef struct {
    uint32_t        magic;
    pthread_mutex_t lock;          /* process-shared + robust: protects the board  */
    pthread_mutex_t records_lock;  /* process-shared + robust: records file access */
    sem_t           printers;      /* process-shared counting semaphore (2 printers)*/
    int             ndoctors, nq;
    int             awaiting;      /* registered, waiting for triage to assign a priority */
    DocSlot         docs[MAX_DOCTORS];
    QSlot           q[MAX_BOARD_Q];
    long            served, emergencies, preemptions, rejected;
    double          avg_wait, mae_learned, mae_naive;
    time_t          updated;
    uint64_t        version;
} Board;

static inline double mono_sec(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* lock a robust mutex; recover if the previous owner died (EOWNERDEAD) */
static inline int rlock(pthread_mutex_t *m)
{
    int r = pthread_mutex_lock(m);
    if (r == EOWNERDEAD) { pthread_mutex_consistent(m); r = 0; }
    return r;
}

static inline int read_full(int fd, void *buf, size_t n)
{
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, (char *)buf + got, n - got);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return got ? -1 : 0;
        got += (size_t)r;
    }
    return 1;
}

static inline int write_full(int fd, const void *buf, size_t n)
{
    size_t put = 0;
    while (put < n) {
        ssize_t r = write(fd, (const char *)buf + put, n - put);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        put += (size_t)r;
    }
    return 0;
}

static inline void print_board(FILE *f, const Board *b)
{
    fprintf(f, "\n================ QueueLess board (v%lu) ================\n",
            (unsigned long)b->version);
    for (int i = 0; i < b->ndoctors; i++) {
        const DocSlot *d = &b->docs[i];
        fprintf(f, " Dr.%d %-8s pid=%-6d speed=x%.2f served=%-3d", d->id,
                DOC_STATE_NAME[d->state], d->pid, d->speed, d->served);
        if (d->patient_id)
            fprintf(f, " | #%d %s  est %.1f, elapsed %.1f min", d->patient_id,
                    d->patient_name, d->est_min, d->elapsed_min);
        fputc('\n', f);
    }
    fprintf(f, "----------------------------------------------------------\n");
    if (b->nq == 0) fprintf(f, " (waiting room empty)\n");
    else fprintf(f, " %-5s %-12s %-12s %-9s %6s %16s %7s\n", "TOKEN", "NAME", "DEPT",
                 "PRIORITY", "EST", "ETA (80% range)", "WAITED");
    for (int i = 0; i < b->nq; i++) {
        const QSlot *q = &b->q[i];
        fprintf(f, " #%-4d %-12s %-12s %-9s %5.1fm %6.1f (%4.1f-%4.1f)m %6.1fm\n", q->id,
                q->name, DEPT_NAME[q->dept], PRIO_NAME[q->prio], q->est_min, q->eta,
                q->lo, q->hi, q->waited);
    }
    if (b->awaiting)
        fprintf(f, " %d patient(s) registered and AWAITING TRIAGE (not schedulable yet)\n", b->awaiting);
    fprintf(f, "----------------------------------------------------------\n");
    fprintf(f, " served=%ld emergencies=%ld preemptions=%ld rejected=%ld avg_wait=%.1fm\n",
            b->served, b->emergencies, b->preemptions, b->rejected, b->avg_wait);
    if (b->served)
        fprintf(f, " estimate error (MAE): learned=%.2f min  vs  naive fixed-average=%.2f min\n",
                b->mae_learned, b->mae_naive);
    fprintf(f, "==========================================================\n");
}

#endif
