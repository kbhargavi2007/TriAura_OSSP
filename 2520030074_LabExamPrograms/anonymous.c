#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int fd[2];
    char msg[] = "Speed: 60 km/h, GPS: 16.5";
    char buffer[100];

    if (pipe(fd) == -1)
        return 1;

    if (fork() == 0)
    {
        close(fd[0]);

        write(fd[1], msg, sizeof(msg));

        close(fd[1]);
    }
    else
    {
        close(fd[1]);

        read(fd[0], buffer, sizeof(buffer));

        printf("Received: %s\n", buffer);
        printf("Telemetry received successfully.\n");

        close(fd[0]);

        wait(NULL);
    }

    return 0;
}
