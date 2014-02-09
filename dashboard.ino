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

#define CMDLEN 5
#define DATALEN 4
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
  
  lcd.setCursor(8, 0);
  lcd.print(incoming.u, 16);
  
  lcd.setCursor(0, 1);
  lcd.print(incoming.s);
  
  lcd.setCursor(8, 1);
  lcd.print(nchars);
}

void handleChar(char c)
{
  curchar = c;
  Serial.print(String(c, HEX) + " ");
  Serial.flush();

  nchars = (nchars + 1) % CMDLEN;
  
  if (nchars < DATALEN) {
    incoming.u <<= 8;
    incoming.u |= (long(c) & long(0xFF));
    return;
  }

  Serial.print(" (");
  if (isGraph(c) && !isSpace(c)) {
    Serial.print(c);
  }
  Serial.println(")");
  
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

  incoming.u = 0;
}

void setup()
{
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH);
  lcd.begin(16,2);
  lcd.clear();
  
  Serial.begin(9600);

  while (Serial.read() != 0) {}
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
      Serial.print(newPage);
      Serial.print("\n");
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

