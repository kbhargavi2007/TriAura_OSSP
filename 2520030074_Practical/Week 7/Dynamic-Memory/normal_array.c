#include <stdio.h>
#include <stdlib.h>

int main() {
    int n, ar[10];

    printf("Enter the size of array: ");
    scanf("%d", &n);

    if (n > 10 || n <= 0) {
        printf("Invalid size. Please enter a value between 1 and 10.\n");
        return 1;
    }

    printf("Enter the elements of array:\n");

    for (int i = 0; i < n; i++) {
        printf("%d element: ", i + 1);
        scanf("%d", &ar[i]);
    }

    printf("\nArray elements:\n");

    for (int i = 0; i < n; i++) {
        printf("%d element is: %d\n", i + 1, ar[i]);
    }

    return 0;
}
