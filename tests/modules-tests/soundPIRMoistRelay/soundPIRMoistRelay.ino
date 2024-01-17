#include <SD.h>
#include <RTClib.h>
#include <Wire.h>
#include <TMRpcm.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <SPI.h>

#define SD_ChipSelectPin 10

int ledPin = A3; // choose the pin for the LED
int inputPin = A2; // choose the input pin (for PIR sensor)
const byte interruptPin = 2; // choose the interrup input pin
volatile byte state = LOW;
int pirState = true; // we start, assuming no motion detected
int val = 0; // variable for reading the pin status
int minimummSecsLowForInactive = 5000; // If the sensor reports low for
// more than this time, then assume no activity
long unsigned int timeLow;
boolean takeLowTime;

TMRpcm audio;
RTC_DS3231 rtc;
int thisChar = 'a';

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const unsigned long playbackInterval = 5UL * 60UL * 1000UL; // 5 minutes
unsigned long nextPlaybackTime;
unsigned long lastPlaybackTime = 0;
bool isPlaying = false;

int relay_1 = 7;
int MOISTURE_PIN_A0 = A0;
int THRESHOLD = 500;
bool isMoistureMessagePrinted = false;

void setup() {
  pinMode(ledPin, OUTPUT); // declare LED as output
  pinMode(inputPin, INPUT); // declare sensor as input
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(relay_1, OUTPUT); // Set relay_1 pin as output


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

  audio.speakerPin = 9; // select the output pin for the audio
  audio.setVolume(3); // set the volume (0 to 7)
  audio.quality(2);

  nextPlaybackTime = millis() + playbackInterval;
  Serial.print("Next playback time: ");
  Serial.println(nextPlaybackTime);

  attachInterrupt(digitalPinToInterrupt(interruptPin), playAudio, CHANGE);
  attachInterrupt(digitalPinToInterrupt(inputPin), pirInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(MOISTURE_PIN_A0), moistInterrupt, CHANGE);

}

void loop() {

  // Audio.play code
  playAudio();

  // PIR sensor code
  pirInterrupt();

  // Moist sensor code
  moistInterrupt();

}

// Audio.play code
void playAudio() {
   unsigned long currentMillis = millis();
  
   // check if it's time to play the audio file
  if (!isPlaying && currentMillis >= nextPlaybackTime) {
    audio.play("lofi.wav");
    Serial.println("Now playing lofi.wav");
    isPlaying = true;
    lastPlaybackTime = currentMillis;
  }
  
  // check if the audio playback has finished
  if (isPlaying && !audio.isPlaying()) {
    isPlaying = false;
    nextPlaybackTime = currentMillis + playbackInterval; // update next playback time
  }
}

// moist sensor code
void moistInterrupt() {
  int value = analogRead(MOISTURE_PIN_A0);
  if (value < THRESHOLD && !isMoistureMessagePrinted) {
    Serial.println("Soil is DRY => turn pump 1 ON (value: " + String(value) + ")");
    digitalWrite(relay_1, HIGH);
    isMoistureMessagePrinted = true;
  } else if (value >= THRESHOLD && isMoistureMessagePrinted){
    Serial.println("Soil is WET => turn pump 1 OFF (value: " + String(value) + ")");
    digitalWrite(relay_1, LOW);
    isMoistureMessagePrinted = false;
  }
}

// PIR sensor code
void pirInterrupt() {
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
      timeLow = millis();
      takeLowTime = false;
    }

    if(!pirState && millis() - timeLow > minimummSecsLowForInactive){
      pirState = true;
      Serial.println("Motion ended!");
    }
  }
}

