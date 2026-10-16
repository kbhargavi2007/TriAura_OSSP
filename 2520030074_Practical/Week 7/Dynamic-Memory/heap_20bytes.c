#include <stdio.h>
#include <stdlib.h>

int main() {
    char *p;

    printf("Allocating 20 bytes of heap memory...\n");

    p = (char *)malloc(20 * sizeof(char));

    if (p == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("20 bytes of heap memory allocated successfully.\n");

    printf("\nEnter a string (maximum 19 characters): ");
    scanf("%19s", p);

    printf("You entered: %s\n", p);

    free(p);

    printf("Heap memory released successfully.\n");

    return 0;
}
