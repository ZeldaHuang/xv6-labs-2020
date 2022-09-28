#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(){
    int p1[2],p2[2];
    pipe(p1);
    pipe(p2);
    int pid=fork();
    if(pid==0){
        char msg[4];
        int num=read(p1[0],msg,1);
        if(num==1){
            fprintf(1,"%d: received ping\n",getpid());
            write(p2[1],"p",1);  
        }
        else{
            fprintf(2,"child process read error");
            exit(1);
        }
    }
    else if(pid>0){
        char parent_msg[4];
        write(p1[1],"p",1);
        // wait(0);
        int num=read(p2[0],parent_msg,1);
        if(num==1){
            fprintf(1,"%d: received pong\n",getpid());
        }
        else{
            fprintf(2,"parent process read error");
            exit(1);
        }
    }
    else{
        fprintf(2,"fork error");
    }
    exit(0);
}