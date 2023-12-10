#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_LEN 1024
#define FIRST1 0
#define FIRST2 1
#define SECOND1 2
#define SECOND2 3
#define TEMP 4
#define LIGHT 5
#define HUMID 6
#define SOIL 7

#define UP 100
#define DOWN 200
#define LEFT 300
#define RIGHT 400


#define BUFFER_MAX 3
int fd;
typedef struct __pipe{
    int fd[2];
}Pipe;
typedef struct params
{
    int pi_num;
    Pipe* fd;
}Params;//스레드에 인자전달을 위한 구조체 선언



void errhandle(char *errmsg){
	fputs(errmsg, stderr);
    fputc('\n', stderr);
    exit(1);
} // 에러핸들링

void initialize_button(){
    int pins[4]={16,17,20,21};
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    fd = open("/sys/class/gpio/export",O_WRONLY);
    if(fd==-1){
        fprintf(stderr, "Failed to open export for writing!\n");
    }

    for(int i=0;i<4;i++){
        bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pins[i]);
        write(fd, buffer, bytes_written);
    }
    printf("button initialize success!\n");
}

void setup_button(int* up, int* down, int* left, int* right){
    *up = open("/sys/class/gpio/gpio20/value",O_RDONLY);
    *down = open("/sys/class/gpio/gpio21/value",O_RDONLY);
    *left = open("/sys/class/gpio/gpio16/value",O_RDONLY);
    *right = open("/sys/class/gpio/gpio17/value",O_RDONLY);
}

int read_button(int direction){
    char value[3];
    if (direction==LEFT){
        int left = open("/sys/class/gpio/gpio16/value",O_RDONLY);
        read(left,value,3);
        close(left);
    }
    else if (direction==RIGHT){
        int right = open("/sys/class/gpio/gpio17/value",O_RDONLY);
        read(right,value,3);
        close(right);
    }
    else if(direction==UP){
        int up = open("/sys/class/gpio/gpio20/value",O_RDONLY);
        read(up,value,3);
        close(up);
    }
    else if(direction==DOWN){
        int down = open("/sys/class/gpio/gpio21/value",O_RDONLY);
        read(down,value,3);
        close(down);
    }

    return atoi(value); 
}

void get_first_page(int num){
    if (num==1){
        system("python /home/pbh7080/team10/LCD_control.py first 1");
    }
    else{
        system("python /home/pbh7080/team10/LCD_control.py first 2");
    }

}
void get_second_page(int num){
    if (num==1){
        system("python /home/pbh7080/team10/LCD_control.py second 1");
    }
    else{
        system("python /home/pbh7080/team10/LCD_control.py second 2");
    }

}
void get_temp_page(int setter, int cur){
    char command[60];
    char* prefix = "python /home/pbh7080/team10/LCD_control.py Temp ";
    snprintf(command,sizeof(command),"%s%d %d",prefix,cur,setter);
    system(command);
}
void get_light_page(int setter, int cur){
    char command[60];
    char* prefix = "python /home/pbh7080/team10/LCD_control.py Light ";
    snprintf(command,sizeof(command),"%s%d %d",prefix,cur,setter);
    system(command);
}
void get_humid_page(int setter, int cur){
    char command[60];
    char* prefix = "python /home/pbh7080/team10/LCD_control.py Humid ";
    snprintf(command,sizeof(command),"%s%d %d",prefix,cur,setter);
    system(command);
}
void get_soil_page(int setter, int cur){
    char command[60];
    char* prefix = "python /home/pbh7080/team10/LCD_control.py Soil ";
    snprintf(command,sizeof(command),"%s%d %d",prefix,cur,setter);
    system(command);
}
char* sliceString(const char *original, int start, int end, char *result) {
    int length = strlen(original);

    if (start < 0 || end >= length || start > end) {
        printf("Invalid indices\n");
        return NULL;
    }

    int j = 0;
    for (int i = start; i <= end; ++i) {
        result[j++] = original[i];
    }

    result[j] = '\0';
    return result;
}
int do_action(int direction, int state, int temp_setter, int light_setter, int humid_setter, int soil_setter, int temp_cur, int light_cur,int humid_cur,int soil_cur){
    switch (state){
        case FIRST1:
            if(direction == RIGHT){
                get_temp_page(temp_setter, temp_cur);
                state=TEMP;
            }
            else if (direction ==DOWN){
                get_first_page(2);
                state = FIRST2;
            }
            else if(direction == UP){
                get_second_page(2);
                state=SECOND2;
            }
            break;
        case FIRST2:
            if(direction == RIGHT){
                get_light_page(light_setter, light_cur);
                state=LIGHT;
            }
            else if (direction ==DOWN){
                get_second_page(1);
                state=SECOND1;
            }
            else if(direction == UP){
                get_first_page(1);
                state = FIRST1;
            }        
            break;
        case SECOND1:
            if(direction == RIGHT){
                get_humid_page(humid_setter, humid_cur);
                state=HUMID;
            }
            else if (direction ==DOWN){
                get_second_page(2);
                state=SECOND2;
            }
            else if(direction == UP){
                get_first_page(2);
                state=FIRST2;
            }     
            break;
        case SECOND2:
            if(direction == RIGHT){
                get_soil_page(soil_setter,soil_cur);
                state=SOIL;
            }
            else if (direction ==DOWN){
                get_first_page(1);
                state=FIRST1;
            }
            else if(direction == UP){
                get_second_page(1);
                state=SECOND1;
            }     
            break;
        case TEMP:
            if(direction == LEFT){
                get_first_page(1);
                state=FIRST1;
            }
            else if (direction ==DOWN || direction== UP){
                get_temp_page(temp_setter,temp_cur);
            }
    
            break;
        case LIGHT:
            if(direction == LEFT){
                get_first_page(2);
                state=FIRST2;
            }
            else if (direction ==DOWN || direction== UP){
                get_light_page(light_setter, light_cur);
            }

            break;
        case HUMID:
            if(direction == LEFT){
                get_second_page(1);
                state=SECOND1;
            }
            else if (direction ==DOWN || direction== UP){
                get_humid_page(humid_setter,humid_cur);
            }
    
            break;
        case SOIL:
            if(direction == LEFT){
                get_second_page(2);
                state=SECOND2;
            }
            else if (direction ==DOWN || direction== UP){
                get_soil_page(soil_setter,soil_cur);
            }
            break;
    }

    return state;
}
