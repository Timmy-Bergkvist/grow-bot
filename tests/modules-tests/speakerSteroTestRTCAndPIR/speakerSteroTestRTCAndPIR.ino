#include <SD.h>
#include <RTClib.h>
#include <Wire.h>
#include <TMRpcm.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <SPI.h>

#define SD_ChipSelectPin 10

int ledPin = A3;            // choose the pin for the LED
int inputPin = A2;          // choose the input pin (for PIR sensor)
bool pirState = false;      // Set initial state of motion detection flag
int val = 0;                // variable for reading the pin status
unsigned long lastInterruptTime = 0;
unsigned long debounceDelay = 50;
unsigned long minimummSecsLowForInactive = 5000; // Set time threshold for motion to be considered inactive

long unsigned int timeLow;
boolean takeLowTime;

//bool takeLowTime = false; // Set initial state of flag to record time when input goes LOW
//unsigned long timeLow = 0; // Record time when input goes LOW
//const unsigned long playInterval = 360000; // 6 minutes in milliseconds
unsigned long lastPlayTime = 0;

TMRpcm audio;
RTC_DS3231 rtc;
int thisChar = 'a';

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  pinMode(ledPin, OUTPUT);  // declare LED as output
  pinMode(inputPin, INPUT); // declare sensor as input

  Serial.begin(9600);

  while (!Serial);
  Serial.println("start");
  
  pinMode(SD_ChipSelectPin, OUTPUT);

  // Check if the card is present and can be initialized
  if (!SD.begin(SD_ChipSelectPin)) {
    Serial.println(F("Card failed, or not present"));
    while(1);
  }
  Serial.println(F("card initialized."));

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

  // Initialize the TMRpcm library
  audio.speakerPin = 9;
  audio.setVolume(5);
  audio.quality(2);

  // set the last play time to the start of the interval
  //lastPlayTime = millis() - playInterval + 1000;
}

void loop() {
  unsigned long currentMillis = millis();
  
  // handleMotion(currentTime);
   val = digitalRead(inputPin); // read input value
  if (val == HIGH) { // check if the input is HIGH
    digitalWrite(ledPin, HIGH); // turn LED ON
    if (pirState) {
      // we have just turned on
      pirState = false;
      Serial.println("Motion detected!");
      // We only want to print on the output change, not state
      takeLowTime = true;
    }
  } else {
    digitalWrite(ledPin, LOW); // turn LED OFF
 
    if (takeLowTime){
      timeLow = currentMillis;
      takeLowTime = false;
    }
 
    if(!pirState && currentMillis - timeLow > minimummSecsLowForInactive){
      pirState = true;
      Serial.println("Motion ended!");
    }
  }
  
  playSound();
  /* if (currentTime - lastPlayTime >= playInterval) {
    playSound();
    lastPlayTime = currentTime;
  } */

  delay(50);
}


//******************* TMRpcm sound *******************
void playSound() {
  DateTime now = rtc.now();
   
  // Check if it's Monday at 4pm
  if (now.day() == 1 && now.hour() == 16 && now.minute() == 0 && now.second() == 0) {
    audio.play("lofi.wav");

    if(audio.isPlaying()) {
      Serial.println("Audio playing");
    } else {
      Serial.println("Audio not playing");
    }
  }

}
//******************* PIR sensor *******************
/* void handleMotion(unsigned long currentTime) {
  digitalWrite(ledPin, HIGH); // turn LED ON

  int val = digitalRead(inputPin);

  if (val == HIGH) {
    if (pirState == false && currentTime - lastInterruptTime > debounceDelay) {
      pirState = true;
      Serial.println("Motion detected!");
      delay(50); // Wait to prevent multiple messages from being printed
    }
  lastInterruptTime = currentTime;
  } else {
    digitalWrite(ledPin, LOW); // turn LED OFF

    if (currentTime - lastInterruptTime > debounceDelay) {
      lastInterruptTime = currentTime;
      timeLow = currentTime;
      pirState = false;
    }

    if (pirState == true && (currentTime - timeLow > minimummSecsLowForInactive)) {
      pirState = false;
      Serial.println("Motion ended!");
      delay(50); // Wait to prevent multiple messages from being printed
    }
  }
} */
