#include<stdio.h>
#include<fcntl.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>
int main(){
char bfr[100];
printf("receiving message from  writer\n");
int fd = open("/tmp/siri_fifo",O_RDONLY);
read(fd,bfr,strlen(bfr));
close(fd);
printf("received message:%s", bfr);
return 0;
}
