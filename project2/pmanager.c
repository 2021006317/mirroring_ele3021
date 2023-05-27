#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

//* user func : parse token from buf input, and return token.
void
parsemytoken(char* buf, char** tokens, int maxTokens) {
    int tokenCount = 0;
    int i = 0;
    int start = 0;
    
    while (buf[i] != '\0') {
        if ((buf[i] == ' ')) {
            buf[i] = '\0';
            if (tokenCount < maxTokens) {
                tokens[tokenCount] = &buf[start];
                tokenCount++;
            }
            start = i + 1;
        }
        i++;
    }
    if (tokenCount < maxTokens) {
        tokens[tokenCount] = &buf[start];
    }
}

//* user func :parse cmd from buf input, and return with defined costant.
int
parsemycmd(char* buf)
{
    char* tokens[4];
    parsemytoken(buf, tokens, 4);
    char* command = tokens[0];

    if ((command[0] == 'l')&&(command[1]=='i')&&(command[2]=='s')&&(command[3]=='t')){
        return MYLIST;
    }
    else if ((command[0] == 'k')&&(command[1]=='i')&&(command[2]=='l')&&(command[3]=='l')){
        return MYKILL;
    }
    else if ((command[0] == 'e')&&(command[1]=='x')&&(command[2]=='e')&&(command[3]=='c')&&(command[4] == 'u')&&(command[5]=='t')&&(command[6]=='e')){
        return MYEXEC;
    }
    else if ((command[0] == 'm')&&(command[1]=='e')&&(command[2]=='m')&&(command[3]=='l')&&(command[4]=='i')&&(command[5]=='m')){
        return MYMEMLIM;
    }
    else if ((command[0] == 'e')&&(command[1]=='x')&&(command[2]=='i')&&(command[3]=='t')){
        return MYEXIT;
    }
    else if (sizeof(command)==0){
        return NOCMD;
    }
    else {
        return INVALID;
    }
}

//* 에러 걸린 경우 출력하고 종료시킬 때
// 1. 토큰이 필요 이상으로 많은 경우
// 2. 토큰 validate = -1 인 경우
void errorexit(int err, char* name){
    if (err==1){
        printf(1, "error: wrong token with %s\n", name);
        exit();
    }
    else if (err==2){
        printf(1, "error: invalid token with %s\n", name);
        exit();
    }
}

//* user func : validate for path, because it should be composed of alphabets and numbers.
// 1이면 정상, -1이면 오류 
int
validate_path(char *path)
{ 
    int valid = 0;
    for (char* c = path; c<&path[strlen(path)]; c++){
        if ((*c >='a' && *c <='z') || (*c >='A' && *c<='Z') || (*c >= '0' && *c <= '9')){
            valid++;
        }
    }
    if (valid == strlen(path))  return 1;
    else                        return -1;
}

//* user func : validate for pid, limit and stacksize
// 1. is it int?
// 2. is it in its own bound? ( 0 이상 10억 이하 )
// 정상이면 num, 아니면 -1
int
validate_num(char* txt)
{
    for (char* c = txt; c<&txt[strlen(txt)]; c++){
        if (!(*c >= '0' && *c <= '9') && (*c != 10)) return -1;
    }
    return atoi(txt);
}

//* user func : main
int
main(int argc, char* argv[])
{
    while(1) {
        static char buf[100];
        int pid, limit, stacksize, command;
        char* path;

        printf(2, "> ");
        memset(buf, 0, sizeof(buf));
        gets(buf, sizeof(buf));
        
        char* tokens[4]={0,};
        parsemytoken(buf, tokens, 4);
        command = parsemycmd(buf);

        switch(command){
        case NOCMD:
            printf(1, "error : need command\n");
            break;
    
        case MYLIST: // list
            if (tokens[1]!=0)   errorexit(1, "list");

            list();
            break;
    
        case MYKILL: // kill <pid>
            if (tokens[2]!=0)   errorexit(1, "kill");

            pid = validate_num(tokens[1]);
            if (pid==-1)        errorexit(2, "kill");
            
            kill(pid);
            break;
    
        case MYEXEC: // execute <path> <stacksize>
            if (tokens[3]!=0)          errorexit(1, "execute");

            path = tokens[1];
            if (validate_path(path)==-1)   errorexit(2, "exec_path");

            stacksize = validate_num(tokens[2]);
            if (stacksize==-1)         errorexit(2, "exec_stacksize");

            pid = fork();
            if(pid==0) {
                pid = fork();
                char* argv[] = { path, 0 };
                if (pid==0)     exec2(path, argv, stacksize);
                exit();
            }
            else if(pid>0) wait();
            else           printf(1, " execute fork failed\n");
            break;
    
        case MYMEMLIM: // memlim <pid> <limit>
            if (tokens[3]!=0)   errorexit(1, "execute");

            pid = validate_num(tokens[1]);
            if (pid == -1)      errorexit(2, "memlim_pid");

            limit = validate_num(tokens[2]);
            if (limit == -1)    errorexit(2, "memlim_limit");

            setmemorylimit(pid, limit);
            break;
    
        case MYEXIT: // exit
            if (tokens[1]!=0)   errorexit(1, "exit");
            
            exit();
            break;
        default: // unknown
            printf(1, "error: unknown command\n");
            break;
        }
    }
    exit();
}