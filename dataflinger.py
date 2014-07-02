#! /usr/bin/python
#
# The MIT License (MIT)
#
#   Copyright (c) 2014 Brandon Lewis
#
#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation
#   files (the "Software"), to deal in the Software without
#   restriction, including without limitation the rights to use, copy,
#   modify, merge, publish, distribute, sublicense, and/or sell copies
#   of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.


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
