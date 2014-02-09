// character LCD example code
// www.hacktronics.com

#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);
int backLight = 13;
int sensorPin = A0;
unsigned int refreshInterval = 100;// miliseconds
unsigned int curFrame;

#define MINUTES 60000L
#define SECONDS 1000L
#define MAX_LAP (9 * MINUTES + 59 * SECONDS + 999)
typedef unsigned long Time;

Time last_lap = MAX_LAP;
Time best_lap = MAX_LAP;
Time predicted_lap = MAX_LAP;
unsigned int lap;

float oil_pressure;
float oil_temp;
float water_temp;
int wheel_speed;

int page_knob;
int page;
boolean update = true;

typedef union {
  unsigned long u;
  long s;
  float f;
} Value;

#define CMDLEN 7
#define DATALEN 6
Value incoming;
char curchar;
unsigned long nchars;
unsigned long ncommands;


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

void engineTemps(void)
{
  lcd.setCursor(0, 0);
  lcd.print("Oil:   ");
  lcd.print(oil_temp);
  lcd.setCursor(0, 1);
  lcd.print("Water: ");
  lcd.print(water_temp);
}

void engineMisc(void)
{
  lcd.setCursor(0, 0);
  lcd.print("Oil Pres.: ");
  lcd.print(oil_pressure);
}

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

void handleChar(char sc)
{
  unsigned char c = (unsigned char) sc;
  curchar = c;

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
    break;
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
  
  switch (c) {
  case 'k':
    lap = incoming.u;
    break;
  case 'l':
    last_lap = incoming.u;
    break;
  case 'b':
    best_lap = incoming.u;
    break;
  case 'p':
    predicted_lap = incoming.u;
    break;
  case 'o':
    oil_temp = incoming.f;
    break;
  case 'w':
    water_temp = incoming.f;
    break;
  case 'r':
    oil_pressure = incoming.f;
    break;
  case 's':
    wheel_speed = incoming.u;
    break;
  }
}

void setup()
{
  Value v;

  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH);
  lcd.begin(16,2);

  lcd.print("Waiting for data");  
  Serial.begin(9600);
  while (Serial.read() != 0) {}
  lcd.clear();
}

void loop()
{
  int newPage = 4 * analogRead(page_knob) / 1024;
    
  if (millis() > (long(curFrame) * long(refreshInterval))) {
    update = true;
    curFrame++;
  }
    
  if (page != newPage)
    {
      lcd.clear();
      page = newPage;
      update = true;
    }

  if (update) {
    switch (page) {
    case 0: lapTimes(); break;
    case 1: engineTemps(); break;
    case 2: engineMisc(); break;
    case 3: debug(); break;
    }
    update = false;
  }
    
  if (Serial.available()) {
    handleChar(Serial.read());
  }
}

