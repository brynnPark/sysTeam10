# requires RPi_I2C_driver.py
import LCD_driver
from time import *
import sys

temp_prompt = "Temperature"
light_prompt = "Light"
humidity_prompt = "Humidity"
soil_prompt="Soil_Humidity"
water_prompt="Water_Line"
blank = "  "
arrow = "> "
set_to = "Set to > "
endpar="-Parameter end-"
def set_temp_page(cur,setter):
    mylcd.lcd_display_string("Cur Temp="+cur, 1)
    mylcd.lcd_display_string(set_to+setter, 2)
        
def set_humidity_page(cur,setter):
    mylcd.lcd_display_string("Cur Humid="+cur, 1)
    mylcd.lcd_display_string(set_to+setter, 2)

def set_light_page(cur,setter):
    mylcd.lcd_display_string("Cur Light="+cur, 1)
    mylcd.lcd_display_string(set_to+setter, 2)
        
def set_soil_humidity_page(cur,setter):
    mylcd.lcd_display_string("Cur Soil_H="+cur, 1)
    mylcd.lcd_display_string(set_to+setter, 2)
    
def set_water_page(cur,setter):
    mylcd.lcd_display_string("Cur water="+cur, 1)
    mylcd.lcd_display_string(set_to+setter, 2)

def first_page(num):
    if num=="1":
        mylcd.lcd_display_string(arrow+temp_prompt, 1)
        mylcd.lcd_display_string(blank+light_prompt, 2)
    else:
        mylcd.lcd_display_string(blank+temp_prompt, 1)
        mylcd.lcd_display_string(arrow+light_prompt, 2)        
def second_page(num):
    if num=="1":
        mylcd.lcd_display_string(arrow+humidity_prompt, 1)
        mylcd.lcd_display_string(blank+soil_prompt, 2)
    else:
        mylcd.lcd_display_string(blank+humidity_prompt, 1)
        mylcd.lcd_display_string(arrow+soil_prompt, 2)     
        
def third_page():
    mylcd.lcd_display_string(arrow+water_prompt, 1)
    mylcd.lcd_display_string(endpar, 2)
 
    
    



arrow_loc=0
state=["menu1p_1","menu1p_2","menu2p","settemp","setlight","sethumidity"]
status=state[0]

command = sys.argv[1]


mylcd = LCD_driver.lcd()
mylcd.lcd_clear()

if command=="first":
    par_1 = sys.argv[2]
    first_page(par_1)
elif command=="second":
    par_1 = sys.argv[2]
    second_page(par_1)
elif command=="third":
    third_page()
elif command=="Temp":
    par_1 = sys.argv[2]
    par_2 = sys.argv[3]
    set_temp_page(par_1,par_2)
elif command=="Light":
    par_1 = sys.argv[2]
    par_2 = sys.argv[3]
    set_light_page(par_1,par_2)
elif command=="Humid":
    par_1 = sys.argv[2]
    par_2 = sys.argv[3]
    set_humidity_page(par_1,par_2)
elif command=="Soil":
    par_1 = sys.argv[2]
    par_2 = sys.argv[3]
    set_soil_humidity_page(par_1,par_2)
elif command=="Water":
    par_1 = sys.argv[2]
    par_2 = sys.argv[3]
    set_water_page(par_1,par_2)
    
