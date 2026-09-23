#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>

int main(){
        int fd1, fd2;//pipe ids
        char msg[100];//message length

        mkfifo("fifo1", 0666);//create pipe
        mkfifo("fifo2", 0666);

        while(1){//repeat messages
                printf("user A:  ");
                fgets(msg, 100, stdin);//reading of messages
                msg[strcspn(msg, "\n")] = '\0';//strcspn function for seaching for a charecter inthe given string

                fd1 = open("fifo1", O_WRONLY);//pipe1 opend for write
                write(fd1, msg, strlen(msg) + 1);//write function avilable in fcnt1.h
                close(fd1);// close the file from user A end

                if(strcmp(msg, "bye") == 0)
                   break;

                fd2 = open("fifo2", O_RDONLY);//pipe2 opend for reading
                read(fd2, msg, 100);//read function
                close(fd2);

                printf("User B: %s\n", msg);

                if(strcmp(msg, "bye") == 0)
                  break;
             }
        return 0;
}
