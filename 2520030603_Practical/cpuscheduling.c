#include<stdio.h>
#include<stdlib.h>
int main(){
	int choice,n;
	Process original[10],p[10];
	printf("
int choice;

while(1){
	printf("CPU Scheduling Algorithms\n");
	printf("1. FCFS \n");
	printf("2. SJF \n");
	printf("3. SRTF \n");
	printf("4. Priority non-preemptive \n");
	printf("5. Priority preemptive \n");
	printf("6. Round Robin \n");
	printf("7. Exit\n");
	printf("Select one of the CPU scheduling Algo:\n");
	scanf("%d",&choice);
	switch(choice){
	 case 1:
	   printf("operations of FCFS\n");
	   break;
	case 2:
           printf("operations of SJF\n");
           break;
	case 3:
           printf("operations of SRTF\n");
           break;
	case 4:
           printf("operations of Priority non-preemptive\n");
           break;
	case 5:
           printf("operations of Priority preemptive\n");
           break;
	case 6:
           printf("operations of Round Robin\n");
           break;
	case 7:
	   exit(0);
}
}
}
