/* qserver.c - QueueLess reception: scheduler + estimator + admin shell.
 *
 * Analogy:  patient = process   |  doctor = CPU core   |  waiting room = ready queue
 *           reception = OS scheduler   |  consultation = CPU burst   |  emergency = high-priority preemption
 *
 * ROLES:  priority is NOT decided here.  It is assigned by the triage desk (qtriage / reception shell)
 * as `triage_priority`; the scheduler only orders patients by that value.  qserver never diagnoses.
 *
 * THE BIG IDEA (CO-2 scheduling): the CPU burst (consultation time) is unknown in advance,
 * exactly like SJF in a real OS.  We do what real schedulers do: PREDICT it from history
 * (exponential averaging) + cheap features that a receptionist CAN collect without diagnosing
 * anything (department, new/follow-up, age band, number of complaints, reports to review),
 * learn per-doctor speed, and keep correcting the estimate while the consultation runs.
 *
 * Threads:  main(admin shell) | reg (FIFO) | sched (cond var) | collector (poll on pipes)
 *           | signal (sigwait) | logger (bounded buffer with counting semaphores)
 *
 * Lock order (prevents deadlock):  qlock  ->  est_lock  ->  board->lock   (never the reverse)
 */
#include "queueless.h"
#include "oslab.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <getopt.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define AGING 0.5          /* 1 minute of waiting lowers the sort key by 0.5 min (anti-starvation) */

typedef struct Patient {
    int id; char name[32];
    uint8_t age, dept, visit, complaints, has_reports, triage_priority;
    pid_t client_pid; int notify, uses_slot;
    double t_arrive, est_min, sd_min, actual_min;
    double true_min, frac_done;          /* SIMULATION ONLY: hidden "reality"; scheduler never reads it */
    int state, doc, waited_counted;      /* 0 waiting (triaged), 1 running, 2 registered but AWAITING TRIAGE */
    struct Patient *next;
} Patient;

typedef struct {
    int id; pid_t pid; int to_fd, from_fd, state;
    Patient *cur; double sess_est, sess_sd; int preempt_sent, restarts, served;
} Doc;

/* ------------------------------------------------------------ globals */
static atomic_int g_shutdown = 0, g_sig_exit = 0;   /* shared between THREADS => must be atomic (volatile sig_atomic_t is only for signal handlers) */
static int      wake_pipe[2] = { -1, -1 }, reg_fd = -1, g_quiet = 0, ndocs = 3, wroom_cap = 15;
static double   scale_ms = 250.0;               /* real ms per simulated minute */
static char     doctor_path[512];
static Board   *board;
static sem_t    waiting_room;                   /* counting semaphore = free chairs */
static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  qcond = PTHREAD_COND_INITIALIZER;
static Patient *qhead, *qtail; static int next_id = 1;
static Doc      docs[MAX_DOCTORS]; static pid_t doc_pgid = 0;
static long     st_served, st_emerg, st_preempt; static double st_wait_sum;
static atomic_long st_rejected;
static pthread_t th_reg, th_sched, th_coll, th_sig, th_log;

static double sec2min(double s) { return s * 1000.0 / scale_ms; }

/* ------------------------------------------------------------ logger: bounded buffer (CO-6 semaphores) */
#define LOG_SLOTS 256
#define LOG_LEN   240
static char logbuf[LOG_SLOTS][LOG_LEN]; static int log_in, log_out;
static sem_t log_empty, log_full; static pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER;
static FILE *logfp;

static void sem_wait_r(sem_t *s) { while (sem_wait(s) < 0 && errno == EINTR) ; }
static void log_put(const char *line)
{
    sem_wait_r(&log_empty);                                   /* wait for a free slot   */
    pthread_mutex_lock(&log_mtx);
    strncpy(logbuf[log_in], line, LOG_LEN - 1); logbuf[log_in][LOG_LEN - 1] = 0;
    log_in = (log_in + 1) % LOG_SLOTS;
    pthread_mutex_unlock(&log_mtx);
    sem_post(&log_full);                                      /* announce a filled slot */
}
static void logmsg(const char *fmt, ...)
{
    char line[LOG_LEN]; time_t t = time(NULL); struct tm tm; localtime_r(&t, &tm);
    int n = (int)strftime(line, sizeof line, "[%H:%M:%S] ", &tm);
    va_list ap; va_start(ap, fmt); vsnprintf(line + n, sizeof line - (size_t)n - 2, fmt, ap); va_end(ap);
    size_t l = strlen(line); line[l] = '\n'; line[l + 1] = 0;
    log_put(line);
}
static void *logger_thread(void *a)
{
    (void)a; char line[LOG_LEN];
    for (;;) {
        sem_wait_r(&log_full);
        pthread_mutex_lock(&log_mtx);
        memcpy(line, logbuf[log_out], LOG_LEN); log_out = (log_out + 1) % LOG_SLOTS;
        pthread_mutex_unlock(&log_mtx);
        sem_post(&log_empty);
        if (line[0] == '\x04') break;
        if (!g_quiet) { ssize_t w = write(STDOUT_FILENO, line, strlen(line)); (void)w; } /* UNBUFFERED: 1 syscall/line */
        fputs(line, logfp);                                                              /* BUFFERED: stdio batches   */
        int v; sem_getvalue(&log_full, &v); if (v == 0) fflush(logfp);
    }
    fclose(logfp); return NULL;
}

/* ------------------------------------------------------------ estimator */
typedef struct {
    double mean[NDEPT][2], var[NDEPT][2]; int n[NDEPT][2];
    double w[NFEAT], speed[MAX_DOCTORS]; int dn[MAX_DOCTORS];
    double sae_learn, sae_naive; long cnt;
} Est;
static Est est; static pthread_rwlock_t est_lock = PTHREAD_RWLOCK_INITIALIZER;
static const double PRIOR[NDEPT] = { 8, 15, 12, 10, 7, 9 };                 /* what a receptionist would guess */
static const double TRUE_BASE[NDEPT][2] = { {9,5}, {18,9}, {14,8}, {11,6}, {8,4}, {10,5} }; /* hidden reality */
static const double TRUE_SPEED[MAX_DOCTORS] = { 1.0, 0.85, 1.25, 1.0, 1.1, 0.9, 1.0, 1.0 };

static void est_init(void)
{
    memset(&est, 0, sizeof est);
    for (int d = 0; d < NDEPT; d++) for (int v = 0; v < 2; v++) { est.mean[d][v] = PRIOR[d]; est.var[d][v] = (0.3 * PRIOR[d]) * (0.3 * PRIOR[d]); }
    for (int i = 0; i < MAX_DOCTORS; i++) est.speed[i] = 1.0;
}
static void features(uint8_t age, uint8_t compl_, uint8_t rep, double x[NFEAT])
{ x[0] = age >= 60; x[1] = age < 12; x[2] = compl_ > 0 ? compl_ - 1 : 0; x[3] = rep; }

/* predicted consultation minutes for patient p on doctor `doc` (-1 = average doctor) */
static double predict(const Patient *p, int doc, double *sd)
{
    double x[NFEAT]; features(p->age, p->complaints, p->has_reports, x);
    pthread_rwlock_rdlock(&est_lock);
    double m = est.mean[p->dept][p->visit], s = sqrt(est.var[p->dept][p->visit]);
    for (int i = 0; i < NFEAT; i++) m += est.w[i] * x[i];
    if (doc >= 0) { m *= est.speed[doc]; s *= est.speed[doc]; }
    pthread_rwlock_unlock(&est_lock);
    if (m < 2) m = 2;
    if (sd) *sd = s;
    return m;
}
static double doc_speed(int k) { pthread_rwlock_rdlock(&est_lock); double s = est.speed[k]; pthread_rwlock_unlock(&est_lock); return s; }

/* learn from one finished consultation:  tau(n+1) = alpha*t(n) + (1-alpha)*tau(n)  (+ LMS on features) */
static void est_update(const Patient *p, int doc, double actual)
{
    double x[NFEAT]; features(p->age, p->complaints, p->has_reports, x);
    pthread_rwlock_wrlock(&est_lock);
    int d = p->dept, v = p->visit;
    double a = actual / est.speed[doc], lin = 0, xx = 0;
    for (int i = 0; i < NFEAT; i++) { lin += est.w[i] * x[i]; xx += x[i] * x[i]; }
    double pred = est.mean[d][v] + lin, err = a - pred;
    est.sae_learn += fabs(actual - p->est_min); est.sae_naive += fabs(actual - PRIOR[d]); est.cnt++;
    for (int i = 0; i < NFEAT; i++) est.w[i] += 0.15 * err * x[i] / (1.0 + xx);   /* normalised LMS */
    lin = 0; for (int i = 0; i < NFEAT; i++) lin += est.w[i] * x[i];
    double alpha = fmax(0.2, 1.0 / (est.n[d][v] + 3));                            /* prior = 3 pseudo-samples */
    est.mean[d][v] += alpha * (a - lin - est.mean[d][v]); est.n[d][v]++;
    est.var[d][v] = fmax(0.25, 0.85 * est.var[d][v] + 0.15 * err * err);
    double r = actual / fmax(pred, 1.0); r = fmin(2.0, fmax(0.5, r));
    est.speed[doc] = est.dn[doc] ? 0.8 * est.speed[doc] + 0.2 * r : r; est.dn[doc]++;
    double sum = 0; int c = 0;                                                     /* keep mean speed = 1 */
    for (int k = 0; k < MAX_DOCTORS; k++) if (est.dn[k]) { sum += est.speed[k]; c++; }
    if (c) { double m = sum / c;
        for (int k = 0; k < MAX_DOCTORS; k++) if (est.dn[k]) est.speed[k] /= m;
        for (int i = 0; i < NDEPT; i++) for (int j = 0; j < 2; j++) { est.mean[i][j] *= m; est.var[i][j] *= m * m; }
        for (int i = 0; i < NFEAT; i++) est.w[i] *= m; }
    pthread_rwlock_unlock(&est_lock);
}

static void est_load_history(void)
{
    int fd = open(RECORDS_PATH, O_RDONLY); if (fd < 0) return;
    Record r; long n = 0;
    while (read(fd, &r, sizeof r) == (ssize_t)sizeof r) {
        Patient p; memset(&p, 0, sizeof p);
        p.age = r.age; p.dept = r.dept % NDEPT; p.visit = r.visit & 1; p.complaints = r.complaints; p.has_reports = r.has_reports; p.est_min = r.est_min;
        est_update(&p, r.doctor >= 0 && r.doctor < ndocs ? r.doctor : 0, r.actual_min); n++;
    }
    close(fd);
    est.sae_learn = est.sae_naive = 0; est.cnt = 0;                             /* report only live accuracy */
    if (n) logmsg("estimator warm-started from %ld past consultations (%s)", n, RECORDS_PATH);
}

/* ------------------------------------------------------------ hidden reality (simulation) */
static double gauss(void) { double u = (drand48() + 1e-12), v = drand48(); return sqrt(-2 * log(u)) * cos(6.283185307 * v); }
static double simulate_true(const Patient *p)
{
    double b = TRUE_BASE[p->dept][p->visit];
    if (p->age >= 60) b *= 1.25;
    if (p->age < 12)  b *= 1.15;
    b += 2.0 * (p->complaints - 1) + (p->has_reports ? 3.0 : 0.0);
    b *= exp(0.18 * gauss());
    return b < 2 ? 2 : b;
}

/* ------------------------------------------------------------ scheduling helpers (qlock held) */
static double sched_key(const Patient *p, double now)
{
    double sd, r = predict(p, -1, &sd) - p->actual_min; if (r < 1) r = 1;
    return r - AGING * sec2min(now - p->t_arrive);
}
static int better(const Patient *a, double ka, const Patient *b, double kb)
{ return a->triage_priority != b->triage_priority ? a->triage_priority < b->triage_priority : ka < kb; }

static double board_elapsed(int k)
{ rlock(&board->lock); double e = board->docs[k].elapsed_min; pthread_mutex_unlock(&board->lock); return e; }

/* simulate the whole queue on all doctors (list scheduling) to give every waiting patient an ETA */
static int plan_locked(Patient **ord, double *eta, double *sdv, int cap)
{
    double now = mono_sec(), key[128]; int n = 0;
    if (cap > 128) cap = 128;
    for (Patient *p = qhead; p && n < cap; p = p->next) if (p->state == 0) { ord[n] = p; key[n] = sched_key(p, now); n++; }
    for (int i = 1; i < n; i++) {                                             /* insertion sort */
        Patient *pp = ord[i]; double kk = key[i]; int j = i - 1;
        while (j >= 0 && better(pp, kk, ord[j], key[j])) { ord[j + 1] = ord[j]; key[j + 1] = key[j]; j--; }
        ord[j + 1] = pp; key[j + 1] = kk;
    }
    double free_at[MAX_DOCTORS], var_at[MAX_DOCTORS];
    for (int k = 0; k < ndocs; k++) {
        Doc *d = &docs[k]; var_at[k] = 0;
        if (d->state == DOC_DOWN) { free_at[k] = 1e9; continue; }
        if (d->state == DOC_BUSY || d->state == DOC_PAUSED) {
            double el = board_elapsed(k), rem = d->sess_est - el;
            if (rem <= 0) rem = fmax(1.0, 0.25 * el);                         /* overrun: still running => expect more */
            free_at[k] = rem; var_at[k] = d->sess_sd * d->sess_sd;
        } else free_at[k] = 0;
    }
    for (int i = 0; i < n; i++) {
        int k = 0; for (int j = 1; j < ndocs; j++) if (free_at[j] < free_at[k]) k = j;
        eta[i] = free_at[k]; sdv[i] = sqrt(var_at[k]);
        double s, pr = predict(ord[i], k, &s) - ord[i]->actual_min; if (pr < 1) pr = 1;
        free_at[k] += pr; var_at[k] += s * s;
    }
    return n;
}

static void publish_board_locked(void)
{
    Patient *ord[128]; double eta[128], sdv[128]; int n = plan_locked(ord, eta, sdv, 128);
    double spd[MAX_DOCTORS], mae_l = 0, mae_n = 0;
    for (int k = 0; k < ndocs; k++) spd[k] = doc_speed(k);
    pthread_rwlock_rdlock(&est_lock);
    if (est.cnt) { mae_l = est.sae_learn / est.cnt; mae_n = est.sae_naive / est.cnt; }
    pthread_rwlock_unlock(&est_lock);
    double now = mono_sec();
    rlock(&board->lock);
    board->ndoctors = ndocs;
    for (int k = 0; k < ndocs; k++) {
        DocSlot *s = &board->docs[k]; Doc *d = &docs[k];
        s->id = k; s->pid = d->pid; s->state = d->state; s->speed = spd[k]; s->served = d->served;
        s->patient_id = d->cur ? d->cur->id : 0;
        if (d->cur) { snprintf(s->patient_name, sizeof s->patient_name, "%s", d->cur->name); s->est_min = d->sess_est; } else { s->patient_name[0] = 0; s->est_min = 0; s->elapsed_min = 0; }
    }
    { int aw = 0; for (Patient *w = qhead; w; w = w->next) if (w->state == 2) aw++; board->awaiting = aw; }
    board->nq = n < MAX_BOARD_Q ? n : MAX_BOARD_Q;
    for (int i = 0; i < board->nq; i++) {
        QSlot *q = &board->q[i]; Patient *p = ord[i];
        q->id = p->id; snprintf(q->name, sizeof q->name, "%s", p->name); q->dept = p->dept; q->prio = p->triage_priority;
        q->est_min = predict(p, -1, NULL); q->eta = eta[i];
        q->lo = fmax(0, eta[i] - 1.28 * sdv[i]); q->hi = eta[i] + 1.28 * sdv[i]; q->waited = sec2min(now - p->t_arrive);
    }
    board->served = st_served; board->emergencies = st_emerg; board->preemptions = st_preempt;
    board->rejected = atomic_load(&st_rejected); board->avg_wait = st_served ? st_wait_sum / st_served : 0;
    board->mae_learned = mae_l; board->mae_naive = mae_n; board->updated = time(NULL); board->version++;
    pthread_mutex_unlock(&board->lock);
}

static void notify_client(Patient *p, int sig, int val)
{
    if (!p->notify || p->client_pid <= 0 || kill(p->client_pid, 0) != 0) return;
    union sigval sv; sv.sival_int = val; sigqueue(p->client_pid, sig, sv);
}

static int assign_locked(Doc *d, Patient *p)
{
    double sd, es = predict(p, d->id, &sd) - p->actual_min; if (es < 1) es = 1;
    Assign a; memset(&a, 0, sizeof a);
    a.cmd = CMD_ASSIGN; a.patient_id = p->id; memcpy(a.name, p->name, sizeof a.name);
    a.age = p->age; a.dept = p->dept; a.visit = p->visit; a.complaints = p->complaints; a.has_reports = p->has_reports; a.triage_priority = p->triage_priority;
    a.est_total_min = (float)p->est_min;
    a.work_ms = (long)(p->true_min * (1.0 - p->frac_done) * TRUE_SPEED[d->id] * scale_ms);   /* hidden truth drives the doctor */
    a.work_prev_ms = (long)(p->actual_min * scale_ms);
    rlock(&board->lock); board->docs[d->id].elapsed_min = 0; pthread_mutex_unlock(&board->lock);
    if (write_full(d->to_fd, &a, sizeof a) < 0) { logmsg("write to Dr.%d failed: %s", d->id, strerror(errno)); return -1; }
    p->state = 1; p->doc = d->id; d->cur = p; d->state = DOC_BUSY; d->sess_est = es; d->sess_sd = sd; d->preempt_sent = 0;
    if (!p->waited_counted) { p->waited_counted = 1; st_wait_sum += sec2min(mono_sec() - p->t_arrive); }
    logmsg("CALL   token #%d %-10s (%s, %s) -> Dr.%d, predicted %.1f min", p->id, p->name, DEPT_NAME[p->dept], PRIO_NAME[p->triage_priority], d->id, es);
    notify_client(p, SIGUSR1, d->id);
    return 0;
}

static void maybe_preempt_locked(void)
{
    int E = 0, P = 0;
    for (Patient *p = qhead; p; p = p->next) if (p->state == 0 && p->triage_priority == PRIO_EMERGENCY) E++;
    for (int k = 0; k < ndocs; k++) if (docs[k].preempt_sent) P++;
    if (E <= P) return;
    Doc *victim = NULL; double worst = -1;
    for (int k = 0; k < ndocs; k++) {
        Doc *d = &docs[k];
        if (d->state != DOC_BUSY || !d->cur || d->cur->triage_priority == PRIO_EMERGENCY || d->preempt_sent) continue;
        double rem = d->sess_est - board_elapsed(k); if (rem > worst) { worst = rem; victim = d; }
    }
    if (!victim) return;
    union sigval sv; sv.sival_int = victim->cur->id;                          /* payload = who to preempt */
    if (sigqueue(victim->pid, SIGUSR1, sv) == 0) {
        victim->preempt_sent = 1;
        logmsg("PREEMPT Dr.%d: interrupting #%d %s for an emergency (SIGUSR1 via sigqueue)", victim->id, victim->cur->id, victim->cur->name);
    }
}

static void dispatch_locked(void)
{
    double now = mono_sec();
    for (;;) {
        Patient *best = NULL; double bk = 0;
        for (Patient *p = qhead; p; p = p->next) {
            if (p->state != 0) continue;
            double k = sched_key(p, now);
            if (!best || better(p, k, best, bk)) { best = p; bk = k; }
        }
        if (!best) return;
        Doc *pick = NULL; double bt = 1e18;
        for (int k = 0; k < ndocs; k++) if (docs[k].state == DOC_IDLE) { double t = predict(best, k, NULL); if (t < bt) { bt = t; pick = &docs[k]; } }
        if (pick) { if (assign_locked(pick, best) < 0) { pick->state = DOC_DOWN; } continue; }
        if (best->triage_priority == PRIO_EMERGENCY) maybe_preempt_locked();
        return;
    }
}

/* ------------------------------------------------------------ doctors: fork + exec */
static int spawn_doctor(Doc *d)
{
    int p2d[2], d2p[2];
    if (pipe2(p2d, O_CLOEXEC) < 0) return -1;
    if (pipe2(d2p, O_CLOEXEC) < 0) { close(p2d[0]); close(p2d[1]); return -1; }
    char a_id[16], a_r[16], a_w[16], a_s[32];                  /* build argv BEFORE fork (no malloc in child) */
    snprintf(a_id, sizeof a_id, "%d", d->id); snprintf(a_r, sizeof a_r, "%d", p2d[0]);
    snprintf(a_w, sizeof a_w, "%d", d2p[1]);  snprintf(a_s, sizeof a_s, "%.3f", scale_ms);
    pid_t target = doc_pgid;
    pid_t pid = fork();
    if (pid < 0) { close(p2d[0]); close(p2d[1]); close(d2p[0]); close(d2p[1]); return -1; }
    if (pid == 0) {                                            /* --- child: only async-signal-safe calls --- */
        if (setpgid(0, target) < 0) setpgid(0, 0);             /* all doctors in ONE process group */
        fcntl(p2d[0], F_SETFD, 0); fcntl(d2p[1], F_SETFD, 0);  /* keep only my two pipe ends across exec */
        sigset_t e; sigemptyset(&e); sigprocmask(SIG_SETMASK, &e, NULL);
        execl(doctor_path, "qdoctor", a_id, a_r, a_w, a_s, BOARD_PATH, (char *)NULL);
        _exit(127);
    }
    if (setpgid(pid, target) < 0 && errno != EACCES) setpgid(pid, 0);   /* set from both sides: no race */
    pid_t g = getpgid(pid); if (g > 0 && g != getpgrp()) doc_pgid = g;
    close(p2d[0]); close(d2p[1]);
    d->pid = pid; d->to_fd = p2d[1]; d->from_fd = d2p[0]; d->state = DOC_STARTING; d->cur = NULL; d->preempt_sent = 0;
    return 0;
}

/* ------------------------------------------------------------ registration */
static int valid_reply_path(const char *p)
{ return !strncmp(p, "/tmp/queueless.reply.", 21) && !strstr(p, "..") && !strchr(p + 5, '/'); }

/* TRIAGE UPDATE: authorised triage staff assign (or change) the priority of a patient who is already
 * registered.  The scheduler is not involved in the decision; it just sees the new triage_priority. */
static int triage_update(const RegRequest *rq, RegReply *rep)
{
    memset(rep, 0, sizeof *rep);
    if (rq->triage_priority > 2) { rep->status = 2; snprintf(rep->msg, sizeof rep->msg, "Invalid triage priority"); return -1; }
    pthread_mutex_lock(&qlock);
    Patient *p = qhead; while (p && p->id != rq->token) p = p->next;
    if (!p || p->state == 1) {
        int busy = p != NULL;
        pthread_mutex_unlock(&qlock);
        rep->status = 2;
        snprintf(rep->msg, sizeof rep->msg, busy ? "token #%d is already with a doctor" : "no waiting patient with token #%d", rq->token);
        return -1;
    }
    int first = p->state == 2, old = p->triage_priority;
    p->triage_priority = rq->triage_priority;
    if (first) p->state = 0;                                        /* now visible to the scheduler */
    if (p->triage_priority == PRIO_EMERGENCY && (first || old != PRIO_EMERGENCY)) st_emerg++;
    Patient *ord[128]; double eta[128], sdv[128]; int n = plan_locked(ord, eta, sdv, 128), idx = -1;
    for (int i = 0; i < n; i++) if (ord[i] == p) idx = i;
    rep->token = p->id; rep->ahead = idx < 0 ? 0 : idx; rep->est = p->est_min;
    if (idx >= 0) { rep->eta = eta[idx]; rep->lo = fmax(0, eta[idx] - 1.28 * sdv[idx]); rep->hi = eta[idx] + 1.28 * sdv[idx]; }
    snprintf(rep->msg, sizeof rep->msg, "Triage desk set priority %s for token #%d.", PRIO_NAME[p->triage_priority], p->id);
    logmsg("TRIAGE token #%d %-10s -> %s (%s by the triage desk)", p->id, p->name, PRIO_NAME[p->triage_priority], first ? "assigned" : "re-triaged");
    pthread_cond_broadcast(&qcond);                                 /* wake the scheduler AFTER the log line */
    pthread_mutex_unlock(&qlock);
    return 0;
}

static int admit(const RegRequest *rq, RegReply *rep)
{
    if (rq->source == SRC_TRIAGE_UPDATE) return triage_update(rq, rep);
    memset(rep, 0, sizeof *rep);
    /* Only the triage desk may supply a priority.  A kiosk registration (SRC_KIOSK, or anything unknown) has its
     * priority field IGNORED: the patient is queued as AWAITING TRIAGE and is invisible to the scheduler until
     * qtriage assigns one.  PRIO_NORMAL below is just a placeholder while awaiting. */
    int triaged = rq->source == SRC_TRIAGE;
    int prio = triaged ? rq->triage_priority : PRIO_NORMAL;
    if (rq->age > 120 || rq->dept >= NDEPT || rq->visit > 1 || rq->complaints < 1 || rq->complaints > 5 || prio > 2) {
        rep->status = 2; snprintf(rep->msg, sizeof rep->msg, "Invalid registration form"); return -1;
    }
    int emergency = triaged && prio == PRIO_EMERGENCY;
    if (!emergency && sem_trywait(&waiting_room) != 0) {
        atomic_fetch_add(&st_rejected, 1); rep->status = 1;
        snprintf(rep->msg, sizeof rep->msg, "Waiting room full (%d chairs). Please come back shortly.", wroom_cap);
        logmsg("REJECT %s: waiting room full", rq->name); return -1;
    }
    Patient *p = calloc(1, sizeof *p);
    if (!p) { if (!emergency) sem_post(&waiting_room); rep->status = 2; snprintf(rep->msg, sizeof rep->msg, "server out of memory"); return -1; }
    snprintf(p->name, sizeof p->name, "%s", rq->name[0] ? rq->name : "patient");
    p->age = rq->age; p->dept = rq->dept; p->visit = rq->visit; p->complaints = rq->complaints; p->has_reports = rq->has_reports;
    p->triage_priority = (uint8_t)prio; p->state = triaged ? 0 : 2;
    p->client_pid = rq->client_pid; p->notify = rq->notify; p->uses_slot = !emergency; p->doc = -1;
    p->t_arrive = mono_sec();
    double sd; p->est_min = predict(p, -1, &sd); p->sd_min = sd;

    pthread_mutex_lock(&qlock);
    p->id = next_id++; p->true_min = simulate_true(p);
    if (qtail) qtail->next = p; else qhead = p; qtail = p;
    if (emergency) st_emerg++;
    Patient *ord[128]; double eta[128], sdv[128]; int n = plan_locked(ord, eta, sdv, 128), idx = -1;
    for (int i = 0; i < n; i++) if (ord[i] == p) idx = i;
    rep->token = p->id; rep->ahead = idx < 0 ? 0 : idx; rep->est = p->est_min;
    if (idx >= 0) { rep->eta = eta[idx]; rep->lo = fmax(0, eta[idx] - 1.28 * sdv[idx]); rep->hi = eta[idx] + 1.28 * sdv[idx]; }
    if (!triaged) snprintf(rep->msg, sizeof rep->msg, "Registered. Waiting for the triage desk to assign a priority (qtriage -t %d -p E|U|N)", p->id);
    else snprintf(rep->msg, sizeof rep->msg, emergency ? "EMERGENCY: a doctor will be called immediately." : "Estimate updates live; watch ./qstatus -w");
    /* log BEFORE waking the scheduler so the log always reads  TRIAGE -> ADMIT -> CALL  */
    if (triaged) {
        logmsg("TRIAGE token #%d %-10s -> %s (assigned by the triage desk)", p->id, p->name, PRIO_NAME[prio]);
        logmsg("ADMIT  token #%d %-10s age %u %s %s %s | predicted %.1f min | ETA %.1f min", p->id, p->name, p->age, DEPT_NAME[p->dept],
               p->visit ? "follow-up" : "new", PRIO_NAME[prio], rep->est, rep->eta);
    } else
        logmsg("REGISTER token #%d %-10s age %u %s %s | AWAITING TRIAGE (not schedulable until a priority is assigned)", p->id, p->name,
               p->age, DEPT_NAME[p->dept], p->visit ? "follow-up" : "new");
    pthread_cond_broadcast(&qcond);
    pthread_mutex_unlock(&qlock);
    return 0;
}

static void *reg_thread(void *a)
{
    (void)a; RegRequest rq;
    for (;;) {
        int r = read_full(reg_fd, &rq, sizeof rq);
        if (r <= 0) { if (g_shutdown) break; continue; }
        if (rq.client_pid == -1 || g_shutdown) break;                 /* shutdown sentinel */
        rq.name[sizeof rq.name - 1] = 0; rq.reply_fifo[sizeof rq.reply_fifo - 1] = 0;
        RegReply rep; admit(&rq, &rep);
        if (valid_reply_path(rq.reply_fifo)) {
            int fd = open(rq.reply_fifo, O_WRONLY | O_NONBLOCK);       /* named pipe back to the patient */
            if (fd >= 0) { write_full(fd, &rep, sizeof rep); close(fd); }
        }
    }
    return NULL;
}

static void *sched_thread(void *a)
{
    (void)a; pthread_mutex_lock(&qlock);
    while (!g_shutdown) {
        dispatch_locked(); publish_board_locked();
        struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); ts.tv_sec += 1;
        pthread_cond_timedwait(&qcond, &qlock, &ts);                   /* woken by events, or every 1 s to refresh ETAs */
    }
    pthread_mutex_unlock(&qlock); return NULL;
}

static void handle_msg_locked(Doc *d, const DocMsg *m)
{
    if (m->type == MSG_READY) { if (d->state == DOC_STARTING) d->state = DOC_IDLE; logmsg("Dr.%d ready (pid %d)", d->id, d->pid); return; }
    Patient *p = d->cur;
    if (!p || p->id != m->patient_id) return;
    double sess = m->work_ms / scale_ms;
    p->actual_min += sess; d->cur = NULL; d->state = DOC_IDLE; p->doc = -1;
    if (m->type == MSG_PREEMPTED) {
        double full = p->true_min * TRUE_SPEED[d->id];
        p->frac_done += full > 0 ? sess / full : 0; if (p->frac_done > 0.98) p->frac_done = 0.98;
        p->state = 0; st_preempt++;
        logmsg("BACK   token #%d %s returned to the queue after %.1f min (progress saved)", p->id, p->name, p->actual_min);
    } else {
        double err = p->actual_min - p->est_min;
        est_update(p, d->id, p->actual_min);
        st_served++; d->served++;
        logmsg("DONE   token #%d %-10s Dr.%d: predicted %.1f min, actual %.1f min (error %+.1f)", p->id, p->name, d->id, p->est_min, p->actual_min, err);
        notify_client(p, SIGUSR2, 0);
        for (Patient **pp = &qhead; *pp; pp = &(*pp)->next) if (*pp == p) { *pp = p->next; if (qtail == p) { Patient *t = qhead; while (t && t->next) t = t->next; qtail = t; } break; }
        if (p->uses_slot) sem_post(&waiting_room);
        free(p);
    }
    pthread_cond_broadcast(&qcond);
}

static void *collector_thread(void *a)
{
    (void)a;
    while (!g_shutdown) {
        struct pollfd pf[MAX_DOCTORS + 1]; int map[MAX_DOCTORS + 1], n = 0;
        pthread_mutex_lock(&qlock);
        for (int k = 0; k < ndocs; k++) if (docs[k].from_fd >= 0) { pf[n].fd = docs[k].from_fd; pf[n].events = POLLIN; pf[n].revents = 0; map[n] = k; n++; }
        pthread_mutex_unlock(&qlock);
        pf[n].fd = wake_pipe[0]; pf[n].events = POLLIN; pf[n].revents = 0;
        int r = poll(pf, (nfds_t)(n + 1), 500);                        /* one thread waits on ALL doctor pipes */
        if (r <= 0 || g_shutdown) continue;
        int hup = 0;
        pthread_mutex_lock(&qlock);
        for (int i = 0; i < n; i++) {
            if (!(pf[i].revents & (POLLIN | POLLHUP))) continue;
            Doc *d = &docs[map[i]]; if (d->from_fd != pf[i].fd) continue;
            DocMsg m; ssize_t got = read(d->from_fd, &m, sizeof m);
            if (got == (ssize_t)sizeof m) handle_msg_locked(d, &m); else if (got == 0) hup = 1;
        }
        pthread_mutex_unlock(&qlock);
        if (hup) usleep(20000);                                        /* dead doctor: wait for SIGCHLD reaper */
    }
    return NULL;
}

/* ------------------------------------------------------------ signals: one dedicated thread uses sigwait() */
static void request_shutdown(void)
{
    if (g_shutdown) return;
    g_shutdown = 1;
    char c = 'x'; ssize_t w = write(wake_pipe[1], &c, 1); (void)w;
    RegRequest z; memset(&z, 0, sizeof z); z.client_pid = -1; w = write(reg_fd, &z, sizeof z);
    pthread_mutex_lock(&qlock); pthread_cond_broadcast(&qcond); pthread_mutex_unlock(&qlock);
}

static void reap_children(void)
{
    pthread_mutex_lock(&qlock);
    for (int k = 0; k < ndocs; k++) {
        Doc *d = &docs[k]; int st; pid_t r;
        while (d->pid > 0 && (r = waitpid(d->pid, &st, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {   /* only OUR doctors */
            if (WIFSTOPPED(st)) { d->state = DOC_PAUSED; logmsg("Dr.%d STOPPED by signal %d (%s) - job control", k, WSTOPSIG(st), strsignal(WSTOPSIG(st))); }
            else if (WIFCONTINUED(st)) { d->state = d->cur ? DOC_BUSY : DOC_IDLE; logmsg("Dr.%d CONTINUED (SIGCONT)", k); }
            else {
                if (WIFEXITED(st)) logmsg("Dr.%d (pid %d) exited with status %d", k, d->pid, WEXITSTATUS(st));
                else logmsg("Dr.%d (pid %d) KILLED by signal %d (%s)", k, d->pid, WTERMSIG(st), strsignal(WTERMSIG(st)));
                if (d->to_fd >= 0) close(d->to_fd);
                if (d->from_fd >= 0) close(d->from_fd);
                d->to_fd = d->from_fd = -1; d->pid = 0; d->state = DOC_DOWN;
                if (d->cur) { d->cur->state = 0; d->cur->doc = -1; logmsg("token #%d %s goes back to the queue (doctor crashed)", d->cur->id, d->cur->name); d->cur = NULL; }
                if (!g_shutdown && spawn_doctor(d) == 0) { d->restarts++; logmsg("Dr.%d respawned as pid %d (restart #%d)", k, d->pid, d->restarts); }
                break;
            }
        }
    }
    pthread_cond_broadcast(&qcond);
    pthread_mutex_unlock(&qlock);
}

static void *signal_thread(void *a)
{
    (void)a; sigset_t set; sigemptyset(&set);
    sigaddset(&set, SIGCHLD); sigaddset(&set, SIGINT); sigaddset(&set, SIGTERM); sigaddset(&set, SIGHUP);
    for (;;) {
        int sig; if (sigwait(&set, &sig) != 0) continue;
        if (g_sig_exit) break;
        if (sig == SIGCHLD) reap_children();
        else { logmsg("signal %d (%s) -> graceful shutdown", sig, strsignal(sig)); request_shutdown(); }
    }
    return NULL;
}

/* ------------------------------------------------------------ admin shell (main thread) */
static void cmd_status(void)
{
    pthread_mutex_lock(&qlock); publish_board_locked(); pthread_mutex_unlock(&qlock);
    Board *c = malloc(sizeof *c); if (!c) return;
    rlock(&board->lock); memcpy(c, board, sizeof *c); pthread_mutex_unlock(&board->lock);
    print_board(stdout, c); free(c);
}

static void cmd_estimate(void)
{
    static const char *fn[NFEAT] = { "age>=60 (senior)", "age<12 (child)", "per extra complaint", "brought old reports" };
    pthread_rwlock_rdlock(&est_lock);
    printf("\nLearned consultation-time model (minutes)  [tau(n+1)=a*t(n)+(1-a)*tau(n) + linear correction]\n");
    printf("  %-12s %-12s %-12s\n", "dept", "new visit", "follow-up");
    for (int d = 0; d < NDEPT; d++)
        printf("  %-12s %5.1f +-%-4.1f(%d) %5.1f +-%-4.1f(%d)   [receptionist's flat guess: %.0f]\n", DEPT_NAME[d], est.mean[d][0], sqrt(est.var[d][0]), est.n[d][0], est.mean[d][1], sqrt(est.var[d][1]), est.n[d][1], PRIOR[d]);
    for (int i = 0; i < NFEAT; i++) printf("  feature weight  %-22s %+5.2f min\n", fn[i], est.w[i]);
    for (int k = 0; k < ndocs; k++) printf("  Dr.%d speed factor x%.2f (%d consultations observed)\n", k, est.speed[k], est.dn[k]);
    if (est.cnt) printf("  live accuracy over %ld consultations: learned MAE %.2f min  vs  naive MAE %.2f min  (%.0f%% better)\n", est.cnt, est.sae_learn / est.cnt, est.sae_naive / est.cnt, 100.0 * (1 - est.sae_learn / est.sae_naive));
    else printf("  no live consultations finished yet - run `demo 20`\n");
    pthread_rwlock_unlock(&est_lock);
}

static int tokenize(char *line, char **argv, int max)
{
    int n = 0; char *sp = NULL;
    for (char *t = strtok_r(line, " \t\r\n", &sp); t && n < max - 1; t = strtok_r(NULL, " \t\r\n", &sp)) argv[n++] = t;
    argv[n] = NULL; return n;
}

static void cmd_add(int argc, char **argv, int emergency)
{
    RegRequest rq; memset(&rq, 0, sizeof rq); RegReply rep;
    if (emergency ? argc < 4 : argc < 8) {
        printf(emergency ? "usage: emergency NAME AGE DEPT\n" : "usage: add NAME AGE DEPT(0-5) new|fu COMPLAINTS(1-5) REPORTS(0|1) E|U|N\n"); return; }
    rq.source = SRC_TRIAGE;                      /* this shell is the authorised reception/triage desk */
    strncpy(rq.name, argv[1], sizeof rq.name - 1); rq.age = (uint8_t)atoi(argv[2]); rq.dept = (uint8_t)atoi(argv[3]);
    if (emergency) { rq.complaints = 2; rq.triage_priority = PRIO_EMERGENCY; }
    else { rq.visit = argv[4][0] == 'f'; rq.complaints = (uint8_t)atoi(argv[5]); rq.has_reports = (uint8_t)atoi(argv[6]);
           rq.triage_priority = argv[7][0] == 'E' ? PRIO_EMERGENCY : argv[7][0] == 'U' ? PRIO_URGENT : PRIO_NORMAL; }
    if (admit(&rq, &rep) < 0) printf("refused: %s\n", rep.msg);
}

static void cmd_demo(int n)
{
    static const char *names[] = { "Asha","Ravi","Meena","Kiran","Sara","Imran","Latha","Vijay","Nisha","Arjun","Divya","Suresh","Priya","Mohan","Anil","Farah","Gita","Hari","Ira","Jay" };
    if (n < 1) n = 10;
    if (n > 60) n = 60;
    for (int i = 0; i < n; i++) {
        RegRequest rq; memset(&rq, 0, sizeof rq); RegReply rep;
        rq.source = SRC_TRIAGE;                  /* demo = simulated triage decisions (random) */
        snprintf(rq.name, sizeof rq.name, "%s%d", names[rand() % 20], i);
        rq.dept = (uint8_t)(rand() % NDEPT); rq.visit = (rand() % 100) < 35; rq.age = (uint8_t)(1 + rand() % 85);
        rq.complaints = (uint8_t)(1 + (rand() % 100 < 60 ? 0 : rand() % 3)); rq.has_reports = (rand() % 100) < 30;
        int r = rand() % 100; rq.triage_priority = r < 5 ? PRIO_EMERGENCY : r < 20 ? PRIO_URGENT : PRIO_NORMAL;
        admit(&rq, &rep); usleep(120000);
    }
}

static void print_help(void)
{
    printf("Reception commands:\n"
           "  status                    live board (doctors, queue, ETAs)      estimate   learned model + accuracy\n"
           "  add NAME AGE DEPT new|fu COMPL REPORTS E|U|N     (desk shell: E|U|N is the TRIAGE priority you assign)\n"
           "  emergency NAME AGE DEPT   demo [N]  random patients with simulated triage   (use ./qtriage from another terminal)\n"
           "  break | resume            SIGSTOP / SIGCONT the whole doctor process GROUP (lunch break, job control)\n"
           "  kill N                    SIGKILL Dr.N -> watch SIGCHLD reaping + respawn + patient re-queued\n"
           "  depts                     list departments        !cmd              same as `sh cmd`     quit\n");
    lab_help();
}

static int run_command(char *line)
{
    char *argv[24]; char tmp[1100]; char *src = line;
    while (*src == ' ') src++;
    if (*src == '!') { snprintf(tmp, sizeof tmp, "sh %s", src + 1); src = tmp; }   /* !cmd == sh cmd */
    int argc = tokenize(src, argv, 24);
    if (argc == 0) return 0;
    const char *c = argv[0];
    if (!strcmp(c, "quit") || !strcmp(c, "exit")) return 1;
    else if (!strcmp(c, "help")) print_help();
    else if (!strcmp(c, "status")) cmd_status();
    else if (!strcmp(c, "estimate")) cmd_estimate();
    else if (!strcmp(c, "add")) cmd_add(argc, argv, 0);
    else if (!strcmp(c, "emergency")) cmd_add(argc, argv, 1);
    else if (!strcmp(c, "demo")) cmd_demo(argc > 1 ? atoi(argv[1]) : 10);
    else if (!strcmp(c, "depts")) { for (int i = 0; i < NDEPT; i++) printf("  %d %s\n", i, DEPT_NAME[i]); }
    else if (!strcmp(c, "break") || !strcmp(c, "resume")) {
        if (doc_pgid > 0 && doc_pgid != getpgrp()) { killpg(doc_pgid, c[0] == 'b' ? SIGSTOP : SIGCONT); printf("killpg(%d, %s) sent to doctor process group\n", doc_pgid, c[0] == 'b' ? "SIGSTOP" : "SIGCONT"); }
    }
    else if (!strcmp(c, "kill")) {
        int k = argc > 1 ? atoi(argv[1]) : -1;
        pthread_mutex_lock(&qlock); pid_t pid = (k >= 0 && k < ndocs) ? docs[k].pid : 0; pthread_mutex_unlock(&qlock);
        if (pid > 0) kill(pid, SIGKILL); else printf("usage: kill N (doctor number)\n");
    }
    else if (!strcmp(c, "ps")) {
        pid_t pids[MAX_DOCTORS]; pthread_mutex_lock(&qlock); for (int k = 0; k < ndocs; k++) pids[k] = docs[k].pid; pthread_mutex_unlock(&qlock);
        lab_ps(pids, ndocs);
        printf("  (ps view)\n"); fflush(stdout);
        char *ps_argv[] = { "sh", "ps", "-o", "pid,ppid,pgid,sid,stat,nlwp,comm", "-C", "qserver,qdoctor", NULL };
        lab_dispatch(6, ps_argv);
    }
    else if (!lab_dispatch(argc, argv)) printf("unknown command '%s' (try help)\n", c);
    fflush(stdout);
    return 0;
}

static void admin_loop(void)
{
    char buf[1024]; size_t len = 0; int in_fd = STDIN_FILENO;
    printf("Type 'help'. Prompt is below; log lines appear asynchronously from the logger thread.\nqueueless> "); fflush(stdout);
    while (!g_shutdown) {
        struct pollfd pf[2] = { { in_fd, POLLIN, 0 }, { wake_pipe[0], POLLIN, 0 } };
        int r = poll(pf, 2, -1);
        if (r < 0) { if (errno == EINTR) continue; break; }
        if (pf[1].revents & POLLIN) break;
        if (pf[0].revents & (POLLIN | POLLHUP)) {
            ssize_t n = read(in_fd, buf + len, sizeof buf - len - 1);
            if (n <= 0) { in_fd = -1; logmsg("stdin closed - running headless; stop with SIGTERM/SIGINT"); continue; }
            len += (size_t)n; buf[len] = 0; char *nl;
            while ((nl = memchr(buf, '\n', len))) {
                *nl = 0; char line[1024]; strncpy(line, buf, sizeof line - 1); line[sizeof line - 1] = 0;
                size_t used = (size_t)(nl - buf) + 1; memmove(buf, buf + used, len - used); len -= used;
                if (run_command(line)) return;
                printf("queueless> "); fflush(stdout);
            }
            if (len >= sizeof buf - 1) len = 0;
        }
    }
}

/* ------------------------------------------------------------ main */
static void usage(void)
{
    fprintf(stderr, "usage: ./qserver [-n doctors(1-8)] [-s ms_per_simulated_minute] [-w waiting_room_chairs] [-q] [-D path/to/qdoctor]\n");
    exit(2);
}

int main(int argc, char **argv)
{
    int opt; srand48((long)time(NULL)); srand((unsigned)time(NULL));
    { char self[400]; ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
      if (n > 0) { self[n] = 0; char *sl = strrchr(self, '/'); if (sl) *sl = 0; snprintf(doctor_path, sizeof doctor_path, "%s/qdoctor", self); } else strcpy(doctor_path, "./qdoctor"); }
    while ((opt = getopt(argc, argv, "n:s:w:qD:h")) != -1) {
        switch (opt) {
        case 'n': ndocs = atoi(optarg); break;
        case 's': scale_ms = atof(optarg); break;
        case 'w': wroom_cap = atoi(optarg); break;
        case 'q': g_quiet = 1; break;
        case 'D': strncpy(doctor_path, optarg, sizeof doctor_path - 1); break;
        default: usage();
        }
    }
    if (ndocs < 1 || ndocs > MAX_DOCTORS || scale_ms < 20 || wroom_cap < 1) usage();

    /* --- files: data dir, log (buffered FILE*), shared board (mmap) --- */
    mkdir(DATA_DIR, 0755);
    logfp = fopen(LOG_PATH, "a"); if (!logfp) { perror(LOG_PATH); return 1; }
    int bfd = open(BOARD_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (bfd < 0 || ftruncate(bfd, sizeof(Board)) < 0) { perror(BOARD_PATH); return 1; }
    board = mmap(NULL, sizeof(Board), PROT_READ | PROT_WRITE, MAP_SHARED, bfd, 0);
    if (board == MAP_FAILED) { perror("mmap board"); return 1; }
    close(bfd); memset(board, 0, sizeof *board); board->magic = BOARD_MAGIC;
    pthread_mutexattr_t ma; pthread_mutexattr_init(&ma);
    pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED); pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&board->lock, &ma); pthread_mutex_init(&board->records_lock, &ma); pthread_mutexattr_destroy(&ma);
    sem_init(&board->printers, 1, 2);                          /* pshared=1: usable by the doctor processes */
    sem_init(&waiting_room, 0, (unsigned)wroom_cap); sem_init(&log_empty, 0, LOG_SLOTS); sem_init(&log_full, 0, 0);
    if (pipe2(wake_pipe, O_CLOEXEC) < 0) { perror("pipe"); return 1; }

    /* --- signals: block in every thread; one thread sigwait()s --- */
    sigset_t set; sigemptyset(&set); sigaddset(&set, SIGCHLD); sigaddset(&set, SIGINT); sigaddset(&set, SIGTERM); sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL); signal(SIGPIPE, SIG_IGN);

    /* --- named pipe for registrations (opened O_RDWR so it never sees EOF) --- */
    unlink(REG_FIFO);
    if (mkfifo(REG_FIFO, 0666) < 0) { perror("mkfifo"); return 1; }
    chmod(REG_FIFO, 0666);
    reg_fd = open(REG_FIFO, O_RDWR | O_CLOEXEC); if (reg_fd < 0) { perror("open fifo"); return 1; }

    est_init();
    pthread_create(&th_log, NULL, logger_thread, NULL);
    logmsg("QueueLess reception up: pid=%d pgid=%d sid=%d, %d doctors, 1 simulated minute = %.0f ms", getpid(), getpgrp(), getsid(0), ndocs, scale_ms);
    est_load_history();

    for (int k = 0; k < ndocs; k++) { docs[k].id = k; docs[k].to_fd = docs[k].from_fd = -1; docs[k].state = DOC_DOWN;
        if (spawn_doctor(&docs[k]) < 0) { perror("spawn_doctor"); return 1; } }
    pthread_create(&th_sig, NULL, signal_thread, NULL);
    pthread_create(&th_reg, NULL, reg_thread, NULL);
    pthread_create(&th_coll, NULL, collector_thread, NULL);
    pthread_create(&th_sched, NULL, sched_thread, NULL);

    admin_loop();

    /* --- graceful shutdown --- */
    request_shutdown();
    pthread_mutex_lock(&qlock);
    for (int k = 0; k < ndocs; k++) if (docs[k].to_fd >= 0) { Assign q; memset(&q, 0, sizeof q); q.cmd = CMD_QUIT; write_full(docs[k].to_fd, &q, sizeof q); }
    pthread_mutex_unlock(&qlock);
    pthread_join(th_sched, NULL); pthread_join(th_coll, NULL); pthread_join(th_reg, NULL);
    for (int i = 0; i < 60; i++) {                              /* wait up to 3 s for doctors to exit (reaped by signal thread) */
        int alive = 0; pthread_mutex_lock(&qlock);
        for (int k = 0; k < ndocs; k++) if (docs[k].pid > 0) alive++;
        pthread_mutex_unlock(&qlock);
        if (!alive) break;
        if (i == 30) { if (doc_pgid > 0) killpg(doc_pgid, SIGCONT); }
        usleep(50000);
    }
    pthread_mutex_lock(&qlock);
    for (int k = 0; k < ndocs; k++) if (docs[k].pid > 0) { kill(docs[k].pid, SIGKILL); waitpid(docs[k].pid, NULL, 0); }
    int left = 0; for (Patient *p = qhead; p;) { Patient *n = p->next; free(p); p = n; left++; }
    pthread_mutex_unlock(&qlock);
    logmsg("reception closed (%ld served, %d still waiting were dropped)", st_served, left);
    g_sig_exit = 1; pthread_kill(th_sig, SIGTERM); pthread_join(th_sig, NULL);
    log_put("\x04"); pthread_join(th_log, NULL);
    unlink(REG_FIFO); unlink(BOARD_PATH); munmap(board, sizeof(Board));
    return 0;
}
