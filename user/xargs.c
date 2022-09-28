#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fs.h"
#define STDIN 0
#define MAXLINE 128
int main(int argc, char *argv[]){
    int i,len,str_len;
    char str[MAXLINE];
    char *params[MAXLINE];
    char tmp[MAXLINE];
    len=argc-1;
    memmove(params,argv+1,sizeof(char*)*len);
    while((str_len=read(STDIN,str,sizeof(str)))>0){
       if(fork()==0){
           int idx=0;
           for(i=0;i<str_len;++i){
               if(str[i]==' '||str[i]=='\n'){
                   char *args=(char *)malloc(idx+1);
                   memmove(args,tmp,idx);
                   args[idx]=0;
                   params[len++]=args;
                   idx=0;
               }
               else{
                   tmp[idx++]=str[i];
               }
           }
           params[len]=0;
           exec(argv[1],params);
       }
       else{
           wait(0);
       }
    }
    exit(0);
}