#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define SUBJECTS 5

typedef struct {
    int marks[SUBJECTS];
} StudentMarks;

int main() {
    const char *shm1_name = "/student1_marks";
    const char *shm2_name = "/student2_marks";

    int shm1_fd, shm2_fd;
    StudentMarks *student1, *student2;

    printf("========================================\n");
    printf("        TEACHER - PRODUCER\n");
    printf("========================================\n");

    /* Create shared memory for Student 1 */
    shm1_fd = shm_open(shm1_name, O_CREAT | O_RDWR, 0666);
    if (shm1_fd == -1) {
        perror("shm_open student1");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm1_fd, sizeof(StudentMarks)) == -1) {
        perror("ftruncate student1");
        exit(EXIT_FAILURE);
    }

    /* Create shared memory for Student 2 */
    shm2_fd = shm_open(shm2_name, O_CREAT | O_RDWR, 0666);
    if (shm2_fd == -1) {
        perror("shm_open student2");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm2_fd, sizeof(StudentMarks)) == -1) {
        perror("ftruncate student2");
        exit(EXIT_FAILURE);
    }

    /* Attach shared memory */
    student1 = mmap(NULL, sizeof(StudentMarks),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, shm1_fd, 0);

    student2 = mmap(NULL, sizeof(StudentMarks),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, shm2_fd, 0);

    if (student1 == MAP_FAILED || student2 == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    /* Enter marks for Student 1 */
    printf("\nEnter marks for Student 1:\n");

    for (int i = 0; i < SUBJECTS; i++) {
        printf("Subject %d: ", i + 1);
        scanf("%d", &student1->marks[i]);
    }

    printf("Student 1 marks written successfully.\n");

    /* Enter marks for Student 2 */
    printf("\nEnter marks for Student 2:\n");

    for (int i = 0; i < SUBJECTS; i++) {
        printf("Subject %d: ", i + 1);
        scanf("%d", &student2->marks[i]);
    }

    printf("Student 2 marks written successfully.\n");

    /*
     * Create Student 1 process
     */
    pid_t pid1 = fork();

    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        execl("./student1", "student1", NULL);
        perror("execl student1");
        exit(EXIT_FAILURE);
    }

    /*
     * Create Student 2 process
     */
    pid_t pid2 = fork();

    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        execl("./student2", "student2", NULL);
        perror("execl student2");
        exit(EXIT_FAILURE);
    }

    printf("\nTeacher: Student processes started.\n");

    /* Wait for both students */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("\nBoth student processes completed.\n");

    /* Detach shared memory */
    munmap(student1, sizeof(StudentMarks));
    munmap(student2, sizeof(StudentMarks));

    close(shm1_fd);
    close(shm2_fd);

    /* Remove shared-memory objects */
    shm_unlink(shm1_name);
    shm_unlink(shm2_name);

    printf("Shared-memory objects cleaned up successfully.\n");

    return 0;
}
