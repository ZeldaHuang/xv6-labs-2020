#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void func(int *p){
    int num_prime;
    close(p[1]);
    if(read(p[0],&num_prime,sizeof(num_prime))!=0)
    {
        int pp[2];
        pipe(pp);
        fprintf(1,"prime %d\n",num_prime);
        if(fork()==0){
            func(pp);
        }
        else{
            int num;
            while(1){
                close(p[1]);
                if(read(p[0],&num,sizeof(num))==0){
                    break;
                }
                if(num%num_prime!=0){
                    write(pp[1],&num,sizeof(num));
                }
            }
            close(pp[1]);
        }
        wait(0);
        exit(0);
    }
}
int main(){
    int i;
    int p[2];
    pipe(p);
    if(fork()==0){
        func(p);
    }
    else{
        close(p[0]);
        for(i=2;i<=35;++i){
            int tmp=i;
            write(p[1],&tmp,sizeof(tmp));
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}
