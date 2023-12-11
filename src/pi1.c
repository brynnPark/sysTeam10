#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT_R1 4
#define POUT_R2 23
#define POUT_F1 20
#define POUT_F2 21
#define POUT_W1 19
#define POUT_W2 26

#define VALUE_MAX 40
#define DIRECTION_MAX 128

/* server로 부터 받는 data 목록
   온도조절: on_H_off_F, off_H_on_F 
   워터펌프 조절: on_WP, off_WP 
   가습기 조절: on_HUMI, off_HUMI */

typedef struct s_sock_data {
	char name[20];
	int sock;

}Sock_data;

static char read_data[40]; 

void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

static int GPIOExport(int pin) {
#define BUFFER_MAX 3
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

  char path[DIRECTION_MAX];
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

static int GPIOWrite(int pin, int value) {
  static const char s_values_str[] = "01";

  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return (-1);
  }

  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
    fprintf(stderr, "Failed to write value!\n");
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


static int GPIOUnexport(int pin) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

void* heater_and_fan_control(void* arg);
void* water_push_control(void* arg);
void* humidifier_control(void* arg);
void* read_data_from_server(void* arg);
void unexport();

int main(int argc, char* argv[]) {
  
  int state = 1;
  int prev_state = 1;

  int serv_sock, clnt_sock = -1;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addr_size;

  // socket을 만듬
  serv_sock = socket(PF_INET, SOCK_STREAM, 0); 
  if (serv_sock == -1) error_handling("socket() error");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

	// bind
  if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) 
	error_handling("bind() error");

  // listen
  if (listen(serv_sock, 5) == -1) error_handling("listen() error"); 

  clnt_addr_size = sizeof(clnt_addr);
  clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
  printf("Connection established\n");
		

  if (GPIOExport(POUT_R1) == -1||GPIOExport(POUT_R2) == -1||GPIOExport(POUT_F1) == -1||GPIOExport(POUT_F2) == -1||GPIOExport(POUT_W1) == -1||GPIOExport(POUT_W2) == -1) {
    return 1;
  }

  if (GPIODirection(POUT_R1, OUT) == -1||GPIODirection(POUT_R2, OUT) == -1||GPIODirection(POUT_F1, OUT) == -1||GPIODirection(POUT_F2, OUT) == -1||GPIODirection(POUT_W1, OUT) == -1||GPIODirection(POUT_W2, OUT) == -1) {
    return 2;
  }

  GPIOWrite(POUT_F1,0);
  GPIOWrite(POUT_W1,0);

  pthread_t p_thread[3];
  int thr_id;
  int status;
	
  // thread함수에 들어갈 값들
  Sock_data p1, p2, p3, pM;
  strcpy(p1.name, "thread_1");
  p1.sock = clnt_sock;
  strcpy(p2.name, "thread_2");
  p2.sock = clnt_sock;
  strcpy(p3.name, "thread_3");
  p3.sock = clnt_sock;
  strcpy(pM.name, "thread_M");
  pM.sock = clnt_sock;

  // tread_1에서 온도조절
  thr_id = pthread_create(&p_thread[0], NULL, heater_and_fan_control, (void *)&p1);
  if (thr_id < 0) {
	perror("thread create error : ");
	exit(0);
  }

  // tread_2에서 워터펌프조절 
  thr_id = pthread_create(&p_thread[1], NULL, water_push_control, (void *)&p2);
  if (thr_id < 0) {
	perror("thread create error : ");
	exit(0);
  }
	
  // tread_3에서 가습기조절
  thr_id = pthread_create(&p_thread[2], NULL, humidifier_control, (void *)&p3);
  if (thr_id < 0) {
	perror("thread create error : ");
	exit(0);
  }
	
  // thread_M에서 서버로부터 data를 받음
  read_data_from_server((void*)&pM);

}

// read_data값이 바뀌는 구간이 Critical Section이므로 mutex look으로 dead lock해결
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

void* read_data_from_server(void* arg){
  
  Sock_data* data = (Sock_data*)arg;
  int str_len;
  fd_set read_fds;
  while(1){
    
    /* select함수로 server의 상태를 계속 확인
		   만약 server가 종료되면 str_len에 0이 적힐 것이다.*/
		FD_ZERO(&read_fds);
		FD_SET(data->sock, &read_fds);

		select(data->sock + 1, &read_fds, NULL, NULL, NULL);

    //get server data
    str_len = recv(data->sock,read_data,40,0);
    if(str_len<=0){
      close(data->sock);
      unexport();
      exit(0);
    }
  }
}

void* heater_and_fan_control(void* arg){
  
  while(1){
    
    pthread_mutex_lock(&data_mutex);
    
    if(strcmp(read_data,"on_H_off_F")==0){
    
      if(GPIOWrite(POUT_R1,1)==-1){
        exit(3);
      }
      if(GPIOWrite(POUT_F2,0)==-1){
        exit(3);
      }
    }
    else if(strcmp(read_data,"off_H_on_F")==0){
    
      if(GPIOWrite(POUT_R1,0)==-1){
        exit(3);
      }
      if(GPIOWrite(POUT_F2,1)==-1){
        exit(3);
      }
    }
    
    pthread_mutex_unlock(&data_mutex);
  }
}

void* water_push_control(void* arg){
  
  while(1){
    
    pthread_mutex_lock(&data_mutex);
    
    if(strcmp(read_data,"on_WP")==0){
    
      if(GPIOWrite(POUT_W2,1)==-1){
        exit(3);
      }
    }
    else if(strcmp(read_data,"off_WP")==0){
    
      if(GPIOWrite(POUT_W2,0)==-1){
        exit(3);
      }
    }
    
    pthread_mutex_unlock(&data_mutex);
  }
}

void* humidifier_control(void* arg){
  
  while(1){
    
    pthread_mutex_lock(&data_mutex);
    
    if(strcmp(read_data,"on_HUMI")==0){
    
      if(GPIOWrite(POUT_R2,1)==-1){
        exit(3);
      }
    }
    else if(strcmp(read_data,"off_HUMI")==0){
    
      if(GPIOWrite(POUT_R2,0)==-1){
        exit(3);
      }
    }
    
    pthread_mutex_unlock(&data_mutex);
  }
}

void unexport(){

  if(GPIOUnexport(POUT_R1)==-1||GPIOUnexport(POUT_R2)==-1||GPIOUnexport(POUT_F1)==-1||GPIOUnexport(POUT_F2)==-1||GPIOUnexport(POUT_W1)==-1||GPIOUnexport(POUT_W2)==-1){
    exit(4);
  }

}
