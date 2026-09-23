#include<stdio.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/shm.h>
int main(){
key_t key;
int shmid;
int *marks;
int sum = 0;
float average;
key=ftok(SM_NAME, 65);
if(key == -1){
perror("ftok");
exit(1);
}
shmid= shmget(key, NUM_MARKS * sizeof(int), 0666);
if(shmid == -1){
perror("shmget");
exit(1);
}
marks = (int *)shmat(shmid, NULL, 0);
if(marks == (int *) -1)
{
perror("shmat");
exit(1);
}
printf("Consumer Process\n");
printf("----------------\n");
printf("Student marks read from shared memory:\n");
for(int i=0;i<NUM_MARKS; i++){
printf("%d ", marks[i]);
sum = sum + marks[i];
}
printf("\n");
average = (float)sum/ NUM_MARKS;
printf("Total Marks = %d\n", sum);
printf("Average = %.1f\n", average);
shmdt(marks);
shmctl(shmid, IPC_RMID, NULL);
return 0;
}

