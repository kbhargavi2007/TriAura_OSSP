#include <stdio.h>
#include <stdlib.h>

int main() {
    int n, new_n;
    int *p;

    printf("Enter the initial size: ");
    scanf("%d", &n);

    p = (int *)malloc(n * sizeof(int));

    if (p == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("\nEnter %d elements:\n", n);

    for (int i = 0; i < n; i++) {
        printf("Element %d: ", i + 1);
        scanf("%d", &p[i]);
    }

    printf("\nEnter the new size: ");
    scanf("%d", &new_n);

    p = (int *)realloc(p, new_n * sizeof(int));

    if (p == NULL) {
        printf("Memory reallocation failed.\n");
        return 1;
    }

    if (new_n > n) {
        printf("\nEnter the additional elements:\n");

        for (int i = n; i < new_n; i++) {
            printf("Element %d: ", i + 1);
            scanf("%d", &p[i]);
        }
    }

    printf("\nFinal array:\n");

    for (int i = 0; i < new_n; i++) {
        printf("Element %d is: %d\n", i + 1, p[i]);
    }

    free(p);

    printf("\nMemory released successfully.\n");

    return 0;
}
