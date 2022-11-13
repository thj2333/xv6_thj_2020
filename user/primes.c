#include "kernel/types.h"
#include "user.h"

void f(int* num, int n){
    printf("prime %d\n",*num);
    if(n==1){
        return ;
    }
    int i,p[2];
    pipe(p);
    if(fork()==0){
        close(p[1]);
        int cnt=0;
        while(read(p[0],num+cnt,sizeof(int))){
            cnt++;
        }
        f(num,cnt);
        exit(0);
    }else{
        close(p[0]);
        for(i=1;i<n;i++){
            if(*(num+i)%(*num)!=0){
                write(p[1],num+i,sizeof(int));
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    return;
}

int main(int argc,char* argv[]){
    int i,num[34];
    for(i=0;i<34;i++){
        num[i]=i+2;
    }
    f(num,34);
    exit(0);
}