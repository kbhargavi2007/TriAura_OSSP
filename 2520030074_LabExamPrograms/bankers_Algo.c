#include <stdio.h>

#define P 5
#define R 3

int a[P][R], m[P][R], n[P][R], av[R];
int finish[P], seq[P], count = 0;

void safe(int k, int w[])
{
    if (k == P)
    {
        count++;

        printf("Sequence %d: ", count);
        for (int i = 0; i < P; i++)
            printf("P%d ", seq[i]);

        printf("\n");
        return;
    }

    for (int i = 0; i < P; i++)
    {
        if (finish[i])
            continue;

        int j;

        for (j = 0; j < R; j++)
        {
            if (n[i][j] > w[j])
                break;
        }

        if (j == R)
        {
            int t[R];

            for (j = 0; j < R; j++)
                t[j] = w[j] + a[i][j];

            finish[i] = 1;
            seq[k] = i;

            safe(k + 1, t);

            finish[i] = 0;
        }
    }
}

int main()
{
    printf("Enter Allocation matrix (5 x 3):\n");

    for (int i = 0; i < P; i++)
        for (int j = 0; j < R; j++)
            scanf("%d", &a[i][j]);

    printf("Enter Maximum matrix (5 x 3):\n");

    for (int i = 0; i < P; i++)
    {
        for (int j = 0; j < R; j++)
        {
            scanf("%d", &m[i][j]);

            if (m[i][j] < a[i][j])
            {
                printf("Invalid input!\n");
                return 0;
            }
        }
    }

    printf("Enter Available resources (3 values):\n");

    for (int j = 0; j < R; j++)
        scanf("%d", &av[j]);

    /* Calculate Need matrix */
    for (int i = 0; i < P; i++)
    {
        for (int j = 0; j < R; j++)
            n[i][j] = m[i][j] - a[i][j];
    }

    printf("\nNeed Matrix:\n");

    for (int i = 0; i < P; i++)
    {
        for (int j = 0; j < R; j++)
            printf("%d ", n[i][j]);

        printf("\n");
    }

    printf("\nAll possible safe sequences:\n");

    safe(0, av);

    if (count > 0)
        printf("\nSystem is SAFE.\nTotal safe sequences = %d\n", count);
    else
        printf("\nSystem is UNSAFE. No safe sequence exists.\n");

    return 0;
}
