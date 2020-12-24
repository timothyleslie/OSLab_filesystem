#include "disk.h"
#include "filesys.h"

#define MAXLINE 100
#define MAXARG 100
char whitespace[] = " \t\r\n\v";

int getcmd(char *cmd, int nbuf);
void parsecmd(char *cmd, char* argv[], int* argc);
void runcmd(char* argv[], int argc);
void execpipe(char* argv[], int argc);

void main()
{
    if(open_disk()!=0)
    {
        printf("fail to open the disk\n");
        return;
    }
    filesys_init();
    char cmd[MAXLINE];
    while(getcmd(cmd, MAXLINE) >= 0)
    {
        char *argv[MAXARG];
        int argc;
        parsecmd(cmd, argv, &argc);
        runcmd(argv, argc);
    }
    return;
}

int getcmd(char* cmd, int nbuf) //从缓冲区中读取命令
{
    printf("=>");
    memset(cmd, 0, nbuf);
    gets(cmd, nbuf);
    if(cmd[0] == 0)
        return -1;
    return 0;
}

void parsecmd(char* cmd, char* argv[], int* argc) //将命令中的变量存入到argv[]中，变量数存入argc
{
  int i,j;
  i=0;
  // 遍历寻找所有变量
  for(j=0; cmd[j]!='\n'&& cmd[j]!='\0'; j++){
    while(strchr(whitespace, cmd[j])) j++; //将j指向变量的开头
    argv[i++] = cmd+j;
    while(!strchr(whitespace, cmd[j])) j++; //将j指向当前变量的末尾
    cmd[j] = '\0';
  }

  argv[i] = 0;
  *argc = i;
}

void runcmd(char* argv[], int argc) //运行命令
{
    int i;
    if(!strcmp(argv[0], "ls"))
    {
        if(argc==1)
        {
            char root = '/';
            ls(&root);
        }
        else
        {
            ls(argv[1]);
        }
    }

    if(!strcmp(argv[0], "mkdir"))
    {
        if(argc==1)
        {
            printf("no enough arguments'\n");
            return;
        }
        mkdir(argv[1]);
    }

    if(!strcmp(argv[0], "touch"))
    {
        if(argc <= 1)
        {
            printf("no enough arguments'\n");
            return;
        }
        touch(argv[1]);
    }

    if(!strcmp(argv[0], "cp"))
    {
        if(argc <= 2)
        {
            printf("no enough arguments'\n");
            return;
        }
        copy(argv[1], argv[2]);
    }

    if(!strcmp(argv[0], "shutdown"))
    {
        shutdown();
    }
}