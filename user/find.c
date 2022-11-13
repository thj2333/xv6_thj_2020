#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path)
{
  char *p;
  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
}

void find(char* path,char* name)
{
  char buf[512], *p;
  int fd;
  struct dirent de; //目录项
  struct stat st; //文件的统计信息

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(!strcmp(fmtname(path),name))
        printf("%s\n",path);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){ //检查缓存有没有溢出
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){ //read到最后一个目录项的下一次退出
      if(de.inum == 0||!strcmp(de.name,".")||!strcmp(de.name,".."))
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf,name);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{
    find(argv[1],argv[2]);
    exit(0);
}
