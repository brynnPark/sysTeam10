#include "header.h"



int state=FIRST1;


int fd_pi1[2];
int fd_pi2[2];
Pipe pi1,pi2;

int temp_setter=50;
int light_setter=150;
int humid_setter=250;
int soil_setter=350;
int water_setter=100;

int temp_cur=0;
int light_cur=0;
int humid_cur=0;
int soil_cur=0;
int water_cur=0;

char on_temp[MAX_LEN] = "on_H_off_F";
char off_temp[MAX_LEN] = "off_H_on_F";           
char on_light[MAX_LEN]="on_led";
char off_light[MAX_LEN] = "off_led";
char on_humid[MAX_LEN] = "on_HUMI";
char off_humid[MAX_LEN] = "off_HUMI";
char on_soil[MAX_LEN] = "on_WP";
char off_soil[MAX_LEN] = "off_WP";




void* button_listener(void* arg){
    while(1){
        if(read_button(LEFT)){
            state=do_action(LEFT,state,temp_setter,light_setter, humid_setter, soil_setter,water_setter,temp_cur,light_cur,humid_cur,soil_cur,water_cur);
        }
        else if(read_button(RIGHT)){
            state=do_action(RIGHT,state,temp_setter,light_setter, humid_setter, soil_setter,water_setter,temp_cur,light_cur,humid_cur,soil_cur,water_cur);
        }
        else if(read_button(UP)){
            switch(state){
                case TEMP:
                    temp_setter++;
                    break;
                case LIGHT:
                    light_setter++;
                    break;
                case HUMID:
                    humid_setter++;
                    break;
                case SOIL:
                    soil_setter++;
                    break;
                case WATER:
                    water_setter++;
                    break;
            }
            state=do_action(UP,state,temp_setter,light_setter, humid_setter, soil_setter,water_setter,temp_cur,light_cur,humid_cur,soil_cur,water_cur);
        }
        else if(read_button(DOWN)){
            switch(state){
                case TEMP:
                    temp_setter--;
                    break;
                case LIGHT:
                    light_setter--;
                    break;
                case HUMID:
                    humid_setter--;
                    break;
                case SOIL:
                    soil_setter--;
                    break;
                case WATER:
                    water_setter--;
                    break;
            }
            state=do_action(DOWN,state,temp_setter,light_setter, humid_setter, soil_setter,water_setter,temp_cur,light_cur,humid_cur,soil_cur,water_cur);

        }
        sleep(0.1);
    }
}

void* sendmsg_to_pi2 (void *arg){
    printf("Connecting to Pi2...\n");
    Params* data = (Params *)arg;

    int pinum = data->pi_num;
    Pipe* p2_pipe = data->fd;
 
    char sendmsg_to_pi2[MAX_LEN];
    char recvmsg_from_pipe_2[MAX_LEN];
    ssize_t cnt;

    char* ip = "192.168.0.3"; //Pi2 주소
    int port = 7777; //포트

    struct sockaddr_in serv_addr;

    int server_socket = socket(PF_INET, SOCK_STREAM,0); //소켓 선언

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port); //소켓 할당

    connect(server_socket, (struct sockaddr*) &serv_addr,sizeof(serv_addr)); //connect

    while(read(p2_pipe->fd[0],recvmsg_from_pipe_2,sizeof(recvmsg_from_pipe_2))){
        if(strcmp(recvmsg_from_pipe_2, on_light)==0){
            send(server_socket,on_light,strlen(on_light),0);
        }
        else{
            send(server_socket,off_light,strlen(off_light),0);
        }
        memset(recvmsg_from_pipe_2,0,sizeof(recvmsg_from_pipe_2));

    }
    
    pthread_exit(0);
}
void* sendmsg_to_pi1 (void *arg){
    Params* data = (Params *)arg;

    int pinum = data->pi_num;
    Pipe* p1_pipe = data->fd;
    //int* pipe = data->fd;
    
    printf("Connecting to Pi1...\n");
    char sendmsg_to_pi1[MAX_LEN];
    char recvmsg_from_pipe_1[MAX_LEN];
    char* ip = "192.168.0.3"; //PI1 주소
    int port = 1234; // 포트

    struct sockaddr_in serv_addr;

    int server_socket = socket(PF_INET, SOCK_STREAM,0); //소켓 선언

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port); //소켓 할당

    connect(server_socket, (struct sockaddr*) &serv_addr,sizeof(serv_addr)); //connect
    ssize_t cnt;

    while(1){
        //logic
        memset(recvmsg_from_pipe_1,0,sizeof(recvmsg_from_pipe_1));
        cnt=read(p1_pipe->fd[0],recvmsg_from_pipe_1,sizeof(recvmsg_from_pipe_1));
        printf("%s\n",recvmsg_from_pipe_1);

        if(strcmp(recvmsg_from_pipe_1,on_temp)==0){
            send(server_socket,on_temp,strlen(on_temp),0);
        }
        else{
            send(server_socket,off_temp,strlen(off_temp),0);                
        }
        
        memset(recvmsg_from_pipe_1,0,sizeof(recvmsg_from_pipe_1));
        cnt=read(p1_pipe->fd[0],recvmsg_from_pipe_1,sizeof(recvmsg_from_pipe_1));
        printf("%s\n",recvmsg_from_pipe_1);

        if(strcmp(recvmsg_from_pipe_1,on_humid)==0){
            send(server_socket,on_humid,strlen(on_humid),0);
        }
        else{
            send(server_socket,off_humid,strlen(off_humid),0);                
        }
        
        memset(recvmsg_from_pipe_1,0,sizeof(recvmsg_from_pipe_1));
        cnt=read(p1_pipe->fd[0],recvmsg_from_pipe_1,sizeof(recvmsg_from_pipe_1));
        printf("%s\n",recvmsg_from_pipe_1);

        if(strcmp(recvmsg_from_pipe_1,on_soil)==0){
            send(server_socket,on_soil,strlen(on_soil),0);
        }
        else{
            send(server_socket,off_soil,strlen(off_soil),0);                
        }
            
        
    
    }

    
    pthread_exit(0);
}

int main(int argc,char* argv[]){
    
    initialize_button();
    get_first_page(1);
    pid_t pid;

    pipe(fd_pi1);
    pipe(fd_pi2);
    pipe(pi1.fd);
    pipe(pi2.fd);
    pid= fork();


 
    if(pid>0){ // 부모프로세스 = pi3로부터 센서값 받아옴 >> server
        pthread_t event_listener;
        pthread_create(&event_listener,NULL,button_listener,NULL);
        pthread_detach(event_listener);
        

        char recvmsg_from_pi3[MAX_LEN];


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
            printf("Connection Success\n");


            int numofsensors=5; // 받아오는 센서값이 4개라고 가정(온도,습도, 물수위, 토양수분, 조도) >> 이 순서대로 들어옴 ex) "111$222$333$444$555$"
            char sensor_value[20];
            while(1){
                ssize_t cnt = recv(client_socket,recvmsg_from_pi3,sizeof(recvmsg_from_pi3),0); //센서값 받아오기
                recvmsg_from_pi3[cnt]='\0';
                printf("%s\n",recvmsg_from_pi3);
                int ret=0;
                int start=0;
                for(int i=0;i<strlen(recvmsg_from_pi3);i++){
                    if(recvmsg_from_pi3[i]=='$'){
                        switch (ret){
                            case 0:
                                temp_cur = atoi(sliceString(recvmsg_from_pi3, start, i, sensor_value));
                                printf("%d\n",temp_cur);
                                break;
                            case 1:
                                humid_cur = atoi(sliceString(recvmsg_from_pi3, start, i, sensor_value));                            
                                printf("%d\n",light_cur);
                                break;
                            case 2:
                                water_cur = atoi(sliceString(recvmsg_from_pi3, start, i, sensor_value));
                                printf("%d\n",humid_cur);
                                break;
                            case 3:
                                soil_cur = atoi(sliceString(recvmsg_from_pi3, start, i, sensor_value));
                                printf("%d\n",soil_cur);
                                break;
                            case 4:
                                light_cur = atoi(sliceString(recvmsg_from_pi3, start, i, sensor_value));
                                printf("%d\n",soil_cur);
                                break;
                        }
                        start=i+1;
                        ret++;
                    }
                }
                
                
                

                if (light_cur < light_setter){
                    write(pi2.fd[1],on_light,sizeof(on_light));

                }
                else{
                    write(pi2.fd[1],off_light,sizeof(off_light));

                }

                if (temp_cur < temp_setter){
                    write(pi1.fd[1],on_temp,sizeof(on_temp));
                    
                    
                }
                else{
                    write(pi1.fd[1],off_temp,sizeof(off_temp));
                    
                }

                if (humid_cur < humid_setter){
                    write(pi1.fd[1],on_humid,sizeof(on_humid));
                }
                else{
                    write(pi1.fd[1],off_humid,sizeof(off_humid));
                }

                if (water_cur>=water_setter && soil_cur < soil_setter){
                    write(pi1.fd[1],on_soil,sizeof(on_soil));
                }
                else{
                    write(pi1.fd[1],off_soil,sizeof(off_soil));
                }


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

        forpi1->pi_num = 1;
        forpi1->fd = &pi1;
        

        forpi2->pi_num = 2;
        forpi2->fd = &pi2;
        


        pthread_t pi1, pi2;


        pthread_create(&pi1,NULL,sendmsg_to_pi1,(void*)forpi1);
        pthread_create(&pi2,NULL,sendmsg_to_pi2,(void*)forpi2);

        pthread_join(pi1,NULL);
        pthread_join(pi2,NULL);
    }
    

    



    return 0;
}
