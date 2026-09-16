#include<stdio.h>
int main() {
int  *ptr  =  NULL;
	int x = 25;
	ptr = &x;
	printf("x = %d \n",x);
	printf("address of x = %ls \n", &x);
	printf("value of ptr = %ls \n", ptr);
	printf("value at ptr =  %d \n", *ptr);
	printf("address of ptr = %ls \n", &ptr);
	return 0;
}
