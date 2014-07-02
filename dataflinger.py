import random
import time
import math
import termios
import sys
import struct

S = 1000
M = S * 60
t0 = time.time(); 

LOWER_RPM = 3000
UPPER_RPM = 6200
RANGE_RPM = UPPER_RPM - LOWER_RPM
X = RANGE_RPM / 2
B = UPPER_RPM - X

def initserial():
    tty = file(sys.argv[1], "r+", 0)
    attrs = termios.tcgetattr(tty)
    attrs[0] &= ~(
        termios.IGNBRK |
        termios.BRKINT |
        termios.PARMRK |
        termios.ISTRIP |
        termios.INLCR  |
        termios.IGNCR  |
        termios.ICRNL  |
        termios.IXON   |
        termios.IXOFF)
    attrs[1] &= ~termios.OPOST
    attrs[2] &= ~(termios.CSIZE  |
                  termios.PARENB |
                  termios.HUPCL)
    attrs[2] |= termios.CS8
    attrs[2] &= ~(
        termios.ECHO   |
        termios.ECHONL |
        termios.ICANON |
        termios.ISIG   |
        termios.IEXTEN)
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(tty, termios.TCSANOW, attrs)

    return tty

tty = initserial()

def t(): return time.time() - t0
def lap(): return random.randrange(55 * S, 2 * M)
best_lap = 9 * M + 59 * S + 999
nlaps = 0

def randomLap():
    global next_lap, next_lap_time, best_lap, nlaps
    next_lap = lap()
    next_lap_time = t() * S + next_lap
    best_lap = min(next_lap, best_lap)
    nlaps += 1;

randomLap()

def rpmCurve(x):
    if -1 <= x <= 0:
        return 2 * x + 1
    elif 0 <= x <= 0.5:
        return 1
    elif 0.5 < x <= 1:
        return -15 * x + 8.5

while True:
    oil_pressure = 50 + random.random() * 10
    water_temp = min(225, 100 + t());
    oil_temp = min(250, 100 + t() * 1.1);
    speed = 100 + 50 * math.sin(math.pi * t() / 15)
    predicted = next_lap + 5 * S *  math.sin(math.pi * t() / 5)
    rpm = 3000 + (t() % 1.0) * 4000

    tty.write('{"s":{"t":%(time)d,"d":['
              '0,0,%(rpm)d,0,0,0,0,0,254]}}\r\n'
              % {"time": t() * 1000,
                 "rpm": rpm})
    
    if t() * S > next_lap_time:
        randomLap()

    sys.stdout.write(tty.read())
    sys.stdout.flush()
    tty.flush()
    time.sleep(0.05)
