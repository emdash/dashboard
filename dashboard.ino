// Dashboard display firmware
// (c) 2014 Brandon Lewis

#include <LiquidCrystal.h>
#include "rcpparse.h"

// LCD Configuration
LiquidCrystal lcd(12,  // rs
		  11,  // enable
		  5,   // d0
		  4,   // d1
		  3,   // d2
		  2);  // d3

/*
 * This macro maps shift light LEDS to their respective RPM threshold
 * and pin. If you want to tune shift thresholds, adjust the values in
 * the third column of this table.
 */
#define LEDS(PIN)      \
  PIN(GN1, 13, 3500)   \
  PIN(GN2, 10, 4000)   \
  PIN(YL1, 9,  4500)   \
  PIN(YL2, 8,  5000)   \
  PIN(RD1, 7,  5500)   \
  PIN(RD2, 6,  6000)

/*
 * RPM threshold above which all the shift lights blink. Change this
 * if you want the shift lights to blink at a different RPM.
 */
#define ALARM_THRESH 6100

/* This macro is passed into the above macro. The total expansion sets
 * enables and disables LEDS based on RPM by ORing in the
 * corresponding bit. Don't change this.
 */
#define FROM_RPM(name, pin, thresh)  digitalWrite(pin, (rpm >= thresh) ? HIGH : LOW);
#define LEDS_INIT(name, pin, thresh) pinMode(pin, OUTPUT);
#define LEDS_HIGH(name, pin, thresh) digitalWrite(pin, HIGH);
#define LEDS_LOW(name, pin, thresh)  digitalWrite(pin, LOW);

// We use a fixed frame-rate. We only re-draw every refreshInterval
// ms, and even then, only when update is true.
unsigned int refreshInterval = 100;// miliseconds
unsigned int curFrame;
boolean update = true;

// We use potentiometer to choose among one of several "pages"
int sensorPin = A0;
int page;

// Time stuff, could be moved to separate header
#define MINUTES 60000L
#define SECONDS 1000L
#define MAX_LAP (9 * MINUTES + 59 * SECONDS + 999)
typedef unsigned long Time;

#define CONFIG						\
  (ANALOG_0_ENABLED					\
   |ANALOG_1_ENABLED					\
   |ANALOG_2_ENABLED					\
   |FREQ_0_ENABLED					\
   |ACCEL_0_ENABLED					\
   |ACCEL_1_ENABLED					\
   |ACCEL_2_ENABLED 					\
   |ACCEL_3_ENABLED					\
   |GPS_LAT_ENABLED					\
   |GPS_LON_ENABLED					\
   |GPS_SPEED_ENABLED					\
   |GPS_TIME_ENABLED					\
   |GPS_SATELITE_ENABLED)


Time last_lap = MAX_LAP;
Time best_lap = MAX_LAP;
Time predicted_lap = MAX_LAP;
unsigned int lap;
float oil_pressure;
float oil_temp;
float water_temp;
int wheel_speed;
int rpm;

class DashboardParser : public RCPParser {

public:

  DashboardParser(ChannelConfig c) : RCPParser(c) {};

private:

  void processSample (float s, ChannelId id) {
    switch (id) {
    case ANALOG_0:
      oil_pressure = s;
      break;
    case ANALOG_1:
      oil_temp = s;
      break;
    case ANALOG_2:
      water_temp = s;
      break;
    case FREQ_0:
      rpm = s;
      break;
    case GPS_SPEED:
      wheel_speed = s;
      break;
    case TRACK_LAP_TIME:
      last_lap = s;
      best_lap = (s < best_lap) ? s : best_lap;
      break;
    case TRACK_PREDICTED_TIME:
      predicted_lap = s;
      break;
    case TRACK_LAP_COUNT:
      lap = s;
      break;
    }
  }
} p(CONFIG);


// Prints an unsigned integer to the LCD in M:SS.mS format. For some
// reason, the compiler didn't like time declared as a Time.
void printTime(unsigned long time)
{
  int minutes = (time / MINUTES) % 10;
  int seconds = (time % MINUTES) / SECONDS;
  int hundredths = (time / 10) % 100;
  
  char buffer[] = {
     '0' + minutes,
     ':',
     '0' + seconds / 10,
     '0' + seconds % 10,
     ':',
     '0' + hundredths / 10,
     '0' + hundredths % 10,
     0x00
  };
  
  lcd.print(buffer);
}


// Implements the lap time "page" of the display.
void lapTimes (void)
{
  float offset;
  
  lcd.setCursor(0,1);
  printTime(last_lap);

  lcd.setCursor(9, 1);
  printTime(best_lap);

  offset = float(long(predicted_lap) - long(best_lap)) / 1000;
  lcd.setCursor(0, 0);
  lcd.print("       ");
  lcd.setCursor(0, 0);
  if (offset > 0) {
    lcd.print("+");
    lcd.print(offset);
  } else {
    lcd.print(offset);
  }

  lcd.setCursor(9, 0);
  lcd.print("  ");
  lcd.setCursor(9, 0);
  lcd.print(lap);
  
  lcd.setCursor(12, 0);
  lcd.print("   ");
  lcd.setCursor(12, 0);
  lcd.print(wheel_speed);
}

// Impements the engine temperature page of the display
void engineTemps(void)
{
  lcd.setCursor(0, 0);
  lcd.print("Oil:   ");
  lcd.print(oil_temp);
  lcd.setCursor(0, 1);
  lcd.print("Water: ");
  lcd.print(water_temp);
}

// Currently just one sensor value, but that might change.
void engineMisc(void)
{
  lcd.setCursor(0, 0);
  lcd.print("Oil Pres.: ");
  lcd.print(oil_pressure);
}

// Arduino API entry point, part 1.
void setup()
{
  // Initialize LCD. 
  digitalWrite(6, LOW);
  lcd.begin(16,2);
  lcd.clear();

  // Initialize LED pins
  LEDS(LEDS_INIT);

  // Wait for the data logger / test app to start sending data.
  lcd.print("Waiting for data");  
  Serial.begin(115200);
  while (!Serial.available()) {}
  lcd.clear();
}

// Arduino API main loop. Used both to process serial data
// character-by-character, and to update the display every 100ms.
void loop()
{
  unsigned char leds = 0;

  // update display every 100ms.
  if (millis() > (long(curFrame) * long(refreshInterval))) {
    update = true;
    curFrame++;
  }

  // handle the page knob changing.
  int newPage = 4 * analogRead(sensorPin) / 1024;
  if (page != newPage) {
    lcd.clear();
    page = newPage;
    update = true;
  }

  if (update) {
    if (rpm < ALARM_THRESH) {
      LEDS(FROM_RPM);
    } else {
      if (curFrame & 1) {
	LEDS(LEDS_HIGH);
      } else {
	LEDS(LEDS_LOW);
      }
    }

    switch (page) {
    case 0: lapTimes(); break;
    case 1: engineTemps(); break;
    case 2: engineMisc(); break;
    }
    update = false;
  }

  // handle serial data
  while (Serial.available()) {
    p.handleChar(Serial.read());
  }
}

