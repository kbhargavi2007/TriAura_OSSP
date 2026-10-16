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

    for (int i = 0; i < n; i++) {
        printf("Enter %d element: ", i + 1);
        scanf("%d", p + i);
    }

    printf("\nStored elements:\n");

    for (int i = 0; i < n; i++) {
        printf("%d element is: %d\n", i + 1, *(p + i));
    }

    free(p);

    printf("\nMemory released successfully.\n");

    return 0;
}
