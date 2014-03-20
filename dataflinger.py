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
    attrs[4] = termios.B9600
    attrs[5] = termios.B9600
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(tty, termios.TCSANOW, attrs)

    return tty

tty = initserial()

def t(): return time.time() - t0
def lap(): return random.randrange(55 * S, 2 * M)

def fToBin(f, code):
    return intToBin(
        struct.unpack("!I", struct.pack("!f", float(f)))[0],
        code)

def intToBin(u, code):
    u = int(u)
    out = bytearray(7)
    out[0] = 128 | ((u >> 28) & 0x0F)
    out[1] = 128 | ((u >> 21) & 0x7F)
    out[2] = 128 | ((u >> 14) & 0x7F)
    out[3] = 128 | ((u >> 7)  & 0x7F)
    out[4] = 128 | (u         & 0x7F)
    out[5] = ord(code) & 0xFF
    out[6] = 0
    print "%x %c" % (u, code);
    return out

def slowWrite(s):
    tty.write(s)
    tty.flush()

best_lap = 9 * M + 59 * S + 999
nlaps = 0

def randomLap():
    global next_lap, next_lap_time, best_lap, nlaps
    next_lap = lap()
    next_lap_time = t() * S + next_lap
    best_lap = min(next_lap, best_lap)
    nlaps += 1;

randomLap()

while True:
    oil_pressure = 50 + random.random() * 10
    water_temp = min(225, 100 + t());
    oil_temp = min(250, 100 + t() * 1.1);
    speed = 100 + 50 * math.sin(math.pi * t() / 15)
    rpm = X * math.sin(t() / 5) + B
    predicted = next_lap + 5 * S *  math.sin(math.pi * t() / 5)

    slowWrite(fToBin(oil_temp, "o"))
    slowWrite(fToBin(water_temp, "w"))
    slowWrite(intToBin(speed, "s"))
    slowWrite(intToBin(predicted, "p"))
    slowWrite(intToBin(nlaps, "k"))
    slowWrite(intToBin(next_lap, "l"))
    slowWrite(intToBin(best_lap, "b"))
    slowWrite(intToBin(rpm, "m"))

    if t() * S > next_lap_time:
        randomLap()

    sys.stdout.write(tty.read())
    sys.stdout.flush()
    tty.flush()
    time.sleep(0.1)
