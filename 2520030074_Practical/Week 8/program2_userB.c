#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

int main() {
    int fd1, fd2;
    char msg[100];

    mkfifo("fifo1", 0666);
    mkfifo("fifo2", 0666);

    while (1) {

        fd1 = open("fifo1", O_RDONLY);

        int n = read(fd1, msg, 99);

        if (n <= 0) {
            close(fd1);
            break;
        }

        msg[n] = '\0';

        close(fd1);

        printf("User A: %s\n", msg);

        if (strcmp(msg, "bye") == 0)
            break;

        printf("User B: ");

        fgets(msg, 100, stdin);

        msg[strcspn(msg, "\n")] = '\0';

        fd2 = open("fifo2", O_WRONLY);

        write(fd2, msg, strlen(msg) + 1);

        close(fd2);

        if (strcmp(msg, "bye") == 0)
            break;
    }

    return 0;
}
