#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SUBJECTS 5

typedef struct {
    int marks[SUBJECTS];
} StudentMarks;

int main() {
    const char *shm_name = "/student2_marks";

    int shm_fd;
    StudentMarks *student;

    shm_fd = shm_open(shm_name, O_RDONLY, 0666);

    if (shm_fd == -1) {
        perror("shm_open student2");
        exit(EXIT_FAILURE);
    }

    student = mmap(NULL, sizeof(StudentMarks),
                   PROT_READ,
                   MAP_SHARED, shm_fd, 0);

    if (student == MAP_FAILED) {
        perror("mmap student2");
        exit(EXIT_FAILURE);
    }

    int total = 0;

    printf("\n========================================\n");
    printf("              STUDENT 2\n");
    printf("========================================\n");

    for (int i = 0; i < SUBJECTS; i++) {
        printf("Subject %d : %d\n",
               i + 1,
               student->marks[i]);

        total += student->marks[i];
    }

    float average = (float)total / SUBJECTS;

    printf("\nTotal   : %d\n", total);
    printf("Average : %.2f\n", average);

    munmap(student, sizeof(StudentMarks));
    close(shm_fd);

    return 0;
}
