#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main() {
	int pid, num;
	printf("enter num\n");
	scanf("%d",&num);
	pid = fork(); //create child process
	if(pid<0) { // process creation failure condition
	printf("fail to create new process\n");
	exit(0);
	}
	else if(pid == 0){ //child process execution // calulation factorial
	unsigned int fact = 1;
	int temp = num;
	while(temp>0) {
	fact = fact + temp;
	temp --;
	}
	printf("factorial of %d is %d",num,fact);
	}
	else { // parent process execution 
// calculating square
	int sqr;
	wait(NULL);
	sqr = num + num;
	printf("Square of %d is %d\n",num,sqr);
	}
	return 0;
	}

	
