#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
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
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <signal.h>
#include <stdbool.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_BUF 2048

#define MAX_TIME 85  
#define DHT11PIN 7
#define LOWHIGH_THRESHOLD 26

typedef struct s_sock_data
{
  /* data */
  char name[20];
  int sock;
} Sock_data;
/*server로 부터 주는 것
 1. 온습도
 2. 조도
 3. 물수위
 4. 토양습도
 */

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = 0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;
int humid_result = -1, temp_result = -1;

static int dht11_val[5] = {0, 0, 0, 0, 0};
static int dht11_temp[5] = {0, 0, 0, 0, 0};
static float farenheit_temp;

void read_dht11_dat();
void *water_control(void *arg);
void *light_control(void *arg);
void *soil_control(void *arg);
void *temperature_control(void *arg);
void *thread_input_to_socket(void *arg);
void reverse(char str[], int lengt);
char *int_to_str(int num, char *str, int base);
void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
static int prepare(int fd)
{
  if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1)
  {
    perror("Can't set MODE");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1)
  {
    perror("Can't set number of BITS");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1)
  {
    perror("Can't set write CLOCK");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1)
  {
    perror("Can't set read CLOCK");
    return -1;
  }

  return 0;
}

uint8_t control_bits_differential(uint8_t channel)
{
  return (channel & 7) << 4;
}

uint8_t control_bits(uint8_t channel)
{
  return 0x8 | control_bits_differential(channel);
}

int readadc(int fd, uint8_t channel)
{
  uint8_t tx[] = {1, control_bits(channel), 0};
  uint8_t rx[3];

  struct spi_ioc_transfer tr = {
      .tx_buf = (unsigned long)tx,
      .rx_buf = (unsigned long)rx,
      .len = ARRAY_SIZE(tx),
      .delay_usecs = DELAY,
      .speed_hz = CLOCK,
      .bits_per_word = BITS,
  };

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1)
  {
    perror("IO Error");
    abort();
  }

  return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

int main(int argc, char **argv)
{
  int sock;
  struct sockaddr_in serv_addr;
  // command는 반드시 ./<파일 이름> <IP> <port>가 되어야한다.
  if (argc != 3)
  {
    printf("Usage : %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  // socket을 만듬
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    error_handling("socket() error");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  // server와 socket연결
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error");

  printf("Connection established\n");

  if (wiringPiSetup() == -1)
  {
    return -1;
  }
  pthread_t p_thread[4];
  int thr_id;
  int status;

  // thread함수에 들어갈 값들
  Sock_data p1, p2, p3, p4, pM;
  strcpy(p1.name, "thread_1");
  p1.sock = sock;
  strcpy(p2.name, "thread_2");
  p2.sock = sock;
  strcpy(p3.name, "thread_3");
  p3.sock = sock;
  strcpy(p4.name, "thread_4");
  p4.sock = sock;
  strcpy(pM.name, "thread_M");
  pM.sock = sock;

  // thread_1 물수위센서
  thr_id = pthread_create(&p_thread[0], NULL, water_control, (void *)&p1);
  if (thr_id < 0)
  {
    perror("thread create error : ");
    exit(0);
  }
  // thread_2 조도센서
  thr_id = pthread_create(&p_thread[1], NULL, light_control, (void *)&p1);
  if (thr_id < 0)
  {
    perror("thread create error : ");
    exit(0);
  }

  // thread_3 토양습도 센서
  thr_id = pthread_create(&p_thread[2], NULL, soil_control, (void *)&p3);
  if (thr_id < 0)
  {
    perror("thread create error : ");
    exit(0);
  }

  // thread_4 온습도센서
  thr_id = pthread_create(&p_thread[3], NULL, temperature_control, (void *)&p3);
  if (thr_id < 0)
  {
    perror("thread create error : ");
    exit(0);
  }

  while (1)
    ;
  return 0;
}

void reverse(char str[], int length)
{
  int start = 0;
  int end = length - 1;
  while (start < end)
  {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

char *int_to_str(int num, char *str, int base)
{
  int i = 0;
  int is_negative = 0;
  if (num == 0)
  {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }
  if (num < 0 && base == 10)
  {
    is_negative = 1;
    num = -num;
  }
  while (num != 0)
  {
    int remainder = num % base;
    if (remainder > 9)
    {
      str[i++] = remainder - 10 + 'a';
    }
    else
    {
      str[i++] = remainder + '0';
    }
    num /= base;
  }
  if (is_negative)
  {
    str[i++] = '-';
  }
  str[i] = '\0';

  reverse(str, i);
  return str;
}

void *water_control(void *arg)
{
  int fd = open(DEVICE, O_RDWR);
  Sock_data *data = (Sock_data *)arg;

  if (fd <= 0)
  {
    perror("Device open error");
    exit(3);
  }

  if (prepare(fd) == -1)
  {
    perror("Device prepare error");
    exit(3);
  }

  char sendmsg[1024];

  while (1)
  {
    char a[1024];
    int_to_str(readadc(fd, 0), sendmsg, 10);
    // printf("water sensor: %s\n", sendmsg);
    send(data->sock, sendmsg, sizeof(sendmsg), 0);
    usleep(10000);
  }
}

void *light_control(void *arg)
{
  int fd = open(DEVICE, O_RDWR);
  Sock_data *data = (Sock_data *)arg;
  int val;

  if (fd <= 0)
  {
    perror("Device open error");
    exit(3);
  }

  if (prepare(fd) == -1)
  {
    perror("Device prepare error");
    exit(3);
  }
  char sendmsg[1024];
  while (1)
  {
    int_to_str(readadc(fd, 0), sendmsg, 10);
    // printf("light sensor: %s\n", sendmsg);
    send(data->sock, sendmsg, sizeof(sendmsg), 0);
    usleep(10000);
  }
}

void *soil_control(void *arg)
{
  int fd = open(DEVICE, O_RDWR);
  Sock_data *data = (Sock_data *)arg;
  if (fd <= 0)
  {
    perror("Device open error");
    exit(3);
  }

  if (prepare(fd) == -1)
  {
    perror("Device prepare error");
    exit(3);
  }
  char sendmsg[1024];
  while (1)
  {
    int_to_str(readadc(fd, 0), sendmsg, 10);
    // printf("soil sensor: %s\n", sendmsg);
    send(data->sock, sendmsg, sizeof(sendmsg), 0);
    usleep(10000);
  }
}

void dht11_read_val()  
{  
  uint8_t lststate=HIGH;  
  uint8_t counter=0;  
  uint8_t j=0,i;  
  float farenheit;  
  for(i=0;i<5;i++)  
     dht11_val[i]=0;  
  pinMode(DHT11PIN,OUTPUT);  
  digitalWrite(DHT11PIN,LOW);  
  delay(18);  
  digitalWrite(DHT11PIN,HIGH);  
  delayMicroseconds(40);  
  pinMode(DHT11PIN,INPUT);  
  for(i=0;i<MAX_TIME;i++)  
  {  
    counter=0;  
    while(digitalRead(DHT11PIN)==lststate){  
      counter++;  
      delayMicroseconds(1);  
      if(counter==255)  
        break;  
    }  
    lststate=digitalRead(DHT11PIN);  
    if(counter==255)  
       break;  
    // top 3 transistions are ignored  
    if((i>=4)&&(i%2==0)){  
      dht11_val[j/8]<<=1;  
      if(counter>16)  
        dht11_val[j/8]|=1;  
      j++;  
    }  
  }  
  // verify cheksum and print the verified data  
  if((j>=40)&&(dht11_val[4]==((dht11_val[0]+dht11_val[1]+dht11_val[2]+dht11_val[3])& 0xFF)))  
  {  
    farenheit=dht11_val[2]*9./5.+32;  
    printf("Humidity = %d.%d %% Temperature = %d.%d *C (%.1f *F)\n",dht11_val[0],dht11_val[1],dht11_val[2],dht11_val[3],farenheit);  
  }  
  else  
    printf("Invalid Data!!\n");  
}
void* temperature_control(void* arg){
  if(wiringPiSetup() == -1){
    exit(1);
  }
  while(1){
    dht11_read_val();
    delay(3000);
  }
  return 0;
}  