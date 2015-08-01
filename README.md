Details/design notes:

HARDWARE
-----------------------------------------------------------------------------
1. Adafruit Music Maker card (Uses digital pins 11,12,13)
- https://learn.adafruit.com/adafruit-music-maker-shield-vs1053-mp3-wav-wave-ogg-vorbis-player/overview
- connect GPIO 4-7 to LEDs in the player buzzers with an 8-wire patch
1. Adafruit Display/button board (2 wire, i2c)
https://www.adafruit.com/products/772
1. CAT 5 Patch Shield
1. Arduino Uno or Dumiellanova (32k AVR required!)
1. Cat 5 connectors and crimpers

Audio connections
--------------------
The VS1053 is hard wired, so you don't get to choose these pins. 

MCS,DCS,CSC,DREQ (3,4,6,7)
  -- there are 7 GPIO pins on board that are extra. woooo
  -- they have onboard 100K Resistors
  -- don't use GPIO 1 - it will boot into midi mode if tied high

  There are 14 pins on the arduino UNO, but we're using most of them.

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

RJ 45 connections
------------------
Tied to Blue/Blue White lines
On Patch Panel, pins labelled 4 and 8 connect at button push.
Use internal pullup resistors on arduino instead of soldering resistors.
Patch panel pins 1+2 go to LEDS in the buzzers

Final Arduino Pinout
---------------------
```
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
```

SD CARD
-------

The SD Card in my unit is a 4GB (over kill!!) SanDisk MicroSD card.
I originally wanted to have a nice directory structure but the VS1053 had serious problems playing back from subdirectories, which is really stupid. Files should be MP3s. I first attempted to use WAV and AIF, which resulted in nothing but problems.

Avoid WAV.  The chip seems to really like playing MP3s. 

The system supports multiple "soundsets" which allow you to change the game's sounds en masse. 

The sound files should be named:

```
1-buzz.mp3    Default buzzer sound, set 1
1-inv.mp3     Invalid buzzer sound (if someone buzzes in during pause), set 1     
1-p1.mp3      Player 1 through 4 unique buzzer sounds (if unique is turned on), set 1
1-p2.mp3          
1-p3.mp3
1-p4.mp3
1-tu.mp3      Time's up sound played when clock runs out
2-buzz.mp3    
2-inv.mp3
2-p1.mp3
2-p2.mp3
2-p3.mp3
2-p4.mp3
2-tu.mp3
```

You can have up to 256 soundsets, just update the #define for MAX_SOUNDSETS. 
