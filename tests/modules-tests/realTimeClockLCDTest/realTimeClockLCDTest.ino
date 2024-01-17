#include <Wire.h>
#include "rgb_lcd.h"
 
rgb_lcd lcd;
 
const int colorR = 255;
const int colorG = 222;
const int colorB = 666;
 
void setup() 
{
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
 
    lcd.setRGB(colorR, colorG, colorB);
 
    // Print a message to the LCD.
    lcd.print("hello, world!");
 
    delay(1000);
}
 
void loop() 
{
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.setCursor(0, 1);
    // print the number of seconds since reset:
    lcd.print(millis()/1000);
 
    delay(100);
}

// #include <Wire.h>
// #include "rgb_lcd.h"

// // Date and time functions using a DS1307 RTC connected via I2C and Wire lib
// #include "RTClib.h"

// rgb_lcd lcd;

// int thisChar = 'a';

// RTC_DS1307 rtc;

// const int colorR = 255;
// const int colorG = 333;
// const int colorB = 221;

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// void setup () {
//   Serial.begin(57600);

//   lcd.begin(16, 2);
//   lcd.setRGB(colorR, colorG, colorB);
//   //lcd.print("fuck off");

// #ifndef ESP8266
//   while (!Serial); // wait for serial port to connect. Needed for native USB
// #endif

//   if (! rtc.begin()) {
//     Serial.println("Couldn't find RTC");
//     Serial.flush();
//     abort();
//   }

//   if (! rtc.isrunning()) {
//     Serial.println("RTC is NOT running, let's set the time!");
//     // When time needs to be set on a new device, or after a power loss, the
//     // following line sets the RTC to the date & time this sketch was compiled
//     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//     // This line sets the RTC with an explicit date & time, for example to set
//     // January 21, 2014 at 3am you would call:
//     // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
//   }

//   // When time needs to be re-set on a previously configured device, the
//   // following line sets the RTC to the date & time this sketch was compiled
//   // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//   // This line sets the RTC with an explicit date & time, for example to set
//   // January 21, 2014 at 3am you would call:
//   // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
// }

// void loop () {
//     DateTime now = rtc.now();

//     lcd.print(now.year(), DEC);
//     lcd.print('/');
//     lcd.print(now.month(), DEC);
//     lcd.print('/');
//     lcd.print(now.day(), DEC);
//     Serial.print(" (");
//     Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
//     Serial.print(") ");
//     Serial.print(now.hour(), DEC);
//     lcd.print(':');
//     lcd.print(now.minute(), DEC);
//     lcd.print(':');
//     lcd.print(now.second(), DEC);
//     lcd.println();

//     Serial.print(" since midnight 1/1/1970 = ");
//     Serial.print(now.unixtime());
//     Serial.print("s = ");
//     Serial.print(now.unixtime() / 86400L);
//     Serial.println("d");

//     // calculate a date which is 7 days, 12 hours, 30 minutes, and 6 seconds into the future
//     DateTime future (now + TimeSpan(7,12,30,6));

//     Serial.print(" now + 7d + 12h + 30m + 6s: ");
//     Serial.print(future.year(), DEC);
//     Serial.print('/');
//     Serial.print(future.month(), DEC);
//     Serial.print('/');
//     Serial.print(future.day(), DEC);
//     Serial.print(' ');
//     Serial.print(future.hour(), DEC);
//     Serial.print(':');
//     Serial.print(future.minute(), DEC);
//     Serial.print(':');
//     Serial.print(future.second(), DEC);
//     Serial.println();

//     Serial.println();
//     delay(3000);
// }