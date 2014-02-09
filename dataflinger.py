import random
import time
import math
import termios
import sys

S = 1000
M = S * 60
t0 = time.time(); 

def initserial():
    tty = file("/dev/ttyACM0", "r+", 0)

    time.sleep(2.0)

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
    attrs[4] = termios.B9600
    attrs[5] = termios.B9600
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(tty, termios.TCSANOW, attrs)

    return tty

tty = initserial()

def t(): return time.time() - t0
def lap(): return random.randrange(55 * S, 2 * M)

best_lap = 9 * M + 59 * S + 999

def randomLap():
    global next_lap, next_lap_time, best_lap
    next_lap = lap()
    next_lap_time = t() * S + next_lap
    best_lap = min(next_lap, best_lap)
    tty.write("%dl\n" % next_lap)
    tty.write("%db\n" % best_lap)

randomLap()

def slowWrite(s):
    tty.write(s)

while True:
    oil_pressure = 50 + random.random() * 10
    water_temp = min(225, 100 + t());
    oil_temp = min(250, 100 + t() * 1.1);
    speed = 100 + 50 * math.sin(math.pi * t() / 15)
    predicted = next_lap + 5 * S *  math.sin(math.pi * t() / 5)

    slowWrite("%fo\n" % oil_temp)
    slowWrite("%fw\n" % water_temp)

    slowWrite("%ds\n" % speed)
    slowWrite("%dp\n" % predicted)

    if t() * S > next_lap_time:
        randomLap()

    sys.stdout.write(tty.read())
    time.sleep(.05)
    sys.stdout.flush()
