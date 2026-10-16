#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    int *p;

    printf("Enter the number of elements: ");
    scanf("%d", &n);

    p = (int *)calloc(n, sizeof(int));

    if (p == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("\nMemory allocated using calloc().\n");
    printf("Initial values:\n");

    for (int i = 0; i < n; i++) {
        printf("Element %d: %d\n", i + 1, p[i]);
    }

    printf("\nEnter %d elements:\n", n);

    for (int i = 0; i < n; i++) {
        printf("Element %d: ", i + 1);
        scanf("%d", &p[i]);
    }

    printf("\nStored elements:\n");

    for (int i = 0; i < n; i++) {
        printf("Element %d is: %d\n", i + 1, p[i]);
    }

    free(p);

    printf("\nMemory released successfully.\n");

    return 0;
}
