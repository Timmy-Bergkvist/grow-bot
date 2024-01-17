#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <RTClib.h>
#include <SD.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include "Grove_LED_Bar.h"


// Custom Characters
byte armsDown[8] = {
   0b00100,
   0b01010,
   0b00100,
   0b00100,
   0b01110,
   0b10101,
   0b00100,
   0b01010
};


byte armsUp[8] = {
   0b00100,
   0b01010,
   0b00100,
   0b10101,
   0b01110,
   0b00100,
   0b00100,
   0b01010
};


byte heart[8] = {
   0b00000,
   0b01010,
   0b11111,
   0b11111,
   0b11111,
   0b01110,
   0b00100,
   0b00000
};


Grove_LED_Bar groveLedBar(3, 4, 0); // Clock pin, Data pin, Orientation
TMRpcm audio;
RTC_DS3231 rtc;
rgb_lcd lcd;
int thisChar = 'a';


// SD card pin
int sdCardPin = 10;


// PIR sensor
int ledPinRelay_1 = A3;       // choose the pin for the LED
int pirSensorPin = A2;        // choose the input pin (for PIR sensor)
bool pirState = true;         // we start, assuming no motion detected
const byte interruptPin = 2;  // choose the interrup input pin
volatile byte state = LOW;
const unsigned long minimummSecsLowForInactive = 5000; // Set time threshold for motion to be considered inactive
long unsigned int timeLow;    // Record time when input goes LOW
boolean takeLowTime;          // Set initial state of flag to record time when input goes LOW


// Moist sensor and 4 channel relay
int waterPumpRelay_2 = 7;
int moistureSensorPin = A0;
int threshold = 500;           // Define max value we consider soil 'wet'
bool isMoistureMessagePrinted = false;


// Hydralic pump motor DBH-12 Dual H-Bridge
//int SENSOR_PIN = 0;
int RPWM = 5;                                         // PWM output pin 5;
int LPWM = 6;                                         // PWM output pin 6;
const unsigned long motorDuration = 1 * 60 * 1000UL;  // Motor active duration (1 minute in milliseconds)
bool motorActivated = false;                          // Motor activation flag
unsigned long motorActivationStartTime = 0;           // Variable to store the motor activation start time


// RTC define time and dayes
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int year;
int month;
int day;
int hour;
int minute;
int second;
String dayOfWeek;
String myDate;
String myTime;


// TMRpcm
const unsigned long playbackInterval = 2UL * 60UL * 1000UL; // 2 minutes
//const unsigned long playbackInterval = 5UL * 60UL * 1000UL; // 5 minutes
//const unsigned long playbackInterval = 7UL * 24UL * 60UL * 60UL * 1000UL + 4UL * 60UL * 60UL * 1000UL; // playback interval set to every Monday at 4am
unsigned long nextPlaybackTime;
unsigned long lastPlaybackTime = 0;
bool isPlaying = false;




void setup(){
 pinMode(ledPinRelay_1, OUTPUT);    // declare LED relay as output
 pinMode(waterPumpRelay_2, OUTPUT); // declare relay as output
 pinMode(RPWM, OUTPUT);             // declare DBH-12 Dual H-Bridge relay as output
 pinMode(LPWM, OUTPUT);             // declare DBH-12 Dual H-Bridge relay as output
 pinMode(sdCardPin, OUTPUT);        // declare sd card as output
 pinMode(pirSensorPin, INPUT);      // declare pir sensor as input


 Serial.begin(9600);
 while (!Serial);


 // Initialize the LCD screen
 lcd.begin(16, 2);
 lcd.setRGB(2, 208, 2);        // Set color to lcd
 lcd.createChar(0, heart);
 lcd.createChar(3, armsDown);
 lcd.createChar(4, armsUp);


 // Check if the card is present and can be initialized
 if (!SD.begin(sdCardPin)) {
   Serial.println(F("Card failed, or not present"));
   while(1);
 }
 Serial.println(F("card initialized."));


 groveLedBar.begin();
  // Initialize the RTC module
 if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   Serial.flush();
   while (1) delay(10);
 }


 if (rtc.lostPower()) {
   Serial.println("RTC is NOT running, let's set the time!");
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 }
 // For testing
 //rtc.adjust(DateTime(2021, 12, 31, 16, 59, 0));
 //rtc.adjust(DateTime(2021, 12, 31, 4, 59, 0));
 //rtc.adjust(DateTime(2021, 12, 31, 7, 59, 2));   // Example: Set morning hour (8 AM)
 //rtc.adjust(DateTime(2021, 12, 31, 19, 59, 2));  // Example: Set evening hour (8 PM)


 //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


 // Initialize the TMRpcm library
 audio.speakerPin = 9;       // select the output pin for the audio
 audio.setVolume(5);         // set the volume (0 to 7)
 audio.quality(5);
 nextPlaybackTime = millis() + playbackInterval;


 // Initialize the attach interrupts
 attachInterrupt(digitalPinToInterrupt(sdCardPin), playAudioInterrupt, CHANGE);
 // Use sdCardPin as interrupt pin or this (interruptPin)
 //attachInterrupt(digitalPinToInterrupt(interruptPin), playAudioInterrupt, CHANGE);
 attachInterrupt(digitalPinToInterrupt(moistureSensorPin), moistureSensorInterrupt, CHANGE);
 attachInterrupt(digitalPinToInterrupt(pirSensorPin), pirSensorInterrupt, CHANGE); 
}


void loop(){
 DateTime now = rtc.now();


 // TMRpcm audio
 playAudioInterrupt();


 // Moist sensor and 4 channel relay
 moistureSensorInterrupt();


 // PIR sensor
 // Active during 17:00 PM to 5:00 AM
 if(now.hour() >= 17 || now.hour() < 5){
   pirSensorInterrupt();
 } else {
   // Turn off the relay if it's not night time
   digitalWrite(ledPinRelay_1, HIGH);
 }


 // rgb_lcd display
 rgbLcd(now);


 // DBH-12 Dual H-Bridge
 // Activate the motor to go up at 8 AM
 if (now.hour() == 8 && now.minute() == 0 && !motorActivated) {
   activateMotorUp();
   motorActivated = true;
   motorActivationStartTime = millis();
 }
 // Activate the motor to go down at 8 PM
 if (now.hour() == 20 && now.minute() == 0 && !motorActivated) {
   activateMotorDown();
   motorActivated = true;
   motorActivationStartTime = millis();
 }
 // Check if the motor is currently activated and if the duration has passed
 if (motorActivated) {
   if (millis() - motorActivationStartTime >= motorDuration) {
     // Deactivate the motor after the motor duration has passed
     motorActivated = false;
     // Deactivation to stop motors
     analogWrite(LPWM, 0);
     analogWrite(RPWM, 0);
     Serial.println("Motor deactivated");
   }
 }


}




//---------------------- TMRpcm audio ----------------------
void playAudioInterrupt() {
  unsigned long currentMillis = millis();
  // Check if it's time to play the audio file
 if (!isPlaying && currentMillis >= nextPlaybackTime) {
   audio.play("lofi.wav");
   Serial.println("Now playing lofi.wav");
   isPlaying = true;
   lastPlaybackTime = currentMillis;
  // digitalWrite(audio.speakerPin, HIGH); // Turn on the speaker
 }
  // Check if the audio playback has finished
 if (isPlaying && !audio.isPlaying()) {
   isPlaying = false;
   nextPlaybackTime = currentMillis + playbackInterval; // Update next playback time
   //digitalWrite(audio.speakerPin, LOW); // Turn off the speaker
 }
}


//---------------------- Moist sensor and 4 channel relay ----------------------
void moistureSensorInterrupt() {
 int moistureValue = analogRead(moistureSensorPin);
  int moistureLevel = map(moistureValue, 0, 1023, 0, 10);
 //groveLedBar.setGreenToRed(10);
 //groveLedBar.setLevel(moistureLevel);


  if (moistureValue < threshold && !isMoistureMessagePrinted) {
   digitalWrite(waterPumpRelay_2, HIGH);


   Serial.println("Soil is WET => turn pump 1 OFF (Moisture Value: " + String(moistureValue) + ")");
   isMoistureMessagePrinted = true;
 } else if (moistureValue >= threshold && isMoistureMessagePrinted){
     digitalWrite(waterPumpRelay_2, LOW);


     Serial.println("Soil is DRY => turn pump 1 ON (Moisture Value: " + String(moistureValue) + ")");
     isMoistureMessagePrinted = false;
   }
}


//---------------------- PIR sensor ----------------------
void pirSensorInterrupt() {
 int pirSensorValue = digitalRead(pirSensorPin);


 if (pirSensorValue == HIGH) {
   digitalWrite(ledPinRelay_1, LOW); // Turn LED ON (relay on)
   if (!pirState) {
     // We have just turned on
     pirState = true;
     Serial.println("Motion detected!");
     // We only want to print on the output change, not state
     takeLowTime = true;
   }
 } else {
   digitalWrite(ledPinRelay_1, HIGH); // Turn LED OFF (relay off)


   if(takeLowTime){
     timeLow = millis();
     takeLowTime = false;
   }


   if(pirState && millis() - timeLow > minimummSecsLowForInactive){
     pirState = false;
     Serial.println("Motion ended!");
   }
 }
}


//---------------------- rgb_lcd display ----------------------
void rgbLcd(DateTime now) {
  unsigned long currentMillis = millis();
  
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  lcd.setCursor(0, 0);
  lcd.print(dayOfWeek + " " + hour + ":" + minute);
  
  lcd.setCursor(0, 1);
  lcd.print(year);
  lcd.print("/");
  lcd.print(month);
  lcd.print("/");
  lcd.print(day);

  // Custom Character Man
  lcd.setCursor(14, 1);
  // Draw arms down:
  //lcd.write(3);
  //while(millis() - currentMillis < 950);
  //lcd.setCursor(14, 1);
  // Draw arms up:
  lcd.write(4);
  //currentMillis = millis();
  //while(millis() - currentMillis < 960);

  // Custom Character Hart
  lcd.setCursor(15, 1);
  lcd.write((unsigned char)0);
}


//---------------------- DBH-12 Dual H-Bridge start ----------------------
void activateMotorUp() {
   int reversePWM = 255; // Scale the potentiometer value to the PWM range
   analogWrite(LPWM, reversePWM);
   analogWrite(RPWM, 0);
   Serial.println("Motor going up (Morning)");
}


void activateMotorDown() {
   int forwardPWM = 255;
   analogWrite(LPWM, 0);
   analogWrite(RPWM, forwardPWM);
   Serial.println("Motor going down (Evening)");
}








//groveLedBar.setLed(2,1);
//---------------------- GroveLedBar ----------------------
/*  for (int i = 0; i <= 5; i++)
 {
   groveLedBar.setLevel(i);
   delay(200);
 }
 groveLedBar.setLevel(0);


 // Swich to green to red
 groveLedBar.setGreenToRed(1);


 // Walk through the 10 levels
 for (int i = 0; i <= 5; i++)
 {
   groveLedBar.setLevel(i);
   delay(200);
 }
 groveLedBar.setLevel(0);


 // Switch back to red to green
 groveLedBar.setGreenToRed(0);
 delay(200);


 // Walk through the levels
 // Each reverse keeps the previously set level
 for (int i = 1; i <= 5; i++)
 {
   groveLedBar.setLevel(i);
   delay(500);


   groveLedBar.setGreenToRed(1);
   delay(500);


   groveLedBar.setGreenToRed(0);
 }
 groveLedBar.setLevel(0); */



void moistureSensorInterrupt() {
 int moistureValue = analogRead(moistureSensorPin);

  if (moistureValue < THRESHOLD && !isMoistureMessagePrinted) {
   digitalWrite(waterPumpRelay_2, HIGH);

 } else if (moistureValue >= THRESHOLD && isMoistureMessagePrinted){
     digitalWrite(waterPumpRelay_2, LOW);
   }
}







