Details/design notes:

HARDWARE
-----------------------------------------------------------------------------
1. Adafruit Music Maker card (Uses digital pins 11,12,13)
https://learn.adafruit.com/adafruit-music-maker-shield-vs1053-mp3-wav-wave-ogg-vorbis-player/overview
  * connect GPIO 4-7 to LEDs in the player buzzers with an 8-wire patch

2. Adafruit Display/button board (2 wire, i2c)

3. CAT 5 Patch Shield

4. Arduino Uno or Dumiellanova (32k AVR required!)

5. Cat 5 connectors and crimpers

Audio connections
--------------------

MCS,DCS,CSC,DREQ (3,4,6,7)
  -- there are 7 GPIO pins on board that are extra. woooo
  -- they have onboard 100K Resistors
  -- don't use GPIO 1 - it will boot into midi mode if tied high

  There are 14 pins on the arduino UNO

  0,1 TX RX
  3-7 used for music maker
  11-13 used for music maker

Player Buzzers and LEDs
----------------------------
LEDs for PLAYERS 1-4 Connected to GPIO 4-7 on music maker card
Player switches connect to digital pins as shown in list below 

Display / Control
------------------

Analog4 and Analog5 go to the display. 
0 through 5 are arduino analog wires.

Could maybe move analog4/5 ?
  No, 4 and 5 are only pins with i2C on Decimelia

Does the music maker board use i2c? NO.
  Wooot, it uses the digital 4 pins

Digital is 0-13
Analog is 0-5

RJ 45 connections
------------------
Tied to Blue/Blue White lines
On Patch Panel, pins labelled 4 and 8 connect at button push.
Use internal pullup resistors on arduino instead of soldering resistors.
Patch panel pins 1+2 go to LEDS in the buzzers

Final Arduino Pinout
--------------------------
DIGITAL

0 RX        no connect
1 TX1       no connect
2 PLAYER1   switch to ground
3 MM MCS    hardwired via shield
4 MM DCS    hardwired via shield
5 PLAYER2   switch to ground
6 MM CSC    hardwired via shield
7 MM DREQ   hardwired via shield
8 PLAYER3   switch to ground
9 PLAYER4   switch to ground
10
11 MM SPI MOSI hardwired via shield
12 MM SPI MISO hardwired via shield
13 MM SPI SCLK hardwired via shield

Analog
0
1
2
3
4 LED DISP 4 i2c
5 LED DISP 5 i2c

GPIO
0 
1 P1 LED
2 P2 LED
3 P3 LED
4 P4 LED
5 
6
7
