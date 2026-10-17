
#include <stdio.h>
int main() {
    int n, at[20], bt[20], rt[20], ct[20], tat[20], wt[20], res[20];
    int t = 0, done = 0, p, min, first[20];
    float aw = 0, atat = 0, ar = 0;

    printf("Enter number of processes: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        printf("Enter AT and BT for P%d: ", i + 1);
        scanf("%d%d", &at[i], &bt[i]);
        rt[i] = bt[i];
        first[i] = -1;
    }

    while (done < n) {
        p = -1;
        min = 9999;

        for (int i = 0; i < n; i++)
            if (at[i] <= t && rt[i] > 0 && rt[i] < min)
                min = rt[i], p = i;

        if (p == -1) { t++; continue; }

        if (first[p] == -1) first[p] = t;
        rt[p]--;
        t++;

        if (rt[p] == 0) {
            ct[p] = t;
            done++;
        }
    }

    printf("\nP\tAT\tBT\tCT\tTAT\tWT\tRT\n");
    for (int i = 0; i < n; i++) {
        tat[i] = ct[i] - at[i];
        wt[i] = tat[i] - bt[i];
        res[i] = first[i] - at[i];

        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               i + 1, at[i], bt[i], ct[i], tat[i], wt[i], res[i]);

        aw += wt[i];
        atat += tat[i];
        ar += res[i];
    }

    printf("Average WT = %.2f\n", aw / n);
    printf("Average TAT = %.2f\n", atat / n);
    printf("Average RT = %.2f\n", ar / n);
    return 0;
}
