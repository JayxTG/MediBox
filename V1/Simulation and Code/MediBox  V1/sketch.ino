#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

// Constants for OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Constants for components pins
#define BUZZER 5
#define LED_1  15
#define PB_CANCEL 34 
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET 0


// Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);
DHTesp dhtSensor;


// Global variables
float UTC_OFFSET_DTS = 19800;  // default time is set to Sri Lankan time
float offset =0;

int months  =0;
int days    =0;
int hours   =0;
int minutes =0;
int seconds =0;

unsigned long timeNow =0;
unsigned long timeLast =0;

bool alarm_enabled =true;
int n_alarms =3;
int alarm_hours[]={0,1,2};
int alarm_minutes[] ={1,10,20};
bool alarm_triggered[] ={false,false,false};

int n_notes =8;
int C= 262;
int D=294;
int E=330;
int F=349;
int G=392;
int A=440;
int B=494;
int C_H=523;

int notes_for_alarm[]={C,D,E,F,G,A,B,C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 -Set Time zone  ", "2 - Set Alaram 1", "3 - Set Alarm 2", "4-set Alarm 3", "5 - Disable Alarms"};


void setup() {
  ledcSetup(0, 5000, 8);
  ledcAttachPin(5, 0);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(500);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);
  delay(1000);

configTime(19800, 0, "pool.ntp.org");
  while (time(nullptr) < 1000) {
    delay(1000);
    Serial.println("Waiting for NTP time...");
 }
  //configTime(UTC_OFFSET, UTC_OFFSET_DTS, NTP_SERVER);
  

  display.clearDisplay();

  print_line("Welcome", 10, 20, 2);
  delay(200);
  display.clearDisplay();

  print_line("ENTC \n Medibox", 10, 20, 2);
  delay(200);
  display.clearDisplay();

  startup_sound();
}

void loop() {
  update_time_with_check_alarm();

  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }

  check_temp();
  display.display();
}

// Add a startup sound
void startup_sound() {
  // Play a startup sound when the device boots
  for (int i = 0; i < n_notes; i++) {
    tone(BUZZER, notes_for_alarm[i]);
    delay(200);
  }
  noTone(BUZZER);
}


// Print a string on OLED display
void print_line(String text, int column, int row, int text_size ){
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row);
  display.println(text);
  display.display();
}

// Print a string on OLED display without buffering
void print_line_no_buff(String text,int row,int column,int size){
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(row,column);
  display.println(text);  
}

//Display time in OLED display
void print_time_now(){
  display.clearDisplay();
  String date_now = String(days) + "/" + String(months);
  String time_now = String(hours) + ":" + String(minutes) + ":" + String(seconds);
  print_line_no_buff(date_now , 0 , 0 , 2);
  print_line_no_buff(time_now , 0 , 20 , 2);
  
}

// Updates the global time variables based on the current local time
void update_time(){

  struct tm timeinfo;         // Create a struct to hold the time information
  getLocalTime(&timeinfo);    // Get the current local time and store it in the timeinfo struct

  // Extract the hour information from the timeinfo struct
  char timeHour[3];
  strftime(timeHour,3,"%H",&timeinfo);
  hours = atoi(timeHour);      // Convert the string to an integer and store it in the global hours variable

  char timeMinute[3];
  strftime(timeMinute,3,"%M",&timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3,"%S",&timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d",&timeinfo);
  days = atoi(timeDay);

  char timeMonth[3];
  strftime(timeMonth,3,"%m",&timeinfo);
  months = atoi(timeMonth);

}

//ring alarm at medicine time
void ring_alarm(){
  
  display.clearDisplay();
  print_line("MEDICINE TIME!",0 ,0 ,2);

  digitalWrite(LED_1,HIGH);

  bool break_happend = false;

  //ring the buzzer
  while(break_happend == false && digitalRead(PB_CANCEL)==HIGH){
  
    for (int i=0 ; i<n_notes; i++){
  
      if (digitalRead(PB_CANCEL)==LOW){
        delay(200);
        break_happend == true;
        break;
      }

      tone(BUZZER,notes_for_alarm[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);      // ensure tone has stopped playing completely before the function returns.
    }    
  }

  digitalWrite(LED_1,LOW);
  display.clearDisplay();
}

void show_warning(){
  //blink led when temp and humidity not in range
  digitalWrite(LED_1,HIGH);
  delay(200);
  digitalWrite(LED_1,LOW);
  delay(200);    
}


void update_time_with_check_alarm(void){
  display.clearDisplay();
  update_time();
  print_time_now();

  if (alarm_enabled == true){
    for (int i=0; i< n_alarms; i++){
      if (alarm_triggered[i]==false && alarm_hours[i]==hours && alarm_minutes[i]==minutes){
        ring_alarm();
        alarm_triggered[i]=true; // Set this alarm as triggered so it doesn't ring again
      }
    }
  }
}


int wait_for_button_press(){

  while (true){

    if (digitalRead(PB_UP)==LOW){
      delay(200);                   //debounce the button and ensure a stable reading
      return PB_UP;
    }

    else if (digitalRead(PB_DOWN)==LOW){
      delay(200);                   //debounce the button and ensure a stable reading
      return PB_DOWN;
    }

    else if (digitalRead(PB_OK)==LOW){
      delay(200);                   //debounce the button and ensure a stable reading
      return PB_OK;
    }

    else if (digitalRead(PB_CANCEL)==LOW){
      delay(200);                    //debounce the button and ensure a stable reading
      return PB_CANCEL;
    }        

    update_time();
  }
}


void go_to_menu(){
  
  while (digitalRead(PB_CANCEL)==HIGH){
    
    display.clearDisplay();
    print_line(modes[current_mode],0,0,2);

    int pressed = wait_for_button_press();
   
    // If the UP button is pressed, move to the next mode in the list
    if (pressed == PB_UP){
      delay(200);
      current_mode+=1;
      current_mode =current_mode% max_modes;
    } 
    
    // If the DOWN button is pressed, move to the previous mode in the list
    else if (pressed == PB_DOWN){
      delay(200);
      current_mode -=1;
      if (current_mode<0){
        current_mode = max_modes -1;
      }
    }
    
    // If the OK button is pressed, run the currently selected mode
    else if (pressed == PB_OK){
      delay(200);
      run_mode(current_mode);
    }                 

    // If the CANCEL button is pressed, exit the function and return to the main program loop
    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
}


void set_time_zone(){
  
  float temp_time_offset=offset;
  
  while (true){

    display.clearDisplay();
    print_line("Enter UTC offcet  :" + String(temp_time_offset),0,0,2);

    int pressed = wait_for_button_press();
        
    if (pressed == PB_UP & temp_time_offset <12){             //maximum positive offset is 12
      delay(200);
      temp_time_offset+=0.5;
    }
    else if (pressed == PB_DOWN & temp_time_offset > -12){     // maximum negetive offset is -12  
      delay(200);
      temp_time_offset-=0.5;
    }
    else if (pressed == PB_OK){
      delay(200);
      offset = temp_time_offset;
      break;
    }
    else if (pressed == PB_CANCEL){
      delay(200);
      return;
    }
  } 

  UTC_OFFSET_DTS = offset*3600;
  configTime(UTC_OFFSET, UTC_OFFSET_DTS, NTP_SERVER);

  display.clearDisplay();
  print_line("TIME ZONE IS SET",0,0,2);
  delay(1000);
}

// set alarms to given input times
void set_alarm(int alarm){

  int temp_hour=alarm_hours[alarm];
  
  while (true){
  
    display.clearDisplay();
    print_line("Enter hour:"+ String(temp_hour),0,0,2);

    int pressed = wait_for_button_press();
  
    if(pressed==PB_UP){
      delay(200);
      temp_hour+=1;
      temp_hour=temp_hour % 24;      // ensure that the value of temp_hour stays within the range of 0 to 23.
    }
  
    else if(pressed==PB_DOWN){
      delay(200);
      temp_hour-=1;
      if (temp_hour<0){
        temp_hour=23;
      }
    }
  
    else if (pressed==PB_OK){
      delay(200);
      alarm_hours[alarm]=temp_hour;
      break;
    }

    else if (pressed=PB_CANCEL){
      delay(200);
      break;
    } 

  }
  
  int temp_minute=alarm_minutes[alarm];
  
  while (true){
  
    display.clearDisplay();
    print_line("Enter minute:"+ String(temp_minute),0,0,2);

    int pressed = wait_for_button_press();
  
    if(pressed==PB_UP){
      delay(200);
      temp_minute+=1;
      temp_minute=temp_minute % 60;    // ensure that the value of temp_minute stays within the range of 0 to 59.
    }
  
    else if(pressed==PB_DOWN){
      delay(200);
      temp_minute-=1;
      if (temp_minute<0){
        temp_minute=59;
      }
    }
  
    else if (pressed==PB_OK){
      delay(200);
      alarm_minutes[alarm]=temp_minute;
      break;
    }

    else if (pressed=PB_CANCEL){
      delay(200);
      break;
    }
  } 
  
  display.clearDisplay();
  print_line("Alarm is set",0,0,2);
  delay(1000);

}

void run_mode(int mode){
  
  // If mode is 0, set the time zone
  if (mode == 0){
    set_time_zone();
  }

  // If mode is 1, 2 or 3, set a new alarm
  else if (mode ==1 || mode ==2 || mode==3){
    set_alarm(mode =1);
  }

  // If mode is 4, disable all alarms
  else if (mode ==4){
    alarm_enabled = false;
    display.clearDisplay();
    print_line("All alrams are disabled",0,0,2);
    delay(1500);

  }
}

void check_temp(){

  //get data from dht sensor
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  
  if (data.temperature>32 ){
   print_line_no_buff("Temp High ",0,40,1);
   show_warning();
  }

  else if (data.temperature<26 ){
   print_line_no_buff("Temp Low",0,40,1);
   show_warning();
  }

  if (data.humidity>80 ){
   print_line_no_buff("Humidity High",0,50,1);
   show_warning();
  }

  else if ( data.humidity<60 ){
   print_line_no_buff("Humidity Low",0,50,1);
   show_warning();
  }
}