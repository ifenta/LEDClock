#include <Adafruit_NeoPixel.h>
#include <DS3231.h>

#define NEO_PIN   12 

#define NUMPIXELS 24 

#define FADE_CONSTANT 3.50 //lower number slower fade
#define FRAME_RATE_MS 50.0000 //how often to update LEDs (ms)
#define FRAME_RATE_S FRAME_RATE_MS/1000.0000

#define HOUR_MAX_LED_PWR   250.00
#define MINUTE_MAX_LED_PWR 100.00
#define SECOND_MAX_LED_PWR 253.00

Adafruit_NeoPixel pixels(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

float hour, minute, second;

String message = "";
bool read_message = false;
unsigned long timer;

RTClib myRTC;

//Sets the time in the DS3231 (RTC) via UART input (used for debugging)
void setInitialTimeUART(){
  Serial.println("Input initial time (e.g. 12:45:30:)");
  while(Serial.available()<=0);
  
  if(Serial.available()>0){
    while(Serial.available()>0){
      message+=(char)Serial.read();
    }
    delay(1);
    while(Serial.available()>0){
      message+=(char)Serial.read();
    }
    delay(1);
    while(Serial.available()>0){
      message+=(char)Serial.read();
    }
    Serial.println(message);
    read_message = true;
  }
  if(read_message){
    stringToTime(message);
    setLEDbyTime();
    read_message = false;
    message="";
  }
}

//Prints the time from the DS3231 (RTC), and sets the clock LEDs based on time
void setInitialTime(){

  DateTime now = myRTC.now();
  
  String message = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ":";
  Serial.println("Current Time:   " + message);
  stringToTime(message);
  setLEDbyTime();

}

//Updates global time variables from string input
//str: hour:minute:second: (ie 17:13:48:)
void stringToTime(String str){
  int colon_index = str.indexOf(":");             //index of first colon of message (used to get hours)
  String int_tmp = str.substring(0, colon_index); //hour in string format
  String tmp = str.substring(colon_index+1);      //time string without hour
  hour = int_tmp.toFloat();                       //hour string converted to float

  colon_index = tmp.indexOf(":");                 //index of second colon of message (used to get minutes)
  int_tmp = tmp.substring(0, colon_index);        //minute in string format
  tmp = tmp.substring(colon_index+1);             //time string without hour or minute
  minute = int_tmp.toFloat();                     //minute string converted to float

  colon_index = tmp.indexOf(":");                 //index of last colon of message (used to get seconds)
  int_tmp = tmp.substring(0, colon_index);        //second in string format
  second = int_tmp.toFloat();                     //seconds string converted to floats

  minute+=second/60.00;   //update global minute variable
  hour+=minute/60.00;     //update global hour variable
}

//round the input float value
int round_val(float val){
  int rounded_val = int(val);
  float remainder = val - rounded_val;
  if(remainder >= 0.50) return rounded_val + 1;
  return rounded_val;
}

//Set the LEDs of the clock by the global time variables
void setLEDbyTime(){
  pixels.clear(); //turn off all LEDs
  int red[NUMPIXELS], green[NUMPIXELS], blue[NUMPIXELS];  //contains how much of each color each LED will have

  //zero out all colors
  for(int i=0; i<NUMPIXELS; i++){
    red[i]=0; green[i]=0; blue[i]=0;
  }

  //used to pick the center point on the LED ring for each time variable
  //pretty much converting time to a point on the LED ring
  float hour_led = (fmod(hour,12.00))*(float(NUMPIXELS)/12.00);
  float min_led = minute*float(NUMPIXELS)/60.00;
  float sec_led = second*float(NUMPIXELS)/60.00;

  /* Calculate the acutal LED power for each time metric
   *  
   * LED_PWR= MAX_LED_PWR*e^( -(FADE_CONST*LED_NUM)^2 )
   * LED_PWR shows the LED power for a specific LED
   * MAX_LED_PWR shows the maximum power we set for the specific metric
   * FADE_CONST sets the width of the bell curve (smaller number increase bell curve which makes the fade slower)
   * LED_NUM is the LED number we are modifying. 
   * 
   * We check the two points on either side of the metrics center LED */
  for(int i=-1; i<2; i++){
    red[int(i+min_led)%NUMPIXELS] =   round_val( MINUTE_MAX_LED_PWR*float(exp(-1* pow((FADE_CONSTANT* ( int(i+min_led) ) -FADE_CONSTANT*min_led),2.) ) ) );
    green[int(i+min_led)%NUMPIXELS] = round_val( MINUTE_MAX_LED_PWR*float(exp(-1* pow((FADE_CONSTANT* ( int(i+min_led) ) -FADE_CONSTANT*min_led),2.) ) ) );
    blue[int(i+sec_led)%NUMPIXELS] =  round_val( SECOND_MAX_LED_PWR*float(exp(-1* pow((FADE_CONSTANT* ( int(i+sec_led) ) -FADE_CONSTANT*sec_led),2.) ) ) );
  }
  for(int i=-1; i<2; i++){
    red[int(i+hour_led)%NUMPIXELS] += round_val( HOUR_MAX_LED_PWR*float(exp(-1* pow((FADE_CONSTANT* ( int(i+hour_led) ) -FADE_CONSTANT*hour_led),2.) ) ) );
  }

  //set and show LEDs
  for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(min(red[i],254), green[i], blue[i]));
  pixels.show();
}

void setup() {
  Serial.begin(115200); //start Serial
  delay(10);
  Serial.println();
  Wire.begin(); //start I2C line for RTC comms
  delay(10);
  pixels.begin(); //start LEDs
  delay(10);
  setInitialTime(); //set initial time and shows it on LED
  Serial.println("Begin");
}

void loop() {

  timer = millis();
  while(1){
    
    if(millis()-timer >= FRAME_RATE_MS){
      timer=millis();

      //increment global time based on frame rate
      second+=FRAME_RATE_S;
      minute+=FRAME_RATE_S/60.0000;
      hour+=(FRAME_RATE_S/60.0000)/60.0000;
      if(hour>=24.00){
        hour=0.00;
      }
      if(minute>60.00){
        minute=0.00;
      }
      if(second>60.00){
        second = 0.00;
        setInitialTime(); //set global time from RTC every 60 seconds to take care internal clock drifting
      }
      
      setLEDbyTime(); //update clock LEDs based on the frame rate
    }else if( timer > millis() ) timer = millis();
    delay(0);
  }
  
}
