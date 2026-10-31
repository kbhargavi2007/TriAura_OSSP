/* oslab.c - live, runnable demonstrations of the syllabus topics.
 * Every function here is triggered from the reception's admin shell.
 *
 *  CO-1  sys, sh        CO-2  ps, zombie
 *  CO-4  mem, demand, cow, vtop, memerr
 *  CO-5  fs, fds, iobench
 *  CO-6  race, deadlock
 */
#include "queueless.h"
#include "oslab.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdatomic.h>
#include <malloc.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/utsname.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/sysmacros.h>

static int         g_init_data = 42;          /* .data */
static int         g_uninit_bss;              /* .bss  */
static const char  g_rodata[] = "read-only";  /* .rodata */

static double now_s(void) { return mono_sec(); }

/* async-signal-safe-ish printing for forked children of a multithreaded process */
static void cw(const char *fmt, ...)
{
    char b[512]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    if (n > 0) { ssize_t r = write(1, b, (size_t)n); (void)r; }
}

static void unblock_all(void)
{
    sigset_t e; sigemptyset(&e); sigprocmask(SIG_SETMASK, &e, NULL);
    signal(SIGPIPE, SIG_DFL);
}

static void describe_status(pid_t pid, int st)
{
    if (WIFEXITED(st)) printf("  parent: waitpid(%d) -> exited normally, status=%d\n", pid, WEXITSTATUS(st));
    else if (WIFSIGNALED(st))
        printf("  parent: waitpid(%d) -> KILLED BY SIGNAL %d (%s)%s\n", pid, WTERMSIG(st),
               strsignal(WTERMSIG(st)), WCOREDUMP(st) ? " [core dumped]" : "");
}

/* ------------------------------------------------------------------ CO-1 */
static void lab_sys(void)
{
    struct utsname u; uname(&u);
    printf("\n[CO-1] OS as a layered service abstraction\n");
    printf("  user program -> libc wrapper -> SYSCALL trap -> kernel (VFS, scheduler, mm) -> hardware\n");
    printf("  kernel : %s %s %s\n", u.sysname, u.release, u.machine);
    printf("  pid=%d ppid=%d pgid=%d sid=%d uid=%d  page=%ld B  cpus=%ld\n", getpid(), getppid(),
           getpgrp(), getsid(0), getuid(), sysconf(_SC_PAGESIZE), sysconf(_SC_NPROCESSORS_ONLN));
    printf("  getpid()=%d  syscall(SYS_getpid)=%ld   <- same kernel service, raw entry\n", getpid(),
           (long)syscall(SYS_getpid));

    const int N = 200000; struct timespec ts; double t0, t1, t2;
    t0 = now_s();
    for (int i = 0; i < N; i++) clock_gettime(CLOCK_MONOTONIC, &ts);           /* vDSO: stays in user space */
    t1 = now_s();
    for (int i = 0; i < N; i++) syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts); /* real kernel entry */
    t2 = now_s();
    printf("  clock_gettime via vDSO (no mode switch): %6.0f ns/call\n", (t1 - t0) / N * 1e9);
    printf("  clock_gettime via real syscall (trap)  : %6.0f ns/call  <- cost of crossing user->kernel\n", (t2 - t1) / N * 1e9);
    printf("  Command execution journey for `ls | wc -l`:\n"
           "   shell reads line -> parses -> pipe() -> fork() x2 -> child: dup2(), execve() -> kernel loads ELF,\n"
           "   dynamic loader maps libc -> program runs, write() to pipe -> exit() -> parent waitpid() -> prompt\n"
           "  Try:  sh ls | wc -l      and in another terminal:  strace -f -p $(pgrep qserver)\n");
}

static char shell_cwd[PATH_MAX];                     /* the mini-shell's own working directory (per-shell state) */

static void child_exec(char **av, int in_fd, int out_fd)
{
    unblock_all();
    if (shell_cwd[0] && chdir(shell_cwd) < 0) { dprintf(2, "sh: cannot enter %s\n", shell_cwd); _exit(126); }
    if (in_fd >= 0)  dup2(in_fd, 0);
    if (out_fd >= 0) dup2(out_fd, 1);
    execvp(av[0], av);
    dprintf(2, "sh: %s: %s\n", av[0], strerror(errno));
    _exit(127);
}

static void lab_shell(int argc, char **argv)
{
    if (argc < 2) { printf("usage: sh <cmd> [args] [| cmd2 [args]] [> file]   (builtin: cd)\n"); return; }
    char **cmd = &argv[1]; int n = argc - 1;
    if (!strcmp(cmd[0], "cd")) {                       /* builtin: a child cannot change its parent's state */
        char want[2 * PATH_MAX], real[PATH_MAX]; const char *t = n > 1 ? cmd[1] : getenv("HOME");
        if (!t) t = "/";
        if (t[0] == '/' || !shell_cwd[0]) snprintf(want, sizeof want, "%s", t);
        else snprintf(want, sizeof want, "%s/%s", shell_cwd, t);
        struct stat cs;
        if (!realpath(want, real) || stat(real, &cs) < 0 || !S_ISDIR(cs.st_mode)) { printf("  cd: %s: not a directory\n", t); return; }
        snprintf(shell_cwd, sizeof shell_cwd, "%s", real);
        printf("  cd is a BUILT-IN: it must change the shell process's own state (now in %s).\n"
               "  (A child process running `cd` would only change ITS directory and then exit.)\n", shell_cwd);
        return;
    }
    const char *outfile = NULL;
    for (int i = 0; i < n; i++) if (!strcmp(cmd[i], ">") && i + 1 < n) { outfile = cmd[i + 1]; cmd[i] = NULL; n = i; break; }
    int bar = -1;
    for (int i = 0; i < n; i++) if (!strcmp(cmd[i], "|")) { bar = i; break; }
    int outfd = -1;
    if (outfile && (outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644)) < 0) { perror(outfile); return; }
    fflush(stdout);
    int st; pid_t p1, p2 = -1;
    if (bar < 0) {
        p1 = fork(); if (p1 == 0) child_exec(cmd, -1, outfd);
        printf("  [shell] fork() -> child %d ; child calls execvp(\"%s\") ; parent waitpid()\n", p1, cmd[0]);
        fflush(stdout);
        waitpid(p1, &st, 0); describe_status(p1, st);
    } else {
        cmd[bar] = NULL; char **c2 = &cmd[bar + 1]; int p[2];
        if (pipe2(p, O_CLOEXEC) < 0) { perror("pipe"); return; }
        p1 = fork(); if (p1 == 0) child_exec(cmd, -1, p[1]);
        p2 = fork(); if (p2 == 0) child_exec(c2, p[0], outfd);
        close(p[0]); close(p[1]);                      /* pitfall: forgetting this => reader never sees EOF */
        printf("  [shell] pipe(); fork() %d (writer) + fork() %d (reader); both exec; parent closes its pipe ends\n", p1, p2);
        fflush(stdout);
        waitpid(p1, &st, 0); describe_status(p1, st);
        waitpid(p2, &st, 0); describe_status(p2, st);
    }
    if (outfd >= 0) close(outfd);
}

/* ------------------------------------------------------------------ CO-2 */
static int read_stat(pid_t pid, char *state, int *ppid, int *pgrp, int *sid)
{
    char path[64], buf[512]; snprintf(path, sizeof path, "/proc/%d/stat", pid);
    int fd = open(path, O_RDONLY); if (fd < 0) return -1;
    ssize_t n = read(fd, buf, sizeof buf - 1); close(fd); if (n <= 0) return -1;
    buf[n] = 0; char *rp = strrchr(buf, ')'); if (!rp) return -1;
    return sscanf(rp + 2, "%c %d %d %d", state, ppid, pgrp, sid) == 4 ? 0 : -1;
}

static const char *state_name(char c)
{
    switch (c) { case 'R': return "Running/Ready"; case 'S': return "Sleeping (waiting, interruptible)";
                 case 'D': return "Uninterruptible I/O wait"; case 'T': return "Stopped (job control)";
                 case 'Z': return "ZOMBIE (exited, not yet waited)"; case 'I': return "Idle kernel thread";
                 default: return "?"; }
}

static long status_kb(pid_t pid, const char *key)
{
    char path[64], line[256]; snprintf(path, sizeof path, "/proc/%d/status", pid);
    FILE *f = fopen(path, "r"); if (!f) return -1; long v = -1; size_t kl = strlen(key);
    while (fgets(line, sizeof line, f)) if (!strncmp(line, key, kl)) { v = atol(line + kl); break; }
    fclose(f); return v;
}

void lab_ps(const pid_t *pids, int n)
{
    printf("\n[CO-2] Process states read from the /proc pseudo-filesystem (VFS)\n");
    printf("  %-8s %-7s %-7s %-7s %-4s %-9s %s\n", "PID", "PPID", "PGID", "SID", "ST", "RSS(kB)", "meaning");
    pid_t list[MAX_DOCTORS + 1]; int c = 0; list[c++] = getpid();
    for (int i = 0; i < n && c < MAX_DOCTORS + 1; i++) if (pids[i] > 0) list[c++] = pids[i];
    for (int i = 0; i < c; i++) {
        char st; int pp, pg, sd;
        if (read_stat(list[i], &st, &pp, &pg, &sd) < 0) { printf("  %-8d (gone)\n", list[i]); continue; }
        printf("  %-8d %-7d %-7d %-7d %-4c %-9ld %s%s\n", list[i], pp, pg, sd, st, status_kb(list[i], "VmRSS:"),
               state_name(st), i == 0 ? "  <- reception" : "  <- doctor");
    }
    printf("  Doctors share one PROCESS GROUP (killpg => `break`/`resume`); reception is in the shell's group.\n");
    printf("  Session id = the terminal login session; job control (Ctrl-Z / fg / bg) works on process groups.\n");
}

static void lab_zombie(void)
{
    printf("\n[CO-2] Zombie demo (classic process-management pitfall)\n");
    fflush(stdout);
    pid_t c = fork();
    if (c == 0) _exit(0);
    usleep(200000);
    char st; int a, b, d;
    if (read_stat(c, &st, &a, &b, &d) == 0) printf("  child %d has exited but parent has not waited -> state '%c' (%s)\n", c, st, state_name(st));
    waitpid(c, NULL, 0);
    printf("  after waitpid(): %s\n", read_stat(c, &st, &a, &b, &d) < 0 ? "process entry removed from the process table" : "still there?!");
    printf("  QueueLess avoids zombies: SIGCHLD is caught by a signal thread (sigwait) which waitpid()s every doctor.\n");
}

/* ------------------------------------------------------------------ CO-4 */
static long rss_kb(void)
{
    long size, res; FILE *f = fopen("/proc/self/statm", "r"); if (!f) return -1;
    if (fscanf(f, "%ld %ld", &size, &res) != 2) res = -1;
    fclose(f);
    return res * (sysconf(_SC_PAGESIZE) / 1024);
}
static long minflt(void) { struct rusage r; getrusage(RUSAGE_SELF, &r); return r.ru_minflt; }
static long majflt(void) { struct rusage r; getrusage(RUSAGE_SELF, &r); return r.ru_majflt; }
static long pss_kb(void)
{
    FILE *f = fopen("/proc/self/smaps_rollup", "r"); if (!f) return -1;
    char line[128]; long v = -1;
    while (fgets(line, sizeof line, f)) if (!strncmp(line, "Pss:", 4)) { v = atol(line + 4); break; }
    fclose(f); return v;
}

static void lab_mem(void)
{
    int local = 1; void *hs = malloc(100), *hb = malloc(1 << 20);
    void *mm = mmap(NULL, 1 << 20, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("\n[CO-4] Linux process address space (virtual addresses; ASLR changes them every run)\n");
    printf("  text  (code)          %p   lab_mem()\n", (void *)lab_mem);
    printf("  rodata                %p   \"%s\"\n", (const void *)g_rodata, g_rodata);
    printf("  data  (initialised)   %p   g_init_data\n", (void *)&g_init_data);
    printf("  bss   (zero-init)     %p   g_uninit_bss\n", (void *)&g_uninit_bss);
    printf("  heap  malloc(100)     %p   (brk/arena)\n", hs);
    printf("  malloc(1 MiB)         %p   (>= 128 KiB threshold => glibc uses mmap)\n", hb);
    printf("  mmap  anonymous       %p\n", mm);
    printf("  libc  printf()        %p   (shared library mapping)\n", (void *)printf);
    printf("  stack local variable  %p   (grows downwards)\n", (void *)&local);
    printf("  --- /proc/self/maps (selected lines) ---\n");
    FILE *f = fopen("/proc/self/maps", "r"); char line[512]; int shown = 0;
    while (f && fgets(line, sizeof line, f) && shown < 14)
        if (strstr(line, "qserver") || strstr(line, "[heap]") || strstr(line, "[stack]") || strstr(line, "[vdso]") || strstr(line, "libc.so")) { printf("  %s", line); shown++; }
    if (f) fclose(f);
    printf("  --- /proc/self/status ---\n");
    const char *keys[] = { "VmSize:", "VmRSS:", "VmData:", "VmStk:", "VmExe:", "VmLib:" };
    for (int i = 0; i < 6; i++) printf("  %-8s %ld kB\n", keys[i], status_kb(getpid(), keys[i]));
    printf("  page faults so far: minor=%ld major=%ld\n", minflt(), majflt());
#if defined(__GLIBC__) && (__GLIBC__ > 2 || __GLIBC_MINOR__ >= 33)
    struct mallinfo2 mi = mallinfo2();
    printf("  malloc arena: %zu B from brk/arena, %zu B via mmap, in-use %zu B\n", mi.arena, mi.hblkhd, mi.uordblks);
#endif
    printf("  Debug memory bugs with:  make asan   |   valgrind --leak-check=full ./qserver -n 1\n");
    free(hs); free(hb); munmap(mm, 1 << 20);
}

static void lab_demand(void)
{
    size_t len = 64UL << 20, psz = (size_t)sysconf(_SC_PAGESIZE);
    printf("\n[CO-4] Demand paging: virtual memory is a promise, physical pages arrive on first touch\n");
    char *m = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (m == MAP_FAILED) { perror("mmap"); return; }
    madvise(m, len, MADV_NOHUGEPAGE);
    long r0 = rss_kb(), f0 = minflt();
    printf("  mmap(64 MiB) done          : RSS = %ld kB (no physical pages yet)\n", r0);
    for (size_t i = 0; i < len; i += psz) m[i] = 1;             /* each first write = a page fault */
    long r1 = rss_kb(), f1 = minflt();
    printf("  touched %zu pages          : RSS = %ld kB (+%ld kB), minor page faults +%ld\n", len / psz, r1, r1 - r0, f1 - f0);
    madvise(m, len, MADV_DONTNEED);
    printf("  madvise(DONTNEED)          : RSS = %ld kB (kernel reclaimed the frames)\n", rss_kb());
    munmap(m, len);
    printf("  Each fault: CPU traps -> kernel finds VMA -> allocates+zeroes a frame -> updates page table -> resumes.\n");
}

static void lab_cow(void)
{
    size_t len = 32UL << 20, psz = (size_t)sysconf(_SC_PAGESIZE);
    printf("\n[CO-4] Copy-on-Write: fork() copies page TABLES, not memory\n");
    char *buf = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); return; }
    madvise(buf, len, MADV_NOHUGEPAGE); memset(buf, 1, len);
    printf("  parent touched 32 MiB, RSS=%ld kB, Pss=%ld kB\n", rss_kb(), pss_kb());
    fflush(stdout);
    pid_t c = fork();
    if (c == 0) {
        long a = minflt(); volatile long s = 0;
        for (size_t i = 0; i < len; i += psz) s += buf[i];      /* read: pages stay shared */
        long b = minflt(), p1 = pss_kb();
        for (size_t i = 0; i < len; i += psz) buf[i] = 2;       /* write: kernel copies each page */
        long d = minflt(), p2 = pss_kb();
        cw("  child after fork: read pass  -> +%ld faults, Pss=%ld kB (still shared)\n", b - a, p1);
        cw("  child write pass -> +%ld faults (one copy per page), Pss=%ld kB (now private)\n", d - b, p2);
        _exit(0);
    }
    waitpid(c, NULL, 0); munmap(buf, len);
    printf("  That is why fork()+exec() is cheap and why doctors are created quickly.\n");
}

static void lab_vtop(void)
{
    printf("\n[CO-4] Address translation: virtual page -> physical frame (via /proc/self/pagemap)\n");
    size_t psz = (size_t)sysconf(_SC_PAGESIZE);
    volatile char *p = malloc(psz * 4); p[0] = 1;
    uintptr_t va = (uintptr_t)p; uint64_t vpn = va / psz, off = va % psz, e = 0;
    int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0 || pread(fd, &e, 8, (off_t)(vpn * 8)) != 8) { perror("pagemap"); if (fd >= 0) close(fd); free((void *)p); return; }
    close(fd);
    printf("  virtual addr %#lx = VPN %#lx (page number) + offset %#lx\n", (unsigned long)va, (unsigned long)vpn, (unsigned long)off);
    printf("  page-table entry: present=%d swapped=%d file/shared=%d\n", (int)(e >> 63) & 1, (int)(e >> 62) & 1, (int)(e >> 61) & 1);
    uint64_t pfn = e & ((1ULL << 55) - 1);
    if (pfn) printf("  physical frame (PFN) %#lx -> physical addr %#lx\n", (unsigned long)pfn, (unsigned long)(pfn * psz + off));
    else printf("  PFN hidden by the kernel for non-root users (security). Run qserver with sudo to see it.\n");
    free((void *)p);
}

static void segv_handler(int sig, siginfo_t *si, void *u)
{
    (void)sig; (void)u;
    cw("  [child] kernel delivered SIGSEGV: fault address=%p, reason=%s\n", si->si_addr,
       si->si_code == SEGV_MAPERR ? "SEGV_MAPERR (address not mapped)" : "SEGV_ACCERR (permission violation)");
    _exit(42);
}

static void memerr_child(int which)
{
    struct rlimit z = { 0, 0 }; setrlimit(RLIMIT_CORE, &z);
    unblock_all();
    struct sigaction sa; memset(&sa, 0, sizeof sa); sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segv_handler; sa.sa_flags = SA_SIGINFO;
    if (which == 0) { volatile int *volatile p = NULL; *p = 1; }
    else if (which == 1) { sigaction(SIGSEGV, &sa, NULL); volatile int *volatile p = (int *)0x10; *p = 1; }
    else if (which == 2) {
        sigaction(SIGSEGV, &sa, NULL);
        char *g = mmap(NULL, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);   /* guard page */
        g[0] = 1;
    } else {
        long vm = status_kb(getpid(), "VmSize:") * 1024;
        struct rlimit rl = { (rlim_t)(vm + (64L << 20)), (rlim_t)(vm + (64L << 20)) };
        setrlimit(RLIMIT_AS, &rl);
        void *m = malloc(1L << 30);
        cw("  [child] RLIMIT_AS=vm+64MiB: malloc(1 GiB) -> %s (errno=%d: %s)\n", m ? "ok?!" : "NULL", errno, strerror(errno));
        _exit(m ? 0 : 3);
    }
    _exit(0);
}

static void lab_memerr(void)
{
    const char *title[] = { "NULL dereference (no handler)", "invalid address 0x10 with SIGSEGV handler",
                            "write to PROT_NONE guard page", "allocation beyond RLIMIT_AS" };
    printf("\n[CO-4] Memory errors and how the OS responds (each test runs in a disposable child)\n");
    for (int i = 0; i < 4; i++) {
        printf("- case %d: %s\n", i + 1, title[i]); fflush(stdout);
        pid_t c = fork();
        if (c == 0) memerr_child(i);
        int st; waitpid(c, &st, 0);
        if (WIFEXITED(st)) printf("  parent: child exited with status %d\n", WEXITSTATUS(st)); else describe_status(c, st);
    }
    printf("  The MMU raises a page fault; the kernel finds no valid VMA / wrong permission => SIGSEGV.\n"
           "  Tools: valgrind, AddressSanitizer (make asan), gdb, /proc/PID/maps, dmesg | tail\n");
}

/* ------------------------------------------------------------------ CO-5 */
static const char *fstype(unsigned long t)
{
    switch (t) { case 0xEF53: return "ext2/ext3/ext4"; case 0x01021994: return "tmpfs"; case 0x9fa0: return "proc";
                 case 0x62656572: return "sysfs"; case 0x58465342: return "xfs"; case 0x9123683E: return "btrfs";
                 case 0x794c7630: return "overlayfs"; case 0x65735546: return "fuse"; case 0x6969: return "nfs";
                 case 0x1cd1: return "devpts"; default: return "other"; }
}

static void lab_fs(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : RECORDS_PATH;
    printf("\n[CO-5] Files: inodes, directory entries, VFS, ext4\n");
    struct stat st;
    if (stat(path, &st) < 0) { printf("  %s: %s (serve a few patients first: `demo 5`)\n", path, strerror(errno)); return; }
    printf("  stat(\"%s\"): inode=%lu links=%lu size=%ld B blocks(512B)=%ld blksize=%ld mode=%o dev=%lu:%lu\n", path,
           (unsigned long)st.st_ino, (unsigned long)st.st_nlink, (long)st.st_size, (long)st.st_blocks, (long)st.st_blksize,
           st.st_mode & 07777, (unsigned long)major(st.st_dev), (unsigned long)minor(st.st_dev));
    const char *mounts[] = { DATA_DIR, "/tmp", "/proc", "/" };
    for (int i = 0; i < 4; i++) {
        struct statfs sf;
        if (statfs(mounts[i], &sf) == 0)
            printf("  statfs(%-6s): type=0x%lx (%s) block=%ld total=%lu free=%lu inodes=%lu free_inodes=%lu\n", mounts[i],
                   (unsigned long)sf.f_type, fstype((unsigned long)sf.f_type), (long)sf.f_bsize, (unsigned long)sf.f_blocks,
                   (unsigned long)sf.f_bfree, (unsigned long)sf.f_files, (unsigned long)sf.f_ffree);
    }
    printf("  VFS: open/read/write are the same calls for ext4, tmpfs and /proc; the kernel dispatches by filesystem.\n");
    int vfd = open("/proc/version", O_RDONLY); char vb[120] = "";
    if (vfd >= 0) { ssize_t n = read(vfd, vb, sizeof vb - 1); if (n > 0) vb[n] = 0; close(vfd); char *nl = strchr(vb, '\n'); if (nl) *nl = 0; }
    printf("  read(/proc/version) = \"%.90s\"  (no disk involved)\n", vb);

    printf("  directory entries of %s/ (name -> inode):\n", DATA_DIR);
    DIR *d = opendir(DATA_DIR); struct dirent *de;
    while (d && (de = readdir(d))) printf("    %-22s ino=%-9lu type=%d\n", de->d_name, (unsigned long)de->d_ino, de->d_type);
    if (d) closedir(d);

    const char *hl = DATA_DIR "/hardlink.tmp", *sl = DATA_DIR "/symlink.tmp";
    unlink(hl); unlink(sl);
    if (link(path, hl) == 0 && symlink(path, sl) == 0) {
        struct stat a, b, c; stat(path, &a); stat(hl, &b); lstat(sl, &c);
        printf("  hard link: same inode? %s (ino %lu = %lu, link count now %lu)\n", a.st_ino == b.st_ino ? "YES" : "no",
               (unsigned long)a.st_ino, (unsigned long)b.st_ino, (unsigned long)b.st_nlink);
        printf("  symlink : own inode %lu, it just stores the path text (size %ld)\n", (unsigned long)c.st_ino, (long)c.st_size);
    }
    unlink(hl); unlink(sl);

    int fd = open(RECORDS_PATH, O_RDONLY);
    if (fd >= 0) {
        struct stat rs; fstat(fd, &rs); long nrec = rs.st_size / (long)sizeof(Record);
        printf("  history file: %ld consultation records (%zu B each)\n", nrec, sizeof(Record));
        if (nrec > 0) {
            Record r;
            if (pread(fd, &r, sizeof r, (nrec - 1) * (off_t)sizeof r) != (ssize_t)sizeof r) memset(&r, 0, sizeof r);   /* random access, no lseek */
            printf("  pread() last record: #%d %s est=%.1f actual=%.1f min\n", r.patient_id, r.name, r.est_min, r.actual_min);
            Record *m = mmap(NULL, rs.st_size, PROT_READ, MAP_SHARED, fd, 0);       /* memory-mapped file I/O */
            if (m != MAP_FAILED) {
                double sum = 0; for (long i = 0; i < nrec; i++) sum += m[i].actual_min;
                long np = (rs.st_size + 4095) / 4096; unsigned char *vec = malloc((size_t)np);
                int res = 0; if (vec && mincore(m, rs.st_size, vec) == 0) for (long i = 0; i < np; i++) res += vec[i] & 1;
                printf("  mmap()+pointer walk: mean consult = %.2f min; %d/%ld pages already in the page cache (mincore)\n", sum / (double)nrec, res, np);
                free(vec); munmap(m, rs.st_size);
            }
        }
        close(fd);
    }
    printf("  ext4 facts: inode table + block groups + extents + journal (jbd2). Inspect: `sh stat -f .`  `sudo dumpe2fs -h /dev/sdXN`\n");
}

static void lab_fds(void)
{
    printf("\n[CO-5] File descriptors: per-process table -> open file description -> inode\n");
    DIR *d = opendir("/proc/self/fd"); struct dirent *de;
    while (d && (de = readdir(d))) {
        if (de->d_name[0] == '.') continue;
        char p[300], t[256]; snprintf(p, sizeof p, "/proc/self/fd/%s", de->d_name);
        ssize_t n = readlink(p, t, sizeof t - 1); if (n < 0) continue; t[n] = 0;
        printf("  fd %-3s -> %s\n", de->d_name, t);
    }
    if (d) closedir(d);
    const char *tmp = DATA_DIR "/dup.tmp";
    int a = open(tmp, O_RDWR | O_CREAT | O_TRUNC, 0644), b = dup(a), c = open(tmp, O_RDWR);
    if (a >= 0 && b >= 0 && c >= 0) {
        ssize_t w = write(a, "0123456789", 10); (void)w;
        printf("  after write(a,10 bytes): offset via a=%ld, via dup(a)=%ld (SHARED description), via 2nd open()=%ld (separate)\n",
               (long)lseek(a, 0, SEEK_CUR), (long)lseek(b, 0, SEEK_CUR), (long)lseek(c, 0, SEEK_CUR));
    }
    if (a >= 0) close(a);
    if (b >= 0) close(b);
    if (c >= 0) close(c);
    unlink(tmp);
    printf("  Pitfall: leaked descriptors. We open pipes with O_CLOEXEC so exec'd doctors inherit only their own ends.\n");
}

static void lab_iobench(void)
{
    printf("\n[CO-5] Buffered vs unbuffered I/O (20000 lines x 64 B)\n");
    const int N = 20000; char line[64]; memset(line, 'x', 63); line[63] = '\n';
    const char *path = DATA_DIR "/bench.tmp"; double t0, t;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror(path); return; }
    t0 = now_s();
    for (int i = 0; i < N; i++) { ssize_t w = write(fd, line, 64); (void)w; }
    t = now_s() - t0; close(fd);
    printf("  write() every line (%d syscalls)     : %7.1f ms\n", N, t * 1e3);
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }
    t0 = now_s();
    for (int i = 0; i < N; i++) fwrite(line, 1, 64, f);
    fclose(f); t = now_s() - t0;
    printf("  fwrite() buffered by libc (~%d syscalls): %7.1f ms\n", N * 64 / 4096 + 1, t * 1e3);
    f = fopen(path, "w");
    if (!f) { perror(path); return; }
    t0 = now_s();
    for (int i = 0; i < N; i++) { fwrite(line, 1, 64, f); fflush(f); }
    fclose(f); t = now_s() - t0;
    printf("  fwrite()+fflush() each line          : %7.1f ms\n", t * 1e3);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644); t0 = now_s();
    for (int i = 0; i < N; i++) { ssize_t w = write(fd, line, 64); (void)w; if (i % 1000 == 999) fsync(fd); } t = now_s() - t0; close(fd);
    printf("  write() + fsync every 1000 (durable) : %7.1f ms\n", t * 1e3);
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || ftruncate(fd, (off_t)N * 64) < 0) { perror(path); return; }
    char *m = mmap(NULL, (size_t)N * 64, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) { perror("mmap"); close(fd); return; }
    t0 = now_s();
    for (int i = 0; i < N; i++) memcpy(m + (size_t)i * 64, line, 64);
    msync(m, (size_t)N * 64, MS_SYNC); t = now_s() - t0;
    printf("  mmap() memcpy + one msync()          : %7.1f ms\n", t * 1e3);
    munmap(m, (size_t)N * 64); close(fd); unlink(path);
    printf("  Fewer user->kernel crossings = faster; fsync buys durability at a price. The logger thread uses BOTH styles.\n");
}

/* ------------------------------------------------------------------ CO-6 */
#define RN 1000000
#define RT 4
static volatile long r_plain; static long r_mtx; static atomic_long r_atomic;
static pthread_mutex_t r_m = PTHREAD_MUTEX_INITIALIZER; static pthread_barrier_t r_bar;

static void *race_worker(void *arg)
{
    int mode = (int)(intptr_t)arg; pthread_barrier_wait(&r_bar);
    for (int i = 0; i < RN; i++) {
        if (mode == 0) {                                                        /* load, add, store: NOT atomic */
            long tmp = r_plain;
            if ((i & 511) == 0) sched_yield();     /* widen the window so the bug shows even on 1 CPU */
            r_plain = tmp + 1;
        }
        else if (mode == 1) { pthread_mutex_lock(&r_m); r_mtx++; pthread_mutex_unlock(&r_m); }
        else atomic_fetch_add(&r_atomic, 1);
    }
    return NULL;
}

static void lab_race(void)
{
    printf("\n[CO-6] Race condition: %d threads x %d increments, expected %d\n", RT, RN, RT * RN);
    const char *nm[] = { "no lock (data race)", "pthread_mutex", "atomic fetch_add" };
    for (int mode = 0; mode < 3; mode++) {
        r_plain = 0; r_mtx = 0; atomic_store(&r_atomic, 0);
        pthread_t t[RT]; pthread_barrier_init(&r_bar, NULL, RT); double t0 = now_s();
        for (int i = 0; i < RT; i++) pthread_create(&t[i], NULL, race_worker, (void *)(intptr_t)mode);
        for (int i = 0; i < RT; i++) pthread_join(t[i], NULL);
        double dt = now_s() - t0; pthread_barrier_destroy(&r_bar);
        long v = mode == 0 ? r_plain : mode == 1 ? r_mtx : atomic_load(&r_atomic);
        printf("  %-22s result=%-8ld %s  (%.0f ms)\n", nm[mode], v, v == RT * RN ? "correct" : "LOST UPDATES", dt * 1e3);
    }
    printf("  QueueLess protects its queue with a mutex + condition variable, the estimator with a rwlock.\n");
}

typedef struct { pthread_mutex_t *first, *second; const char *name; int result; } DlArg;
static void *dl_worker(void *p)
{
    DlArg *a = p; pthread_mutex_lock(a->first); usleep(150000);
    struct timespec abs; clock_gettime(CLOCK_REALTIME, &abs); abs.tv_sec += 1;
    if (pthread_mutex_timedlock(a->second, &abs) == ETIMEDOUT) a->result = -1;
    else { a->result = 0; pthread_mutex_unlock(a->second); }
    pthread_mutex_unlock(a->first); return NULL;
}

static void lab_deadlock(void)
{
    static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER, B = PTHREAD_MUTEX_INITIALIZER;
    printf("\n[CO-6] Deadlock: doctor needs the ECG machine (A) and the report printer (B)\n");
    DlArg x = { &A, &B, "doctor-1 (A then B)", 0 }, y = { &B, &A, "doctor-2 (B then A)", 0 };
    pthread_t t1, t2;
    pthread_create(&t1, NULL, dl_worker, &x); pthread_create(&t2, NULL, dl_worker, &y);
    pthread_join(t1, NULL); pthread_join(t2, NULL);
    printf("  opposite lock order: %s -> %s ; %s -> %s\n", x.name, x.result ? "DEADLOCK (timed out)" : "ok", y.name, y.result ? "DEADLOCK (timed out)" : "ok");
    DlArg u = { &A, &B, "doctor-1 (A then B)", 0 }, v = { &A, &B, "doctor-2 (A then B)", 0 };
    pthread_create(&t1, NULL, dl_worker, &u); pthread_create(&t2, NULL, dl_worker, &v);
    pthread_join(t1, NULL); pthread_join(t2, NULL);
    printf("  global lock order  : %s -> %s ; %s -> %s\n", u.name, u.result ? "timed out" : "ok", v.name, v.result ? "timed out" : "ok");
    printf("  Coffman conditions: mutual exclusion, hold&wait, no preemption, circular wait. Break one => no deadlock.\n"
           "  QueueLess breaks circular wait (always records_lock -> printer) and uses timeouts (sem_timedwait).\n");
}

/* ------------------------------------------------------------------ dispatcher */
void lab_help(void)
{
    printf("OS labs:\n"
           "  sys                 CO-1 syscalls, user/kernel cost, command journey\n"
           "  sh <cmd> [| cmd2] [> file]   CO-1/2/3 mini-shell: fork+exec+wait, pipes, redirection\n"
           "  ps                  CO-2 process states, PPID/PGID/SID of reception and doctors\n"
           "  zombie              CO-2 zombie demo\n"
           "  mem | demand | cow | vtop | memerr     CO-4 address space, demand paging, COW, page tables, faults\n"
           "  fs [file] | fds | iobench              CO-5 inode/VFS/ext4, descriptors, buffered vs unbuffered vs mmap\n"
           "  race | deadlock     CO-6 lost updates vs mutex/atomic ; deadlock and lock ordering\n");
}

int lab_dispatch(int argc, char **argv)
{
    if (argc < 1) return 0;
    const char *c = argv[0];
    if (!strcmp(c, "sys")) lab_sys();
    else if (!strcmp(c, "sh")) lab_shell(argc, argv);
    else if (!strcmp(c, "zombie")) lab_zombie();
    else if (!strcmp(c, "mem")) lab_mem();
    else if (!strcmp(c, "demand")) lab_demand();
    else if (!strcmp(c, "cow")) lab_cow();
    else if (!strcmp(c, "vtop")) lab_vtop();
    else if (!strcmp(c, "memerr")) lab_memerr();
    else if (!strcmp(c, "fs")) lab_fs(argc, argv);
    else if (!strcmp(c, "fds")) lab_fds();
    else if (!strcmp(c, "iobench")) lab_iobench();
    else if (!strcmp(c, "race")) lab_race();
    else if (!strcmp(c, "deadlock")) lab_deadlock();
    else return 0;
    fflush(stdout);
    return 1;
}
