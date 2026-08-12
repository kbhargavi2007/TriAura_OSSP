#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
int main(){
     int pid;
     printf("Start of parent\n");
     pid=fork(); //create child process
     if(pid == 0){//body of child process
        printf(" Start of Child \n");
        sleep(15);//child process sleep for 15 sec
        exit(0);//force termination of child without completing child process
        printf("End of Child \n");
    }
    else{//body of parent process
        sleep(10);//parent process sleep for 5sec
        wait(NULL);//parent process waits for child process completion
        printf("End of Parent\n");
    }
    return 0;
}


