#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>


#define BUF_SIZE 303
#define EPOLL_SIZE 100
#define MAX_USER 100
void error_handing(char *message);
//void send_bulletin();
//void listen_connect(char *port);

void *listen_connect(void *port);
void *send_bulletin(void *args);
void send_to_everyone(char *buf);


char recv_buf[BUF_SIZE];
char send_buf[BUF_SIZE];// 303

char bulletin[302];

//user queue
int user_list[MAX_USER];
int front = 0, rear = 0, count = 0;
int is_empty();
int is_full();
void push_back(int user);
int pop_front();
void print_all_user(){
    int i=front;
    printf("----user list start------\n");

    while(i%100!=rear)
    {
        printf("%d\n", user_list[i%100]);
        i++;
    }
    printf("----user list end--------\n");
}



struct PORT{
    char port[5];
};

int main(int argc, char *argv[]){
    argc = 2;
    argv[1] = "6666";
    if (argc != 2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    pthread_t t_id1, t_id2;
    struct PORT p;
    strcpy(p.port, argv[1]);
    if(pthread_create(&t_id1, NULL, listen_connect, (void*)&p) != 0){
        puts("pthread_create() error!");
        return -1;
    }
    if(pthread_create(&t_id2, NULL, send_bulletin, NULL)){
        puts("pthread_create() error!");
        return -1;
    }

    void *thr_ret;
    if(pthread_join(t_id1, &thr_ret) != 0){
        puts("pthread_join() error");
        return -1;
    }
    if(pthread_join(t_id1, &thr_ret) != 0){
        puts("pthread_join() error");
        return -1;
    }
//    pid_t pid;
//    pid = fork();
//    if(pid == 0)
//        listen_connect(argv[1]);
//    else
//        send_bulletin();
    printf("Thread return message: %s \n", (char *)thr_ret);
    free(thr_ret);
    return 0;
}

void *send_bulletin(void *args){
    while(1){
        printf("Input Server Message : ");
        int c = 0;
        char t = ' ';
        while ((t = getchar()) != '\n' && t != EOF){
            bulletin[c ++] = t;
        }
        if(strlen(bulletin) > 250){
            printf("Input too long!");
            continue;
        }
        send_buf[0] = '2';
        int i = 0;
        for(i = 0; i < strlen(bulletin); i ++){
            send_buf[i + 1] = bulletin[i];
        }
        send_buf[i + 1] = 0;
        send_to_everyone(send_buf);
    }
}

void error_handing(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

//void listen_connect(char *port){
void *listen_connect(void *port){
    struct PORT *temp = (struct PORT*)port;
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(((char*)temp->port)));

    if(bind(serv_sock, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) == -1){
        error_handing("bind() error!");
    }
    if(listen(serv_sock, 5) == -1){
        error_handing("listen() error!");
    }

    epfd=epoll_create(EPOLL_SIZE);
    ep_events=(struct epoll_event*)malloc(sizeof(struct epoll_event)*EPOLL_SIZE);
    event.events=EPOLLIN;
    event.data.fd=serv_sock;
    epoll_ctl(epfd,EPOLL_CTL_ADD,serv_sock,&event);

    int str_len, i;
    while(1){
        //printf("Waiting for connect...");
        event_cnt=epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if(event_cnt == -1){
            puts("epoll_wait() error");
            break;
        }
        for(i = 0; i < event_cnt; i++){
            if(ep_events[i].data.fd == serv_sock){
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                //printf("connected client: %d \n", clnt_sock);
                push_back(clnt_sock);
            } else {
                str_len = read(ep_events[i].data.fd, recv_buf, BUF_SIZE);
                if(str_len == 0){
                    int j;
                    pop_front();
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    //printf("closed client: %d \n", ep_events[i].data.fd);
                }else{
                    send_to_everyone(recv_buf);
                }
            }
        }
    }
    close(serv_sock);
    close(epfd);
}

void send_to_everyone(char *buf){
    if(strlen(buf) != 0){
        int i=front;
        while(i%100!=rear)
        {
            write(user_list[i % 100], buf, BUF_SIZE);
            i++;
        }
    }
}

int is_empty() {
    if (front == rear)
        return 1;
    else
        return 0;
}

int is_full() {
    if ((rear + 1) % 100 == front)
        return 1;
    else
        return 0;
}


void push_back(int user){
    if (is_full())
        pop_front();
    user_list[rear] = user;
    rear = (rear + 1) % 100;
    count += 1;
}

int pop_front(){
    if (is_empty())
     {
        return NULL;
     }
     else
     {
        int temp = user_list[front];
        front = (front + 1) % 100;
        count -= 1;
        return temp;
     }
}

