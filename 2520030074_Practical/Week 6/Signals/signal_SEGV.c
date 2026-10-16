#include<stdio.h>
int main(){
	int x, *p = NULL;
	printf("Enter the number");
	scanf("%d", &x);
	p = &x;
	p = (int *)100;
	printf("The entered number is: %d\n", x);
	printf("address of x is: %u\n", &x);
	printf("The entered number is: %d \n", *p);
	printf("value o p: %u\n", p);
	printf("address of p is: %u\n", &p);
	return 0;
}
