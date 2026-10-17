
#include <stdio.h>

int main(void) {
    int n, at[100], bt[100], pr[100], done[100]={0};
    int ct[100], wt[100], tat[100], rt[100];
    printf("Number of processes: ");
    if(scanf("%d",&n)!=1 || n<1 || n>100) return 1;
    for(int i=0;i<n;i++){
        printf("AT BT Priority for P%d: ",i+1);
        if(scanf("%d%d%d",&at[i],&bt[i],&pr[i])!=3 || at[i]<0 || bt[i]<=0) return 1;
    }
    int time=0, count=0; double sw=0, st=0, sr=0;
    printf("Execution order: ");
    while(count<n){
        int best=-1;
        for(int i=0;i<n;i++) if(!done[i] && at[i]<=time){
            if(best==-1 || pr[i]<pr[best] ||
              (pr[i]==pr[best] && at[i]<at[best]) ||
              (pr[i]==pr[best] && at[i]==at[best] && i<best)) best=i;
        }
        if(best==-1){ time++; continue; }
        printf("P%d ",best+1);
        rt[best]=time-at[best]; time+=bt[best]; ct[best]=time;
        tat[best]=ct[best]-at[best]; wt[best]=tat[best]-bt[best];
        done[best]=1; count++;
    }
    printf("\nPID\tAT\tBT\tPR\tCT\tTAT\tWT\tRT\n");
    for(int i=0;i<n;i++){
        sw+=wt[i]; st+=tat[i]; sr+=rt[i];
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
          i+1,at[i],bt[i],pr[i],ct[i],tat[i],wt[i],rt[i]);
    }
    printf("Average TAT = %.2f, WT = %.2f, RT = %.2f\n",st/n,sw/n,sr/n);
    return 0;
}


