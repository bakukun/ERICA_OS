/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * 2023-03-22 , 컴퓨터학부 , 2019004593 , 홍진수 , 60-99번째 line => 표준 입출력 리다이렉션 기능 구현
 * 2023-03-24 , 컴퓨터학부 , 2019004593 , 홍진수 , 100-138번째 line => 파이프 기능 구현
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;               /* 인자의 개수 */
    char *p, *q;                /* 명령어를 파싱하기 위한 변수 */
    int in_fd = STDIN_FILENO;  /* 표준 입력 파일 기본값은 stdin */
    int out_fd = STDOUT_FILENO;  /* 표준 출력 파일 기본값은 stdout */
    int fd[2]; /* 파이프를 만들 때 사용하는 파일 디스크립터를 저장하는 배열 fd[0] = 입력, fd[1] = 출력 */
    pid_t pid; /* 자식 프로세스의 PID를 저장하는 변수 */
    int pipe_val; /* 파이프 생성 결과를 저장하는 변수 파이프 생성이 성공하면 0을, 실패하면 -1을 저장*/


    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    // strspn 이라는 함수는 " \t"라는 문자열이 p에서 일치하지 않는 위치를 리턴
    // 그 위치를 p에 더한다
    do {
        /*
         * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
         * strpbrk() 함수는 원본 문자열에서 구분 문자 또는 문자열에 속해있는 문자 중 가장 먼저 발견된 문자를 찾아 주소를 반환
         */ 
        q = strpbrk(p, " \t\'\"");
        /*
         * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         strsep() 함수는 " " or "\t" 문자열을 만나면, 그 문자열 단위로 문자를 분리한다.
         만약 연속 된 " \t"문자열을 만나면 빈문자열을 리턴한다.
         그리고 q가 빈문자열(첫글자)가 아닌 경우 이를 커맨드라인 인자를 저장하는 argv배열에 저장한 뒤, argc의 인덱스도 증가시킨다.
         */
        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
            if (*q) { // 문자열 변수 q가 NULL이 아닌 경우 아래 조건문을 실행한다.
                if (*q == '<') {  // 문자열 변수 q가 표준 입력 파일 변경 기호인 '<' 인 경우 표준 입력 파일 리다이렉션을 실행한다. 
                    if (*q) {  // 문자열 변수 q가 NULL이 아닌 경우 아래 코드를 실행한다. q는 파일명이 될 것이다.
                        in_fd = open(q, O_RDONLY, 0644); 
                        /*
                        * 파일을 읽기 전용 모드(O_RDONLY)로 열고, 파일 디스크립터를 반환받는다.
                        * 0644 = 이 파일에 대한 소유자가 읽기와 쓰기, 그룹과 기타 사용자는 읽기 권한이 있음을 의미한다.
                        */
                        dup2(in_fd, STDIN_FILENO); // 표준 입력 파일 디스크립터(STDIN_FILENO)를 in_fd로 복제한다.
                        close(in_fd); //in_fd로 전달된 파일 디스크립터는 더 이상 사용하지 않으므로 닫아준다.
                    }   
                    else{
                        fprintf(stderr, "Error: no input file specified\n"); //파일을 열지 못했거나, 파일명이 지정되지 않은 경우에는 오류 메시지를 출력한다.
                        return;
                    }
                }
                else if (*q == '>') {  // 문자열 변수 q가 표준 출력 파일 변경 기호인 '>' 인 경우 표준 출력 파일 리다이렉션을 실행한다.
                    if (*q) { // 문자열 변수 q가 NULL이 아닌 경우 아래 코드를 실행한다. q는 파일명이 될 것이다.
                        q = strsep(&p, " \t");
                        /*
                         * 입력한 명령어와 인자들 중에 > 기호 다음에 오는 문자열(즉, 출력 파일명)을 추출한다.
                         * 이 코드는 >를 추출한 후에 출력 파일명만을 추출할 수 있게 된다. 
                         * > 다음에 바로 출력 파일명이 오는 경우에도 정상적으로 출력 파일명을 추출할 수 있다.
                         * "ls -l >demel" 과 같은 명령어 사용 가능
                        */
                        out_fd = open(q, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        /*
                        * 해당 파일을 쓰기 모드로 열고, 파일 디스크립터를 반환받는다.
                        * O_RDWR: 파일을 쓰기전용 모드로 연다.
                        * O_CREAT: 파일이 없을 경우 새로 만들어준다.
                        * O_TRUNC: 파일이 이미 존재하는 경우, 파일의 내용을 삭제한다. 파일의 크기를 0으로 만든다.
                        */
                        dup2(out_fd, STDOUT_FILENO); // 표준 출력 파일 디스크립터(STDOUT_FILENO)를 out_fd로 복제한다.
                        close(out_fd); //out_fd로 전달된 파일 디스크립터는 더 이상 사용하지 않으므로 닫아준다.
                    }   
                    else {
                        fprintf(stderr, "Error: no output file specified\n"); //파일을 열지 못했거나, 파일명이 지정되지 않은 경우에는 오류 메시지를 출력한다.
                        return;           
                    }
                }
                else if (*q == '|'){  // 문자열 변수 q가 파이프 기호인 '|' 인 경우 아래 코드를 실행한다.
                    if (*q) {
                        pipe_val = pipe(fd);
                        /*
                        * 파이프를 생성한다. 
                        * 성공 시 0, 실패 시 -1을 리턴하는데, 그값을 pipe_val에 대입한다.
                        */
                        if (pipe_val == -1) { // 파이프 생성에 실패한 경우 에러메세지를 출력한다.
                            perror("pipe error");
                            return; 
                        }                        
                        pid = fork(); 
                        /*
                        * 현재 프로세스의 복사본을 생성한다.
                        * 부모 프로세스에서 호출하면 새로운 자식 프로세스를 생성하며, 자식 프로세스에서 호출하면 0을 반환한다.
                        */
                        if (pid > 0) { // 부모 프로세스인 경우 pid가 0보다 크다.
                            dup2(fd[0], STDIN_FILENO);  // 자식 프로세스의 입력을 파이프로부터 받아오도록 STDIN_FILENO (표준 입력 파일 디스크립터) 를 fd[0] (파이프의 읽기용 파일 디스크립터) 로 복제한다.
                            close(fd[1]); // 쓰기용 파일 디스크립터를 닫아준다. 
                            wait(0); // 자식 프로세스가 끝나기를 기다린다.
                            cmdexec(p); // 자식 프로세스의 실행 결과를 받아오기 위해 cmdexec 함수를 재귀적으로 호출한다. 
                            return; //재귀적으로 호출한 cmdexec 실행이 끝나면, 부모 프로세스를 종료한다.
                        }
                        else if (pid == 0){ // 자식 프로세스인 경우 pid가 0이다.
                            dup2(fd[1], STDOUT_FILENO); //자식 프로세스의 출력을 파이프에 쓰도록 STDOUT_FILENO (표준 출력 파일 디스크립터) 를 fd[1] (파이프의 쓰기용 파일 디스크립터) 로 복제한다.
                            close(fd[0]); // 읽기용 파일 디스크립터를 닫아준다.
                            p = NULL; // 문자열을 Null로 초기화 해준다.
                        }
                        else { // fork() 함수 호출에 실패한 경우 에러 메시지를 출력한다.
                            perror("fork");
                            return;
                        }
                    }
                }
                else{ //위 3개의 기호가 아닌 경우 문자열 q를 커맨드라인 인자를 저장하는 argv배열에 저장한 뒤, argc의 인덱스도 증가시킨다.
                    argv[argc++] = q; 
                }
            }
        }
        /*
         * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
        }
        /*
         * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         * 따음표로 감싸진 문자열을 찾기위함 / (") 자체로는 빈 문자열로 리턴되기 때문이다.
         */
        else {
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
        }        
    } while (p);
    argv[argc] = NULL;
    /*
     * argv에 저장된 명령어를 실행한다.
     */

    if (argc > 0)
        execvp(argv[0], argv);    
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
        /*위의 커맨드를 주석차리하고 다시 make및 커맨드를 쓰고나서, 좀비로 남아있으면 (defun)으로 뜸*/
        /*빈 라카룸에 이름만 넣어있는 느낌 exit하면 그래도 parents로 올라가서 지워지긴함 */
        
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}
