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
    int sum = 0;
    float average;

    // Generate the same key
    key = ftok(SHM_NAME, 65);

    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Get shared memory
    shmid = shmget(key, NUM_MARKS * sizeof(int), 0666);

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
    printf("          CONSUMER PROCESS\n");
    printf("========================================\n");

    printf("Student marks read from shared memory:\n\n");

    for (int i = 0; i < NUM_MARKS; i++) {
        printf("Subject %d : %d\n", i + 1, marks[i]);
        sum += marks[i];
    }

    average = (float)sum / NUM_MARKS;

    printf("\nTotal Marks : %d\n", sum);
    printf("Average     : %.2f\n", average);

    // Detach shared memory
    if (shmdt(marks) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Remove shared memory
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    printf("\nShared memory removed successfully.\n");

    return 0;
}
