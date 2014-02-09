import random
import time
import math
import termios
import sys
import struct

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

def sintToBin(i, code):
    return struct.pack("!Ib", i, ord(code))

def uintToBin(u, code):
    return struct.pack("!ib", u, ord(code))

def fToBin(f, code):
    return struct.pack("!fb", f, ord(code))

best_lap = 9 * M + 59 * S + 999
nlaps = 0

def randomLap():
    global next_lap, next_lap_time, best_lap, nlaps
    next_lap = lap()
    next_lap_time = t() * S + next_lap
    best_lap = min(next_lap, best_lap)
    tty.write(uintToBin(next_lap, "l"))
    tty.write(uintToBin(best_lap, "b"))
    tty.write(uintToBin(nlaps, "k"))
    nlaps += 1;

randomLap()

def slowWrite(s):
    for c in s:
        tty.write(c)
        tty.flush()
        time.sleep(0.01)

while True:
    oil_pressure = 50 + random.random() * 10
    water_temp = min(225, 100 + t());
    oil_temp = min(250, 100 + t() * 1.1);
    speed = 100 + 50 * math.sin(math.pi * t() / 15)
    predicted = next_lap + 5 * S *  math.sin(math.pi * t() / 5)

    slowWrite(fToBin(oil_temp, "o"))
    slowWrite(fToBin(water_temp, "w"))

    slowWrite(uintToBin(speed, "s"))
    slowWrite(uintToBin(predicted, "p"))

    if t() * S > next_lap_time:
        randomLap()
    sys.stdout.write(tty.read())
    sys.stdout.flush()
