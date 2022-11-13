#include "kernel/types.h"
#include "user.h"
#include "kernel/param.h"

int main(int argc,char* argv[]){
    char buf[512];
    char* p = buf;
    int i;
    while(read(0,p,1)){
        if(*p=='\n'){
            char* args[MAXARG];
            for(i=0;i<argc-1;i++){
                args[i] = argv[i+1];
            }
            for(char *j=buf,*k=buf;k<=p;k++)
                if(*k==' '||*k=='\n'){
                    char* a = (char*)malloc(sizeof(buf));
                    memcpy(a,j,k-j);
                    args[i++] = a;
                    j = k+1;
                }
            if(fork()==0){
                exec(args[0],args);
            }else{
                wait(0);
            }
            p=buf;
        }else{
            p++;
        }
    }
    exit(0);
}