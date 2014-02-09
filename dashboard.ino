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

unsigned long incoming_i = 0;
float incoming_f = 0;
int point = 0;
char curchar;
unsigned long nchars;
unsigned long ncommands;
char garbage[17];
unsigned char gi;


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
  lcd.print(millis());
  
  lcd.setCursor(8, 0);
  lcd.print(curFrame * refreshInterval);
  
  lcd.setCursor(0, 1);
  lcd.print(long(curFrame) * refreshInterval);
  
  lcd.setCursor(8, 1);
  lcd.print(incoming_f);
}

void handleChar(char c)
{
    Serial.write(c);
    curchar = c;
    nchars++;
   
    switch (c) {
        case 'l':
          last_lap = incoming_i;
          best_lap = min(best_lap, last_lap);
          lap++;
          break;
        case 'p':
          predicted_lap = incoming_i;
          break;
        case 'o':
          oil_temp = incoming_f;
          break;
        case 'w':
          water_temp = incoming_f;
          break;
        case 'r':
          oil_pressure = incoming_f;
          break;
        case 's':
          wheel_speed = incoming_i;
          break;
        case '\n':
          incoming_i = 0;
          incoming_f = 0;
          point = 0;
          ncommands++;
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          incoming_i = incoming_i * 10 + (c - '0');
          if (point) {
            incoming_f += (float(c - '0') / float(point));
            point *= 10;
          } else {
            incoming_f = incoming_f * 10 + (c - '0');
          }
          break;
       case '.':
         point = 10;
         break;
       default:
         garbage[gi] = c;
         gi = (gi + 1) % 16;
    }
}

void setup()
{
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH);
  lcd.begin(16,2);
  lcd.clear();
  
  Serial.begin(9600);
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
