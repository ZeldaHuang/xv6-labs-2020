#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find_file(char *path,char *target){
    char *p,buf[512];
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        exit(0);
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit(0);
    }
    
    if(st.type!=T_DIR){
        fprintf(2, "find:%s is not directory\n", path);
        close(fd);
        exit(0);
    };

    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      exit(0);
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      if(st.type==2&&strcmp(de.name,target)==0){
          fprintf(1,buf);
          fprintf(1,"\n");
      }
      else if(st.type==1&&strcmp(de.name,".")&&strcmp(de.name,"..")){
          find_file(buf,target);
      }
    //   printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
}
int main(int argc, char *argv[]){
    if(argc!=3){
        fprintf(2,"argument error\n");
    }
    char *path=argv[1];
    char *target=argv[2];
    find_file(path,target);
    exit(0);
}