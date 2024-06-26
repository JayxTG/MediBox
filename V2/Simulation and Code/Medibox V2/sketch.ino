#include <PubSubClient.h>
#include <WiFi.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Constants for OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER 5
#define LED_1  15
#define BUTTON_CANCEL 34 
#define BUTTON_OK 32
#define BUTTON_UP 33
#define BUTTON_DOWN 35
#define DHTPIN 12
#define LDR_LEFT 36
#define LDR_RIGHT 39
#define SERVO_PIN 2

char tempAr[6];
bool isScheduledON = false;
unsigned long scheduledOnTime;
char ldrMaxAr[6];
int leftintensity; 
int rightintensity;
float normalizedMaxIntensity;
char maximumFrom[2];
int minimumAngle = 30;  
float controllingFactor = 0.75;
float servoAngle;
float D;

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET 0


WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntdUDP;
NTPClient timeClient(ntdUDP);

DHTesp dhtSensor;
Servo servoMotor;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global variables
float UTC_OFFSET_DTS = 19800;  // default time is set to Sri Lankan time
float offset =0;

int months  =0;
int days    =0;
int hours   =0;
int minutes =0;
int seconds =0;

bool alarm_enabled =true;
int n_alarms =3;
int alarm_hours[]={0,1,2};
int alarm_minutes[] ={1,10,20};
bool alarm_triggered[] ={false,false,false};

int n_notes =8;
int C= 262;
int R=294;
int E=330;
int F=349;
int G=392;
int A=440;
int B=494;
int C_H=523;

int notes_for_alarm[]={C,R,E,F,G,A,B,C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 -Set Time zone  ", "2 - Set Alaram 1", "3 - Set Alarm 2", "4-set Alarm 3", "5 - Disable Alarms"};

//setting up servo
int pos = 0;

void setup() {
  ledcSetup(0, 5000, 8);
  ledcAttachPin(5, 0);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(BUTTON_CANCEL, INPUT);
  pinMode(BUTTON_OK, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(LDR_LEFT, INPUT);
  pinMode(LDR_RIGHT, INPUT);

  Serial.begin(115200);
  setupWifi();
  setupMqtt();

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  servoMotor.attach(SERVO_PIN, 500, 2400);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600); 

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER,LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(500);

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);
  delay(1000);

  display.clearDisplay();
  print_line("Welcome", 10, 20, 2);
  delay(200);
  display.clearDisplay();
  print_line("ENTC \n Medibox", 10, 20, 2);
  delay(200);
  display.clearDisplay();

  configTime(19800, 0, "pool.ntp.org");
  while (time(nullptr) < 1000) {
    delay(1000);
    Serial.println("Waiting for NTP time...");
 }

  connectToBroker(); // Connect to MQTT broker

  startup_sound();
}

void loop() {
  
  if(!mqttClient.connected()){
    connectToBroker();
  }
  mqttClient.loop();

  update_time_with_check_alarm();

  if (digitalRead(BUTTON_OK) == LOW) {
    delay(200);
    go_to_menu();
  }

  updateTemperature();
  Serial.print("Temperature: ");
  Serial.println(tempAr);
  // Publish temperature to MQTT
  mqttClient.publish("DJTG-ADMIN-TEMP",tempAr);

  leftintensity= readLightIntensity(LDR_LEFT);
  rightintensity=readLightIntensity(LDR_RIGHT);
  maxIntensity();
  Serial.print("Light intensity: ");
  Serial.println(ldrMaxAr);
  // Publish light intensity to MQTT
  mqttClient.publish("LIGHT-INTENSITY",ldrMaxAr);
  mqttClient.publish("MAXIMUM-LIGHT-INTENSITY",ldrMaxAr);
  maxIntensityfrom();

  servoangle();
  checkSchedule();
  // Print servo angle to Serial monitor
  Serial.print("Servo Angle: ");
  Serial.println(servoAngle);

  display.display();

  delay(1000);
}

// Turn buzzer on or off
void buzzerOn(bool on){
  if(on){
      tone(BUZZER,256);
  }
  else{
    noTone(BUZZER);
  }
}

// Update temperature readings
void updateTemperature(){

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

  String(data.temperature,2).toCharArray(tempAr, 6);
}

// Read light intensity from LDR
int readLightIntensity(int ldrPin) {
  int intensity = analogRead(ldrPin);
  return intensity;
}

// Determine maximum light intensity
void maxIntensity(){
  int maxInt = min(leftintensity, rightintensity);
  float normalizedMaxIntensity = normalizeValue(maxInt);
  dtostrf(normalizedMaxIntensity, 4, 2, ldrMaxAr);
  
}

// Determine maximum intensity source
void maxIntensityfrom(){
  if (leftintensity > rightintensity) {
    mqttClient.publish("MAX-LIGHT-INTENSITY-SOURCE", "1");
    strcpy(maximumFrom, "1");
    D= 0.5;
    Serial.println("Maximum from right LDR");
  } else {
    mqttClient.publish("MAX-LIGHT-INTENSITY-SOURCE", "0");
    strcpy(maximumFrom, "0");
    D= 1.5;
    Serial.println("Maximum from left LDR");
  }
  
  
}

// Normalize value to a range of 0 to 1
float normalizeValue(int value) {
  if (value < 1 || value > 4095) {
    return -1.0; // Indicate invalid input 
  }
  return 1.0 - static_cast<float>(value - 1) / (4095.0 - 1.0);
}

// Calculate servo angle
void servoangle(){
float maxIntensity = atof(ldrMaxAr);
 
int angle = minimumAngle;
float factor = controllingFactor;
  
  
  if (minimumAngle != 0) {
    angle = minimumAngle;
  }
  if (controllingFactor != 0) {
    factor = controllingFactor;
  }
  
  servoAngle = min(static_cast<int>(angle * D + (180 - angle) * maxIntensity * factor), 180);
}

// Connect to Wi-Fi
void setupWifi() {
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
// Setup MQTT client
void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

// Callback function for MQTT messages
void receiveCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length];
  for(int i=0 ; i <length; i++){
    Serial.println((char)payload[i]);
    payloadCharAr[i]=(char)payload[i];
  }
  Serial.println();
  if(strcmp(topic,"DJTG-ADMIN-MAIN-ON-OFF")==0){
      buzzerOn(payloadCharAr[0]=='1');
  }
  else if(strcmp(topic,"DJTG-ADMIN-SCH-ON")==0){
    if(payloadCharAr[0]=='N'){
      isScheduledON =false;
    }
    else{
      isScheduledON =true;
      scheduledOnTime =atol(payloadCharAr);
    }
  }  
  else if (strcmp(topic, "MINIMUM-SERVO-ANGLE") == 0) {
    minimumAngle = atof(payloadCharAr);
  }
  else if (strcmp(topic, "CONTROLLING-FACTOR-DJTG") == 0) {
    controllingFactor = atof(payloadCharAr);
  }
  }

// Connect to MQTT broker
void connectToBroker(){
  while(!mqttClient.connected()){
    Serial.println("Attempting MQTT connection..");
    if(mqttClient.connect("ESP32-465456454")){
      Serial.println("Connected");
      mqttClient.subscribe("DJTG-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("DJTG-ADMIN-SCH-ON");
      mqttClient.subscribe("MINIMUM-SERVO-ANGLE");
      mqttClient.subscribe("CONTROLLING-FACTOR-DJTG");
    }
    else{
      Serial.print("Failed");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

// Get current time
unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

// Check schedule for medication intake
void checkSchedule(){
  if(isScheduledON){
    unsigned long currentTime= getTime();
    if (currentTime > scheduledOnTime){
      buzzerOn(true);
      isScheduledON =false;
      mqttClient.publish("DJTG-ADMIN-MAIN-ON-OFF-ESP","1");
      mqttClient.publish("DJTG-ADMIN-SCH-ESP-ON","0");
      Serial.println("Scheduled ON");
    }
  }
}


//-----Past code ------

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
  while(break_happend == false && digitalRead(BUTTON_CANCEL)==HIGH){
  
    for (int i=0 ; i<n_notes; i++){
  
      if (digitalRead(BUTTON_CANCEL)==LOW){
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

    if (digitalRead(BUTTON_UP)==LOW){
      delay(200);                   //debounce the button and ensure a stable reading
      return BUTTON_UP;
    }

    else if (digitalRead(BUTTON_DOWN)==LOW){
      delay(200);                   //debounce the button and ensure a stable reading
      return BUTTON_DOWN;
    }

    else if (digitalRead(BUTTON_OK)==LOW){
      delay(200);                   //debounce the button and ensure a stable reading
      return BUTTON_OK;
    }

    else if (digitalRead(BUTTON_CANCEL)==LOW){
      delay(200);                    //debounce the button and ensure a stable reading
      return BUTTON_CANCEL;
    }        

    update_time();
  }
}


void go_to_menu(){
  
  while (digitalRead(BUTTON_CANCEL)==HIGH){
    
    display.clearDisplay();
    print_line(modes[current_mode],0,0,2);

    int pressed = wait_for_button_press();
   
    // If the UP button is pressed, move to the next mode in the list
    if (pressed == BUTTON_UP){
      delay(200);
      current_mode+=1;
      current_mode =current_mode% max_modes;
    } 
    
    // If the DOWN button is pressed, move to the previous mode in the list
    else if (pressed == BUTTON_DOWN){
      delay(200);
      current_mode -=1;
      if (current_mode<0){
        current_mode = max_modes -1;
      }
    }
    
    // If the OK button is pressed, run the currently selected mode
    else if (pressed == BUTTON_OK){
      delay(200);
      run_mode(current_mode);
    }                 

    // If the CANCEL button is pressed, exit the function and return to the main program loop
    else if (pressed == BUTTON_CANCEL){
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
        
    if (pressed == BUTTON_UP & temp_time_offset <12){             //maximum positive offset is 12
      delay(200);
      temp_time_offset+=0.5;
    }
    else if (pressed == BUTTON_DOWN & temp_time_offset > -12){     // maximum negetive offset is -12  
      delay(200);
      temp_time_offset-=0.5;
    }
    else if (pressed == BUTTON_OK){
      delay(200);
      offset = temp_time_offset;
      break;
    }
    else if (pressed == BUTTON_CANCEL){
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
  
    if(pressed==BUTTON_UP){
      delay(200);
      temp_hour+=1;
      temp_hour=temp_hour % 24;      // ensure that the value of temp_hour stays within the range of 0 to 23.
    }
  
    else if(pressed==BUTTON_DOWN){
      delay(200);
      temp_hour-=1;
      if (temp_hour<0){
        temp_hour=23;
      }
    }
  
    else if (pressed==BUTTON_OK){
      delay(200);
      alarm_hours[alarm]=temp_hour;
      break;
    }

    else if (pressed=BUTTON_CANCEL){
      delay(200);
      break;
    } 

  }
  
  int temp_minute=alarm_minutes[alarm];
  
  while (true){
  
    display.clearDisplay();
    print_line("Enter minute:"+ String(temp_minute),0,0,2);

    int pressed = wait_for_button_press();
  
    if(pressed==BUTTON_UP){
      delay(200);
      temp_minute+=1;
      temp_minute=temp_minute % 60;    // ensure that the value of temp_minute stays within the range of 0 to 59.
    }
  
    else if(pressed==BUTTON_DOWN){
      delay(200);
      temp_minute-=1;
      if (temp_minute<0){
        temp_minute=59;
      }
    }
  
    else if (pressed==BUTTON_OK){
      delay(200);
      alarm_minutes[alarm]=temp_minute;
      break;
    }

    else if (pressed=BUTTON_CANCEL){
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

