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


#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#define MAX_LEN 1024

#define FIRST1 0
#define FIRST2 1
#define SECOND1 2
#define SECOND2 3
#define THIRD 9
#define TEMP 4
#define LIGHT 5
#define HUMID 6
#define SOIL 7
#define WATER 8

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

/*LCD Handling*/
#define LCD_I2C_ADDR 0x27

// Define some device constants
#define LCD_WIDTH 16   // Maximum characters per line
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
// Define some LCD Device constants
#define LCD_CHR 1  // Character mode
#define LCD_CMD 0  // Command mode

#define LCD_LINE_1 0x80  // LCD RAM address for the 1st line
#define LCD_LINE_2 0xC0  // LCD RAM address for the 2nd line
#define LCD_LINE_3 0x94  // LCD RAM address for the 3rd line
#define LCD_LINE_4 0xD4  // LCD RAM address for the 4th line

#define LCD_BACKLIGHT 0x08  // On
// #define LCD_BACKLIGHT 0x00  // Off

#define ENABLE 0b00000100  // Enable bit

// Timing constants
#define E_PULSE 0.0005
#define E_DELAY 0.0005

int i2c_fd;
const char *temp_prompt = "Temperature";
const char *light_prompt = "Light";
const char *humidity_prompt = "Humidity";
const char *soil_prompt = "Soil_Humidity";
const char *water_prompt = "Water_Line";
const char *blank = "  ";
const char *arrow = "> ";
const char *set_to = "Set to > ";
const char *endpar = "-Parameter end-";

void lcd_byte(int bits, int mode) {
    // Send byte to data pins
    // bits = data
    // mode = 1 for character
    //        0 for command

    int bits_high = mode | (bits & 0xF0);
    int bits_low = mode | ((bits << 4) & 0xF0);

    i2c_smbus_write_byte_data(i2c_fd, bits_high, LCD_BACKLIGHT);
    usleep(1000);
    i2c_smbus_write_byte_data(i2c_fd, bits_high | ENABLE, LCD_BACKLIGHT);
    usleep(E_PULSE * 1000);
    i2c_smbus_write_byte_data(i2c_fd, bits_high & ~ENABLE, LCD_BACKLIGHT);
    usleep(1000);

    i2c_smbus_write_byte_data(i2c_fd, bits_low, LCD_BACKLIGHT);
    usleep(1000);
    i2c_smbus_write_byte_data(i2c_fd, bits_low | ENABLE, LCD_BACKLIGHT);
    usleep(E_PULSE * 1000);
    i2c_smbus_write_byte_data(i2c_fd, bits_low & ~ENABLE, LCD_BACKLIGHT);
    usleep(1000);
}

void lcd_init() {
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open the I2C bus");
        exit(1);
    }

    if (ioctl(i2c_fd, I2C_SLAVE, LCD_I2C_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }

    lcd_byte(0x33, LCD_CMD);  // Initialize
    lcd_byte(0x32, LCD_CMD);  // Initialize
    lcd_byte(0x06, LCD_CMD);  // Cursor move direction
    lcd_byte(0x0C, LCD_CMD);  // Display On, Cursor Off, Blink Off
    lcd_byte(0x28, LCD_CMD);  // Data length, number of lines, font size
    lcd_byte(0x01, LCD_CMD);  // Clear display
    usleep(E_DELAY * 1000);
}

void lcd_string(char *message, int line) {
    switch (line) {
        case 1:
            lcd_byte(LCD_LINE_1, LCD_CMD);
            break;
        case 2:
            lcd_byte(LCD_LINE_2, LCD_CMD);
            break;
        case 3:
            lcd_byte(LCD_LINE_3, LCD_CMD);
            break;
        case 4:
            lcd_byte(LCD_LINE_4, LCD_CMD);
            break;
        default:
            lcd_byte(LCD_LINE_1, LCD_CMD);
    }

    for (int i = 0; i < LCD_WIDTH; i++) {
        if (*message) {
            lcd_byte(*message++, LCD_CHR);
        } else {
            break;
        }
    }
}
void clear_LCD(){
    lcd_byte(LCD_CLEARDISPLAY, LCD_CMD);
    lcd_byte(LCD_RETURNHOME, LCD_CMD);
}
/*LCD Handling*/

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
    clear_LCD();
    char concat_string_1[30];
    char concat_string_2[30]; 

    if (num==1){
        strcpy(concat_string_1, arrow);
        strcat(concat_string_1, temp_prompt);
        lcd_string(concat_string_1,1);
        strcpy(concat_string_2, blank);
        strcat(concat_string_2, light_prompt);
        lcd_string(concat_string_2,2);
    }
    else{
        strcpy(concat_string_1, blank);
        strcat(concat_string_1, temp_prompt);
        lcd_string(concat_string_1,1);
        strcpy(concat_string_2, arrow);
        strcat(concat_string_2, light_prompt);
        lcd_string(concat_string_2,2);
    }

}
void get_second_page(int num){
    clear_LCD();
    char concat_string_1[30];
    char concat_string_2[30]; 

    if (num==1){
        strcpy(concat_string_1, arrow);
        strcat(concat_string_1, humidity_prompt);
        lcd_string(concat_string_1,1);
        strcpy(concat_string_2, blank);
        strcat(concat_string_2, soil_prompt);
        lcd_string(concat_string_2,2);
    }
    else{
        strcpy(concat_string_1, blank);
        strcat(concat_string_1, humidity_prompt);
        lcd_string(concat_string_1,1);
        strcpy(concat_string_2, arrow);
        strcat(concat_string_2, soil_prompt);
        lcd_string(concat_string_2,2);
    }

}
void get_third_page(){
    clear_LCD();
    char concat_string_1[30];

    strcpy(concat_string_1, arrow);
    strcat(concat_string_1, water_prompt);
    lcd_string(concat_string_1,1);

    lcd_string(endpar,2);


}
void get_temp_page(int setter, int cur){

    clear_LCD();
    char concat_string_1[30];
    char* cur_prompt = "Cur Temp=";
    snprintf(concat_string_1,sizeof(concat_string_1),"%s%d",cur_prompt,cur);
    lcd_string(concat_string_1,1);

    char concat_string_2[30];
    snprintf(concat_string_2,sizeof(concat_string_2),"%s%d",set_to,setter);
    lcd_string(concat_string_2,2);


}
void get_light_page(int setter, int cur){
    clear_LCD();
    char concat_string_1[30];
    char* cur_prompt = "Cur Light=";
    snprintf(concat_string_1,sizeof(concat_string_1),"%s%d",cur_prompt,cur);
    lcd_string(concat_string_1,1);

    char concat_string_2[30];
    snprintf(concat_string_2,sizeof(concat_string_2),"%s%d",set_to,setter);
    lcd_string(concat_string_2,2);
}
void get_humid_page(int setter, int cur){
    clear_LCD();
    char concat_string_1[30];
    char* cur_prompt = "Cur Humid=";
    snprintf(concat_string_1,sizeof(concat_string_1),"%s%d",cur_prompt,cur);
    lcd_string(concat_string_1,1);

    char concat_string_2[30];
    snprintf(concat_string_2,sizeof(concat_string_2),"%s%d",set_to,setter);
    lcd_string(concat_string_2,2);
}
void get_soil_page(int setter, int cur){
    clear_LCD();
    char concat_string_1[30];
    char* cur_prompt = "Cur Soil_H=";
    snprintf(concat_string_1,sizeof(concat_string_1),"%s%d",cur_prompt,cur);
    lcd_string(concat_string_1,1);

    char concat_string_2[30];
    snprintf(concat_string_2,sizeof(concat_string_2),"%s%d",set_to,setter);
    lcd_string(concat_string_2,2);
}
void get_water_page(int setter, int cur){
    clear_LCD();
    char concat_string_1[30];
    char* cur_prompt = "Cur Water=";
    snprintf(concat_string_1,sizeof(concat_string_1),"%s%d",cur_prompt,cur);
    lcd_string(concat_string_1,1);

    char concat_string_2[30];
    snprintf(concat_string_2,sizeof(concat_string_2),"%s%d",set_to,setter);
    lcd_string(concat_string_2,2);
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
int do_action(int direction, int state, int temp_setter, int light_setter, int humid_setter, int soil_setter, int water_setter, int temp_cur, int light_cur,int humid_cur,int soil_cur,int water_cur){
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
                get_third_page();
                state=THIRD;
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
                get_third_page();
                state=THIRD;
            }
            else if(direction == UP){
                get_second_page(1);
                state=SECOND1;
            }     
            break;
        case THIRD:
            if(direction == RIGHT){
                get_water_page(water_setter,water_cur);
                state=WATER;
            }
            else if (direction ==DOWN){
                get_first_page(1);
                state=FIRST1;
            }
            else if(direction == UP){
                get_second_page(2);
                state=SECOND2;
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
        case WATER:
            if(direction == LEFT){
                get_third_page();
                state=THIRD;
            }
            else if (direction ==DOWN || direction== UP){
                get_water_page(water_setter,water_cur);
            }
            break;
    }

    return state;
}
