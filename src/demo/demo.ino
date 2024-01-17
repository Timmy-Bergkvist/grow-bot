#include <rgb_lcd.h>
#include <Grove_LED_Bar.h>
#include <pcmConfig.h>
#include <TMRpcm.h>
#include <pcmRF.h>
#include <RTClib.h>
#include <SD.h>
#include <Wire.h>

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

// Constants
const int SD_CARD_PIN = 10;
const int WATER_PUMP_RELAY_2 = 7;
const int LED_PIN_RELAY_1 = A3;
const int MOISTURE_SENSOR_PIN = A0;
const int PIR_SENSOR_PIN = A2;
const int RPWM = 5;
const int LPWM = 6;
const int MOTOR_PWM_VALUE = 255; // motor PWM value
const unsigned long MOTOR_DURATION = 1 * 60 * 1000UL;      // Motor active duration (1 minute in millisec)
const unsigned long ACTIVATION_INTERVAL = 15 * 60 * 1000UL; // 15 min Interval for activating the motor
const unsigned long PLAYBACK_INTERVAL = 20UL * 60UL * 1000UL; // 20 min activating speaker
const unsigned long MINIMUM_MSECS_LOW_FOR_INACTIVE = 5000; // Set time for motion to be considered inactive

// Grove components
Grove_LED_Bar groveLedBar(3, 4, 0); // Clock pin, Data pin, Orientation
TMRpcm audio;
RTC_DS3231 rtc;
rgb_lcd lcd;

// Other variables
unsigned long motorActivationStartTime = 0; // Variable to store the motor activation start time
volatile byte state = LOW;
long unsigned int timeLow;                  // Record time when input goes LOW
unsigned long nextPlaybackTime;
unsigned long lastPlaybackTime = 0;
unsigned long currentTime = 0;

// State variables
volatile bool pirState = true;              // Start, assuming no motion detected
volatile bool takeLowTime = false;          // Set initial state of flag to record time when input goes LOW
bool motorActivated = false;                // Motor activation flag
bool isPlaying = false;
bool motorDirectionUp = true;               // Initial direction is up

// RTC variables
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

void setup(){
  pinMode(LED_PIN_RELAY_1, OUTPUT);
  pinMode(WATER_PUMP_RELAY_2, OUTPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(SD_CARD_PIN, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);

  Serial.begin(9600);
  while (!Serial);

  // Initialize the LCD screen
  lcd.begin(16, 2);
  lcd.setRGB(2, 208, 2);
  lcd.createChar(0, heart);
  lcd.createChar(3, armsDown);
  lcd.createChar(4, armsUp);

  // Check if the card is present and can be initialized
  if (!SD.begin(SD_CARD_PIN)) {
    Serial.println(F("Card failed, or not present"));
    while(1);
  }
  Serial.println(F("card initialized."));

  groveLedBar.begin();

  // Initialize the RTC module
  unsigned long startTime = millis();
  unsigned long timeout = 10000;
  
  if (!rtc.begin()) {
    //Serial.println("Couldn't find RTC");
    Serial.flush();
    while (millis() - startTime < timeout);  // Wait until timeout
    while (1);
  }

  if (rtc.lostPower()) {
    //Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Examples for testing
  //rtc.adjust(DateTime(2023, 7, 3, 15, 59, 0));  // Monday 15:59
  //rtc.adjust(DateTime(2023, 6, 30, 7, 59, 2));   // Morning hour (8 AM)
  //rtc.adjust(DateTime(2023, 6, 30, 19, 59, 2));  // Evening hour (8 PM)s
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Initialize the TMRpcm library
  audio.speakerPin = 9;       // Output pin for audio
  audio.setVolume(5);         // volume (0 to 7)
  audio.quality(5);
  nextPlaybackTime = millis() + PLAYBACK_INTERVAL;
}

void loop(){
  DateTime now = rtc.now();
  currentTime = millis();

  // TMRpcm audio
  playAudio(currentTime);

  // Moist sensor
  moistureSensor(currentTime);

  // PIR sensor
  pirSensor();

  // LCD RGB display
  rgbLcd(now);

  // DBH-12 Dual H-Bridge
  // Check if it's time to activate the motor every 30 minutes
  if (currentTime - motorActivationStartTime >= ACTIVATION_INTERVAL) {
    if (motorDirectionUp) {
      activateMotorUp();
    } else {
      activateMotorDown();
    }
    motorActivated = true;
    motorActivationStartTime = currentTime;
    motorDirectionUp = !motorDirectionUp; // Toggle direction
  }
  // Check if the motor is currently activated and if the duration has passed
  if (motorActivated) {
    if (currentTime - motorActivationStartTime >= MOTOR_DURATION) {
      // Deactivate the motor after the motor duration has passed
      motorActivated = false;
      // Deactivation to stop motors
      analogWrite(LPWM, 0);
      analogWrite(RPWM, 0);
    }
  }
}

//---------------------- TMRpcm audio ----------------------
void playAudio(unsigned long currentTime) {
  // Audio is set to play every 40 minutes
  
  // Check if it's time to play the audio file
  if (!isPlaying && currentTime >= nextPlaybackTime) {
    audio.play("toto.wav");
    isPlaying = true;
    lastPlaybackTime = currentTime;
  }
  
  // Check if the audio playback has finished
  if (isPlaying && !audio.isPlaying()) {
    isPlaying = false;
    nextPlaybackTime = currentTime + PLAYBACK_INTERVAL; // Update next playback time
    digitalWrite(audio.speakerPin, LOW); // Turn off speaker
  }
}

//---------------------- Moist sensor ----------------------
void moistureSensor(unsigned long currentTime) {
  const int THRESHOLD = 525;                 // Define max value consider dry soil
  const int dry = 535;
  const int wet = 225;
  const int lowPercentage = 0;               // Percentage for low sensor value
  const int highPercentage = 100;            // Percentage for high sensor value
  const unsigned long readingInterval = 100; // Delay interval in milliseconds
  unsigned long lastReadingTime = 0;

  if (currentTime - lastReadingTime >= readingInterval) {
    lastReadingTime = currentTime;
  
    int moistureValue = analogRead(MOISTURE_SENSOR_PIN);

    // Convert the sensor value to a percentage
    int percentage = map(moistureValue, dry, wet, lowPercentage, highPercentage);
    
    // Scale the percentage to match the range of the Grove LED Bar
    int scaledPercentage = map(percentage, lowPercentage, highPercentage, 2, 11);
    groveLedBar.setLevel(scaledPercentage);

    // ****** For demo, be inactive all the time ******
    digitalWrite(WATER_PUMP_RELAY_2, HIGH); // (relay off)
  }
}


//---------------------- PIR sensor ----------------------
void pirSensor() {
  int pirSensorValue = digitalRead(PIR_SENSOR_PIN);

  if (pirSensorValue == HIGH) {
    digitalWrite(LED_PIN_RELAY_1, LOW); // (relay on)
    if (!pirState) {
      pirState = true;
      takeLowTime = true;
    }
  } else {
    digitalWrite(LED_PIN_RELAY_1, HIGH); // (relay off)

    if(takeLowTime){
      timeLow = millis();
      takeLowTime = false;
    }

    if(pirState && millis() - timeLow > MINIMUM_MSECS_LOW_FOR_INACTIVE){
      pirState = false;
    }
  }
} 

//---------------------- LCD RGB display ----------------------
void displayTimeAndDate(DateTime now) {
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  lcd.setCursor(0, 0);
  lcd.print(dayOfWeek + " ");
  lcd.print(hour < 10 ? "0" : "");
  lcd.print(hour);
  lcd.print(":");
  lcd.print(minute < 10 ? "0" : "");
  lcd.print(minute);
  
  lcd.setCursor(0, 1);
  lcd.print(year);
  lcd.print("/");
  lcd.print(month);
  lcd.print("/");
  lcd.print(day);
}

void displayCustomCharacter() {
  //old 
  unsigned long currentMillis = millis();
  lcd.setCursor(14, 1);
  // Draw arms down:
  lcd.write(3);
  while(millis() - currentMillis < 3000);
  lcd.setCursor(14, 1);
  // Draw arms up:
  lcd.write(4);
  currentMillis = millis();
  while(millis() - currentMillis < 3000);

  // new for test
/*   lcd.setCursor(14, 1);
  // Draw arms down:
  lcd.write(3);
  unsigned long startTime = currentTime; // Record the start time

  while (currentTime - startTime < 3000) {
     // Wait for another 3 seconds
  }

  lcd.setCursor(14, 1);
  // Draw arms up:
  lcd.write(4);
  startTime = currentTime; // Reset the start time
  while (currentTime - startTime < 3000) {
    // Wait for another 3 seconds
  } */
}

void rgbLcd(DateTime now) {
  displayTimeAndDate(now);
  
  displayCustomCharacter();

  // Custom Character Hart
  lcd.setCursor(15, 1);
  lcd.write((unsigned char)0);
}

//---------------------- DBH-12 Dual H-Bridge ----------------------
void activateMotorDown(){
    // Reverse PWM
    analogWrite(LPWM, MOTOR_PWM_VALUE);
    analogWrite(RPWM, 0);
}

void activateMotorUp() {
    // Forward PWM
    analogWrite(LPWM, 0);
    analogWrite(RPWM, MOTOR_PWM_VALUE);
}
