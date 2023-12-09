#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <poll.h>

#define MAX_LEN 1024



typedef struct params
{
    int* fd;
    char pi_num;
}Params;//스레드에 인자전달을 위한 구조체 선언

int fd_pi1[2];
int fd_pi2[2];

void* sendmsg (void *arg){
    //read(fd[0],recvmsg_from_pi4_to_pi2,sizeof(recvmsg_from_pi4_to_pi2));
    //read(fd[0],recvmsg_from_pi4_to_pi1,sizeof(recvmsg_from_pi4_to_pi1));

    char* ip = "XXXXXX";
    int port = 1234;

    struct sockaddr_in serv_addr;

    int server_socket = socket(PF_INET, SOCK_STREAM,0); //소켓 선언

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port); //소켓 할당

    connect(server_socket, (struct sockaddr*) &serv_addr,sizeof(serv_addr)); //connect

    send 
}

int main(int argc,char* argv[]){
    

    pid_t pid;
    pid= fork();

    pipe(fd_pi1);
    pipe(fd_pi2);

 
    if(pid>0){ // 부모프로세스 = pi3로부터 센서값 받아옴 >> server

        char recvmsg_from_pi3[MAX_LEN];
        char sendmsg_for_pi2[MAX_LEN];
        char sendmsg_for_pi1[MAX_LEN];


        int serv_port = 7777;
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(serv_port); 

        int optvalue=1;
        int server_socket = socket(PF_INET,SOCK_STREAM,0);
        if(server_socket == -1) errhandle("socket() ERR!");
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue)); 

        int bindret = bind(server_socket,(struct sockaddr*) &serv_addr, sizeof(serv_addr));
        if(bindret == -1) errhandle("bind() ERR!");

        listen(server_socket,3);

        int client_socket;
        socklen_t client_addr_size;
        struct sockaddr_in client_addr;
        client_addr_size = sizeof(client_addr);

        printf("Server Listening...........\n");

        if((client_socket=accept(server_socket,(struct sockaddr*)&client_addr,&client_addr_size))<0){ 
            close(server_socket);
            close(client_socket);
        }
        else{ 
            printf("Connection Success");
            while(1){
                recv(client_socket,recvmsg_from_pi3,strlen(recvmsg_from_pi3),0); //센서값 받아오기

                //logic

                //logic


                write(fd_pi2[1],sendmsg_for_pi2,sizeof(sendmsg_for_pi2));
                write(fd_pi1[1],sendmsg_for_pi1,sizeof(sendmsg_for_pi1));


            }

        }

        int status;
        wait(&status);

    }else{ // 자식프로세스 = 다른 서버로 명령 보냄 >> client
        char recvmsg_from_pi4_to_pi2[MAX_LEN];
        char recvmsg_from_pi4_to_pi1[MAX_LEN];

        Params* forpi1;
        forpi1 = (Params*)malloc(sizeof(Params));
        Params* forpi2;
        forpi2 = (Params*)malloc(sizeof(Params));

        forpi1->fd = fd_pi1;
        forpi1->pi_num = 1;

        forpi2->fd = fd_pi2;
        forpi2->pi_num = 2;



        pthread_t pi1, pi2;


        pthread_create(&pi1,NULL,sendmsg,(void*)&forpi1);


        pthread_create(&pi2,NULL,sendmsg,(void*)&forpi2);

        pthread_join(pi1);
        ptrhead_join(pi2);
    }


    



    



    return 0;
}
