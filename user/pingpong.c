#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){
    int p1[2],p2[2]; //p1:father->child,p2:child->father
    pipe(p1);
    pipe(p2);
    char ping[] = "a";
    char pong[] = "";
    if(fork()==0){
        close(p1[1]);
        read(p1[0],ping,1);
        printf("%d: received ping\n", getpid());
        close(p2[0]);
        write(p2[1],pong,1);
    }else{
        close(p1[0]);
        write(p1[1],ping,1);
        close(p2[1]);
        read(p2[0],pong,1);
        printf("%d: received pong\n", getpid());
    }
    exit(0); //确保进程退出
}