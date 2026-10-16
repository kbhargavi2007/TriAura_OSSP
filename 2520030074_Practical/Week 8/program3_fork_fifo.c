#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

int main() {

    int fd;
    char msg[100];

    mkfifo("A2B", 0666);
    mkfifo("B2A", 0666);

    int pid = fork();

    if (pid < 0) {

        perror("Failed to create child");
        exit(1);
    }

    // CHILD = RECEIVER
    else if (pid == 0) {

        fd = open("A2B", O_RDONLY);

        while (1) {

            int n = read(fd, msg, sizeof(msg) - 1);

            if (n <= 0)
                break;

            msg[n] = '\0';

            if (strcmp(msg, "bye") == 0)
                break;

            printf("\nUser B: %s\n", msg);
            printf("User A: ");
            fflush(stdout);
        }

        close(fd);
    }

    // PARENT = SENDER
    else {

        fd = open("A2B", O_WRONLY);

        while (1) {

            printf("User A: ");
            fflush(stdout);

            fgets(msg, sizeof(msg), stdin);

            msg[strcspn(msg, "\n")] = '\0';

            write(fd, msg, strlen(msg) + 1);

            if (strcmp(msg, "bye") == 0)
                break;
        }

        close(fd);
    }

    return 0;
}
