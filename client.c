#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define NICKNAME_SIZE 50
#define BUF_SIZE 303
#define INPUT_SIZE 250

//buf = flag + nickname + ': ' + input
//buf[0]: flag(1)
//buf[1]~buf[300]:nickname: input(303)
//flag: 1: user input
//      2: server bulletin

char send_buf[BUF_SIZE];
char recv_buf[BUF_SIZE];

char input[INPUT_SIZE];
char nickname[NICKNAME_SIZE];
int nickname_length = 0;

char bulletin[100];

//message queue
char messages[15][250];
int front = 0, rear = 0, count = 0;
int is_empty();
int is_full();
void push_back(char *message);
char *pop_front();
int print();//return how many messages have benn printed

void error_handing(char *message);
//read & write
void read_routine(int sock);
void write_routine(int sock);

//socket
int sock;
int connect_server(char *ip, char *port);

//output screen
void output_welcome();
void output_chat();
void clear();
void GetWinSize(int *w, int *h);
int w, h;

int main(int argc, char *argv[]){
    pid_t pid;
    argc = 3;
    argv[1] = "127.0.0.1";
    argv[2] = "6666";
    if(argc != 3){
        printf("Usage : %s <IP> <port> \n", argv[0]);
        exit(1);
    }
    int sock = connect_server(argv[1], argv[2]);
    GetWinSize(&w,&h);
    output_welcome();
    pid = fork();
    if(pid == 0){
        write_routine(sock);
    }
    else{
        read_routine(sock);
    }
    close(sock);
    return 0;
}

void output_welcome(){
    clear();
    int i = 0, j = 0;
    while(1){
        for(i = 0; i < h; i ++)
            printf("-");
        printf("\n");
        for(i = 0; i < w - 3; i ++){
            printf("|");
            for(j = 1; j < h - 1; j ++)
                printf(" ");
            printf("|\n");
        }
        for(i = 0; i < h; i ++)
            printf("-");
        for(i = 0; i < h / 4; i ++)
            printf(" ");
        printf("\033[%dA", (w / 2));
        printf("Your Nickname : ");

        char temp;
        while ((temp = getchar()) != EOF && temp != '\n'){
            nickname[nickname_length++] = temp;
        }

        if(nickname_length > 20){
            printf("Nickname too long!\n");
            nickname_length = 0;
        }else{

            break;
        }
    }
    output_chat();
}

void output_chat(){
    clear();
    printf("Server Message: %s \n", bulletin); // for server message
    int i = 0, j = 0;
    for(i = 0; i < h; i ++)
        printf("-");
    printf("\n");
    int n = print();
    for(i = 0; i < (4 * w) / 5 - 3 - n - 1; i ++){
        printf("|");
        for(j = 1; j < h - 1; j ++)
            printf(" ");
        printf("|\n");
    }
    for(i = 0; i < h; i ++)
        printf("-");
    printf("\n");
    for(i = 0; i < w / 5 ; i ++){
        printf("|");
        for(j = 1; j < h - 1; j ++)
            printf(" ");
        printf("|\n");
    }
}

void clear(){
//    printf("\033[2J");
    printf("\033c");
}

void read_routine(int sock){
    while(1){
        int t = read(sock, recv_buf, BUF_SIZE);
        if(t == 0){
            printf("The Server shutdown!\n");
            return;
        }else{
            if(recv_buf[0] == '1'){
                push_back(recv_buf);
                output_chat();
            }else if(recv_buf[0] == '2'){
                int i = 0;
                for(i = 0; i < 100; i ++){
                    bulletin[i] = 0;
                }
                strcpy(bulletin, recv_buf);
                bulletin[0] = ' ';
                output_chat();
            }
        }
    }
}

void write_routine(int sock){

    while(1){

        int i = 0;
        for(i = 0; i < h; i ++)
            printf("-");
        printf("\033[%dA", (w / 5));
        printf("\033[%dD", (h - 2));
        int c = 0;
        char t = ' ';

        while ((t = getchar()) != '\n' && t != EOF){
            input[c ++] = t;
        }
        if(c > 250){
            printf("Input too long!\n");
            continue;
        }
        for(i = 0; i < 303; i ++)
            send_buf[i] = 0;
        send_buf[0] = '1';

        for (i = 0; i < nickname_length; i ++){
            send_buf[i + 1] = nickname[i];
        }
        send_buf[i + 1] = ':';
        send_buf[i + 2] = ' ';
        int temp = i + 3;
        for(i = 0; i < c; i ++){
            send_buf[temp + i] = input[i];
        }
        write(sock, send_buf, BUF_SIZE);
        output_chat();
    }
}

void error_handing(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int connect_server(char *ip, char *port){
    int sock;
    struct sockaddr_in serv_adr;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(ip);
    serv_adr.sin_port = htons(atoi(port));
    if(connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1){
        error_handing("connect() error!");
    }
    return sock;
}

//message queue

int is_empty() {
    if (front == rear)
        return 1;
    else
        return 0;
}

int is_full() {
    if ((rear + 1) % 15 == front)
        return 1;
    else
        return 0;
}

void push_back(char *message){
    if (is_full())
        pop_front();
    strcpy(messages[rear], message);
    rear = (rear + 1) % 15;
    count += 1;
}

char *pop_front(){
    if (is_empty())
     {
        return NULL;
     }
     else
     {
         front = (front + 1) % 15;
         count -= 1;
         return messages[front];
     }
}

int print(){
    int i=front;
    int r = 0;
    int j = 0;
    char temp[303];
    int len = 0;
    while(i%15!=rear)
    {
        for(j = 0; j < 303 && messages[i % 15][j+1] != '\n'; j ++)
            temp[j] = messages[i % 15][j + 1];
        len = strlen(temp);
        printf("| %s", temp);
        for(j = 0; j < h - len - 3; j ++){
            printf(" ");
        }
        printf("|\n");
        r ++;
        i++;
    }
    return r;
}

void GetWinSize(int *w, int *h)
{
    struct winsize size;
    ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&size);
    *w = size.ws_row, *h = size.ws_col;
}

