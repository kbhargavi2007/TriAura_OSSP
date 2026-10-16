#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    int *p;

    printf("Enter the size of dynamic memory: ");
    scanf("%d", &n);

    p = (int *)malloc(n * sizeof(int));

    if (p == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("\nEnter %d elements:\n", n);

    for (int i = 0; i < n; i++) {
        printf("%d element: ", i + 1);
        scanf("%d", p + i);
    }

    printf("\nElements before reallocation:\n");

    for (int i = 0; i < n; i++) {
        printf("%d element is: %d\n", i + 1, *(p + i));
    }

    p = (int *)realloc(p, (n + 2) * sizeof(int));

    if (p == NULL) {
        printf("Memory reallocation failed.\n");
        return 1;
    }

    p[n] = 5;
    p[n + 1] = 10;

    printf("\nElements after reallocation:\n");

    for (int i = 0; i < n + 2; i++) {
        printf("%d element is: %d\n", i + 1, *(p + i));
    }

    free(p);

    printf("\nMemory released successfully.\n");

    return 0;
}
