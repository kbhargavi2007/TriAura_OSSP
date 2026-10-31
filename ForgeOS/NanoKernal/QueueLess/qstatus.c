/* qstatus.c - waiting-room display screen (separate process)
 * Reads the board through the SAME file mapping the server and doctors use:
 * memory-mapped file I/O + a process-shared mutex. No copying through pipes.
 * usage: ./qstatus [-w]     (-w = refresh every second)
 */
#include "queueless.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char **argv)
{
    int watch = (argc > 1 && !strcmp(argv[1], "-w"));
    int fd = open(BOARD_PATH, O_RDWR);
    if (fd < 0) { fprintf(stderr, "board not found - is ./qserver running?\n"); return 1; }
    Board *b = mmap(NULL, sizeof(Board), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (b == MAP_FAILED) { perror("mmap"); return 1; }
    Board *copy = malloc(sizeof *copy);
    do {
        struct timespec abs; clock_gettime(CLOCK_REALTIME, &abs); abs.tv_sec += 1;
        int r = pthread_mutex_timedlock(&b->lock, &abs);
        if (r == EOWNERDEAD) { pthread_mutex_consistent(&b->lock); r = 0; }
        if (r != 0) { fprintf(stderr, "board busy/locked (%s)\n", strerror(r)); sleep(1); continue; }
        memcpy(copy, b, sizeof *copy);
        pthread_mutex_unlock(&b->lock);
        if (watch) printf("\033[H\033[2J");
        print_board(stdout, copy);
        if (watch) { fflush(stdout); sleep(1); }
    } while (watch);
    free(copy); munmap(b, sizeof(Board)); close(fd);
    return 0;
}
