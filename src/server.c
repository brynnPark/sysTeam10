#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define PIN1 20
#define POUT1 21

typedef struct s_clnt_ip_data{
  int clnt_sock;
  char ip[20];

  int cur_temp;
  int cur_humi;

}Clnt_ip_data;


typedef struct s_lcd_data{
  int cur_temp;
  int cur_humi;
}Lcd_data;

void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

void *get_in_addr(struct sockaddr *sa){
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int GPIOExport(int pin) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIODirection(int pin, int dir) {
  static const char s_directions_str[] = "in\0out";

  char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    return (-1);
  }

  if (-1 ==
      write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
    fprintf(stderr, "Failed to set direction!\n");
    return (-1);
  }

  close(fd);
  return (0);
}

static int GPIORead(int pin) {
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return (-1);
  }

  if (-1 == read(fd, value_str, 3)) {
    fprintf(stderr, "Failed to read value!\n");
    return (-1);
  }

  close(fd);

  return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
  static const char s_values_str[] = "01";
  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    close(fd);
    return (-1);
  }

  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
    fprintf(stderr, "Failed to write value!\n");
    close(fd);
    return (-1);
  }
  close(fd);
  return (0);
}

static int temp_inf =0, temp_sup =0;
static int humi_inf =0, humi_sup =0;
static Lcd_data *lcd_data;


void *threadTH(void *argment){
  //botten
  while(1){
    printf("wait temp, humi\n");
    scanf("%d %d %d %d", &temp_inf,&temp_sup,&humi_inf,&humi_sup);
    printf("temp: %d~%d humi: %d~%d\n",temp_inf,temp_sup,humi_inf,humi_sup);
  }
}

void *threadLCD(void *argment){

  while(1){

  }
}

//pi
void *threadPi(void *argment){
  
  Clnt_ip_data *client_data = (void*)argment;
  char buf_len;

  /* pthread_detach(pthread_t th);
    ->pthread_join()을 이용하지 않아도 스레드가 종료될때 자원이 반납된다.
    th -> 스레드 식별자. 성공하면 0, 실패하면 error 리턴
  */

  pthread_detach(pthread_self());
  
  int clnt_sock = client_data->clnt_sock;
  char ip[20];
  strcpy(ip,client_data->ip);

  while(1) //10
  {
    int num;
    char buf[1024] = {0};
    num = read(clnt_sock, buf, sizeof(buf) );
    //클라이언트가 갑자기 끊어버리면 read함수는 0을 리턴한다.
    if ( num == 0 ) //여기들어갔다가 while문 탈출.
    {
      printf("connection end bye\n");
      break;
    }

    
    if((strcmp(buf,"pi3")==0)){
      //read temp and humi
      while(1){
        //read(clnt_sock,buf,sizeof(buf));
        //read temp
        //read humi
      }
    }
    else if((strcmp(buf,"pi2")==0)){
      
      if(lcd_data->cur_temp<temp_inf){
        //write led on
      }
      else if (lcd_data->cur_temp>temp_sup){
        //write led off
      }

      if(lcd_data->cur_humi<humi_inf){
        //write led on
      }
      else if(lcd_data->cur_temp<temp_sup) {
        //write led off
      }

    }
    else if((strcmp(buf,"pi1")==0)){

    }
    
    printf("recv %s : [%s] %dbyte\n",ip, buf, num );

  }

  printf("disconnected client ip %s\n",ip);
  free(client_data);
  close(client_data->clnt_sock);

}

int main (int argc, char *argv[]){
  
  int state = 1;
  int prev_state = 1;
  
  int serv_sock = -1;
  int clnt_sock;
  char address[20];

  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addr_size;

  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_sock == -1) error_handling("socket() error");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));
  
  
  if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("bind() error");

  if (listen(serv_sock, 5) == -1) error_handling("listen() error");
  

  printf("set temp and humi\n");
  scanf("%d %d %d %d", &temp_inf,&temp_sup,&humi_inf,&humi_sup);
  printf("temp: %d~%d humi: %d~%d\n",temp_inf,temp_sup,humi_inf,humi_sup);


  pthread_t p_threadM[2];
  
  char* data;
  data = (char*)malloc(sizeof(char));
  char* data2;
  data2 = (char*)malloc(sizeof(char));
  
  pthread_create(&p_threadM[0],NULL,threadTH,(void*)data);
  pthread_create(&p_threadM[1],NULL,threadLCD,(void*)data2);


  pthread_t p_thread;
  int thr_id;
  int status;

  while(1){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1) error_handling("accept() error");
    // wait() client
    
    /* #include <srpa/inet.h>
      inet_ntop(int af, const void * scr, char *dst, socklen_t size);
      :네트워크 주소 구조체인 scr을 char 문자열 dst로 변환.
      af->AF_INET or AF_INET6
    */
   
    inet_ntop(clnt_addr.sin_family,
      get_in_addr((struct sockaddr *)&clnt_addr),
      address, sizeof address);

    printf("server: got connection from %s\n",address);
    
    Clnt_ip_data *data;
    data = (Clnt_ip_data*)malloc(sizeof(Clnt_ip_data));

    data->clnt_sock = clnt_sock;
    strcpy(data->ip,address);

    pthread_create(&p_thread,NULL,threadPi,(void*)data);

  }
  
  return 0;
  
}
