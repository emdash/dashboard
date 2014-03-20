// Dashboard display firmware
// (c) 2014 Brandon Lewis

#include <LiquidCrystal.h>

// LCD Configuration
LiquidCrystal lcd(8, 6, 7, 9, 10, 11, 12);

/*
 * This macro maps shift light LEDS to their respective RPM threshold
 * and bit position I crossed some of the wires. Hence the second
 * column. If you want to tune shift thresholds, adjust the values in
 * the third column of this table.
 */
#define LEDS(PIN) \
  PIN(GN1, 2,  3500) \
  PIN(GN2, 4,  4000) \
  PIN(YL1, 8,  4500) \
  PIN(YL2, 64, 5000) \
  PIN(RD1, 16, 5500) \
  PIN(RD2, 32, 6000)

/*
 * RPM threshold above which all the shift lights blink. Change this
 * if you want the shift lights to blink at a different RPM.
 */
#define ALARM_THRESH 6100

/* This macro is passed into the above macro. The total expansion sets
 * enables and disables LEDS based on RPM by ORing in the
 * corresponding bit. Don't change this.
 */
#define SET(name, pinbit, thresh) if (rpm >= thresh) { leds |= pinbit; }
 
/*
 * SPI Pins for the shift light shift register. Don't change this unless you also change these pins.
 */
#define DATA 3
#define SCLK 4
#define LATCH 5


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

// All samples are 32-bit words after decoding. We use this union to
// interpret the data in the format required. We are relying on the
// fact that AVR micro-controllers use network byte order.
typedef union {
  unsigned long u;
  long s;
  float f;
} Value;


// Data channels
Time last_lap = MAX_LAP;
Time best_lap = MAX_LAP;
Time predicted_lap = MAX_LAP;
unsigned int lap;
float oil_pressure;
float oil_temp;
float water_temp;
int wheel_speed;
int rpm;


// Used for for the protocol state machine. They could be local,
// static variables, except for the fact that it's useful to debug
// them.
Value incoming;
char curchar;
unsigned long nchars;


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


// Sends LED bits to the shift register using SPI
void updateLeds(unsigned char b) {
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, SCLK, MSBFIRST, b);
  digitalWrite(LATCH, HIGH);
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

// Random stuff for debugging purposes. Mostly not useful unless you
// slow down the data stream.
void debug(void)
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print((unsigned char) curchar, 16);

  lcd.setCursor(3, 0);
  lcd.print((unsigned char) curchar & 0x7f, 16);

  lcd.setCursor(6, 0);
  lcd.print(nchars);
  
  lcd.setCursor(8, 0);
  lcd.print(incoming.u, 16);
  
  lcd.setCursor(0, 1);
  lcd.print(incoming.f);
}

// Copies the current input to the appropriate sensor variable. Think
// of it as a mapping between channel ID, variable, and data type.
void updateValues(unsigned char c)
{ 
  switch (c) {
  case 'b':
    best_lap = incoming.u;
    break;
  case 'k':
    lap = incoming.u;
    break;
  case 'l':
    last_lap = incoming.u;
    break;
  case 'm':
    rpm = incoming.u;
    break;
  case 'o':
    oil_temp = incoming.f;
    break;
  case 'p':
    predicted_lap = incoming.u;
    break;
  case 'r':
    oil_pressure = incoming.f;
    break;
  case 's':
    wheel_speed = incoming.u;
    break;
  case 'w':
    water_temp = incoming.f;
    break;
  }
}

// Protocol state machine. Called once for each byte received over
// serial. Data is encoded such that null bytes never occur within
// the message body, leaving allowing them to serve as delimiters.
// Think of it as "base 128" as opposed to "base 64".
void handleChar(char sc)
{
  unsigned char c = (unsigned char) sc;
  curchar = c;

  // reset whenver we get a null byte
  if (c == 0) {
    nchars = 0;
    return;
  }

  switch (nchars++) {
  case 0:
    incoming.u = c & 0x0F;
    return;
  case 1:
  case 2:
  case 3:
  case 4:
    incoming.u <<= 7;
    incoming.u |= c & 0x7f;
    return;
  case 5:
    updateValues(c);
    return;
  default:
    nchars = 0;
    return;
  }

#if 0
  Serial.print(" ");
  Serial.print(String(incoming.u, HEX));
  Serial.print(" "); 
  Serial.print(incoming.f);
  Serial.print(" '");
  Serial.print(sc);
  Serial.println("'");
#endif
} 


// Arduino API entry point, part 1.
void setup()
{
  lcd.begin(16,2);
  lcd.clear();

  // Initialize LED shift register SPI pins
  pinMode(DATA, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(LATCH, OUTPUT);

  // Wait for the data logger / test app to start sending data.
  lcd.print("Waiting for data");  
  Serial.begin(9600);
  while (Serial.read() != 0) {}
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
      LEDS(SET)
    } else if (curFrame & 1) {
      leds = 0xFF;
    }
    
    updateLeds(leds);

    switch (page) {
    case 0: lapTimes(); break;
    case 1: engineTemps(); break;
    case 2: engineMisc(); break;
    case 3: debug(); break;
    }
    update = false;
  }

  // handle serial data
  if (Serial.available()) {
    handleChar(Serial.read());
  }
}

