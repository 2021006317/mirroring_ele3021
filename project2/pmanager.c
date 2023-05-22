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
        // 공백 문자를 기준으로 파싱
        if (buf[i] == ' ') {
            buf[i] = '\0';  // 파싱된 토큰의 끝을 표시
            if (tokenCount < maxTokens) {
                tokens[tokenCount] = &buf[start];  // 파싱된 토큰 저장
                tokenCount++;
            }
            start = i + 1;  // 다음 토큰의 시작 위치
        }
        i++;
    }
    if (tokenCount < maxTokens) {
        tokens[tokenCount] = &buf[start];  // 마지막 토큰 저장
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
    else{
        return NOCMD;
    }
}

//* user func : validate for path, becuase it should be composed of alphabets and numbers. 
int
validate_path(char *path)
{ 
    int valid = 0;
    for (char* c = path; c<&path[strlen(path)]; c++){
        if ((*c >='a' && *c <='z') || (*c >='A' && *c<='Z') || (*c >= '0' && *c <= '9')){
            valid++;
        }
    }
    if (valid == strlen(path)){ return 1; }
    else return 0;
}

//* user func : validate for pid and stacksize, because it has own bound : 0 이상 10억 이하
int
validate_num(int n)
{
    if (n<0 || n > 100000000){
        return 0;
    }
    return 1;
}

//* user func : main
int main(int argc, char* argv[]){

    if (!strcmp(argv[0],"pmanager")){
        while(1){
            static char buf[100];
            int pid, limit, stacksize, command;
            char* path;
            
            printf(2, "> ");
            memset(buf, 0, sizeof(buf));
            gets(buf, sizeof(buf));
            char* tokens[4];

            parsemytoken(buf, tokens, 4);
            command = parsemycmd(buf);

            switch(command){
            case NOCMD:
                printf(1, "error : need command\n");
                break;
        // list
            case MYLIST:
                list();
                break;
        // kill <pid>
            case MYKILL:
                pid = atoi(tokens[1]);
                if(validate_num(pid)) {
                    printf(1, "kill\n");
                    printf(1, "%d\n", pid);
                    kill(pid);}
                break;
        // execute <path> <stacksize>
            case MYEXEC:
                path = tokens[1];
                char* argv[] = { path, 0 };
                stacksize = atoi(tokens[2]);
                printf(1, "%s\n", argv[0]);
                printf(1, "%s\n", argv[1]);
                if(validate_num(stacksize) && validate_path(path)) exec2(path, argv, stacksize);
                break;
        // memlim <pid> <limit>
            case MYMEMLIM:
                pid = atoi(tokens[1]);
                limit = atoi(tokens[2]);
                if(validate_num(pid) && validate_num(limit)) setmemorylimit(pid, limit);
                break;
        // exit
            case MYEXIT:
                exit();
                break;
        // unknown
            default:
                printf(1, "error: unknown command");
            }
        }
    }

    else if (!strcmp(argv[0], "my")){
        printf(1, "me\n");
    }

    return 0;
}