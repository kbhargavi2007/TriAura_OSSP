#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_NAME "shmfile"
#define NUM_MARKS 5

int main() {
    key_t key;
    int shmid;
    int *marks;

    // Generate the same key
    key = ftok(SHM_NAME, 65);

    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Create/Get shared memory
    shmid = shmget(key, NUM_MARKS * sizeof(int), IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory
    marks = (int *)shmat(shmid, NULL, 0);

    if (marks == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    printf("========================================\n");
    printf("          PRODUCER PROCESS\n");
    printf("========================================\n");

    printf("Enter marks for 5 subjects:\n");

    for (int i = 0; i < NUM_MARKS; i++) {
        printf("Subject %d: ", i + 1);
        scanf("%d", &marks[i]);
    }

    printf("\nMarks successfully written to shared memory.\n");

    // Detach shared memory
    if (shmdt(marks) == -1) {
        perror("shmdt");
        exit(1);
    }

    printf("Producer detached from shared memory.\n");

    return 0;
}
