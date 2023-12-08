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
#include <string.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define POUT 17

/*
  서버로부터 받는 data
  on_led , off_led
*/


static char data[50]; 

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

void* led_lightening(void* arg);
void* data_reading(void* arg);


int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in serv_addr;

  if (argc != 3) {
    printf("Usage : %s <IP> <port>\n", argv[0]);
    exit(1);
  }
  /*
  * socket
  */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1) error_handling("socket() error");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error");

  printf("Connection established\n");

  /*
  * GPIO
  */
  if (-1 == GPIOExport(POUT)) return (1);

  if (-1 == GPIODirection(POUT, OUT)) return (2);

  pthread_t read_data_thread;
  int read_data_pthread_id;

  pthread_t led_thread;
  int led_pthread_id;

  //reading data thread
  read_data_pthread_id = pthread_create(&led_thread, NULL, data_reading,(void*)&sock);  
  if (read_data_pthread_id != 0) {
        error_handling("read_data_thread_error");
    }

  //lightening led thread
  led_pthread_id = pthread_create(&led_thread, NULL, led_lightening, NULL);  
  if (led_pthread_id != 0) {
        error_handling("led_thread_error");
    }

}

void* led_lightening(void* arg){

    while(1){

      if(strcmp(data,"on_led") == 0){

        if(GPIOWrite(POUT,1) == -1){
          error_handling("led_write_error");
        }

      }
      else if (strcmp(data,"off_led") == 0){

        if(GPIOWrite(POUT,0) == -1){
          error_handling("led_write_error");
        }

      }
    }
}

void* data_reading(void* arg){

  int sock = *((int*) arg);
  int value_read;

  while(1){

    value_read = read(sock, data, 50);

    if(value_read <= 0){

      close(sock);

      GPIOUnexport(POUT);
    }
  }
}
