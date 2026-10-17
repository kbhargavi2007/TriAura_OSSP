#include <stdio.h>
#include <stdbool.h>

#define MAXP 10
#define MAXR 10

int n, m;
int alloc[MAXP][MAXR];
int maxm[MAXP][MAXR];
int need[MAXP][MAXR];
int available[MAXR];
int sequence[MAXP];

bool finished[MAXP];

int safe_count = 0;
int dead_count = 0;

void print_sequence(int depth) {
    if (depth == 0) {
        printf("(none)");
        return;
    }

    for (int i = 0; i < depth; i++) {
        printf("P%d", sequence[i]);
        if (i < depth - 1)
            printf(" -> ");
    }
}

void enumerate_safe(int depth, int work[MAXR]) {
    if (depth == n) {
        safe_count++;

        printf("\nSafe sequence %d: ", safe_count);
        print_sequence(depth);
        printf("\n");
        return;
    }

    bool found = false;

    for (int p = 0; p < n; p++) {
        if (finished[p])
            continue;

        bool can_run = true;

        for (int r = 0; r < m; r++) {
            if (need[p][r] > work[r]) {
                can_run = false;
                break;
            }
        }

        if (can_run) {
            found = true;

            int next_work[MAXR];

            for (int r = 0; r < m; r++)
                next_work[r] = work[r] + alloc[p][r];

            finished[p] = true;
            sequence[depth] = p;

            enumerate_safe(depth + 1, next_work);

            finished[p] = false;
        }
    }

    if (!found) {
        dead_count++;

        printf("\nDead-end path %d: ", dead_count);
        print_sequence(depth);

        printf("\nWork resources: ");
        for (int r = 0; r < m; r++)
            printf("%d ", work[r]);

        printf("\nBlocked processes and their Need:\n");

        for (int p = 0; p < n; p++) {
            if (finished[p])
                continue;

            printf("P%d Need: ", p);

            for (int r = 0; r < m; r++)
                printf("%d ", need[p][r]);

            printf("\n");
        }
    }
}

int main(void) {
    printf("BANKER'S ALGORITHM - CLOUD RESOURCE MANAGER\n");

    printf("Enter number of processes and resource types: ");

    if (scanf("%d%d", &n, &m) != 2 ||
        n < 1 || n > MAXP || m < 1 || m > MAXR) {
        printf("Invalid dimensions. Maximum: %d processes, "
               "%d resource types.\n", MAXP, MAXR);
        return 1;
    }

    printf("\nEnter Allocation matrix (%d x %d):\n", n, m);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (scanf("%d", &alloc[i][j]) != 1 ||
                alloc[i][j] < 0) {
                printf("Invalid allocation value.\n");
                return 1;
            }
        }
    }

    printf("\nEnter Maximum Claim matrix (%d x %d):\n", n, m);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (scanf("%d", &maxm[i][j]) != 1 ||
                maxm[i][j] < alloc[i][j]) {
                printf("Invalid maximum claim.\n");
                return 1;
            }

            need[i][j] = maxm[i][j] - alloc[i][j];
        }
    }

    printf("\nEnter Available resource vector (%d values): ", m);

    for (int j = 0; j < m; j++) {
        if (scanf("%d", &available[j]) != 1 ||
            available[j] < 0) {
            printf("Invalid available resource value.\n");
            return 1;
        }
    }

    printf("\nNeed Matrix (Maximum - Allocation):\n");

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++)
            printf("%d ", need[i][j]);

        printf("\n");
    }

    for (int i = 0; i < n; i++)
        finished[i] = false;

    int work[MAXR];

    for (int j = 0; j < m; j++)
        work[j] = available[j];

    printf("\n--- Checking all execution paths ---\n");

    enumerate_safe(0, work);

    printf("\n========== FINAL RESULT ==========\n");

    if (safe_count > 0) {
        printf("System state: SAFE\n");
        printf("Total safe sequences: %d\n", safe_count);
    } else {
        printf("System state: UNSAFE\n");
        printf("No complete safe execution sequence exists.\n");
    }

    printf("Dead-end paths encountered: %d\n", dead_count);

    return 0;
}
