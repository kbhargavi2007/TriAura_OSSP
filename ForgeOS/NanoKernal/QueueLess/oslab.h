#ifndef OSLAB_H
#define OSLAB_H
#include <sys/types.h>

/* Handles the "OS lab" admin commands. argv[argc] must be NULL.
 * Returns 1 if the command was one of the labs, 0 otherwise. */
int  lab_dispatch(int argc, char **argv);
void lab_ps(const pid_t *pids, int n);
void lab_help(void);
#endif
