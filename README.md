
This a software and hardware project to build a small Arduino based 
game show that allows for up to four players. Players can "buzz in" 
to answer trivia questions or otherwise and there are a number of 
features that allow you to customize the game build into the software.

It will automatically determine the first person to buzz in, play sound
effects and time the game for you. 

-- jna July, 2015. 

# Controls

## General Use

- Use RIGHT to enter setup at initial screen, when time is up, or game paused.
- Use LEFT to exit setup or the current menu option.
- Use DOWN to start a game.
- Use SELECT to pause a running game. Hit SELECT again to continue.
- To abort the currently running game, Pause the game, and hit UP

## Setup Options

Once you've entered setup you get a number of options. These changes are
written to the Arduino's EEPROM. 

- Move through the options with UP or DOWN.
- Use SELECT to start editing
- Use UP or DOWN when editing to change values.
- Use LEFT to exit 
- Exit setup completely to save your changes. Changes do not save until you have fully exited setup.
- Setup will timeout after ten seconds if you do nothing.

### Options

- **Max Time** (Default 5 minutes): In increments of 30 seconds, change the length of the round. Set this to 00:00:00 to disable the timer entirely. 
- **Autonext** (Default Off): When someone buzzes in the game does not continue until the game master pushes the SELECT button. If Autonext is on the game will automatically continue after the specified time. Time is in seconds. 
- **Soundset** (Default 1): Chooses the currently active soundset
- **Unique Sounds** (Default Off): Play a unique per-player sound for their buzzer
- **MultiBuzz** (Default Off): Instead of allowing only one person to buzz in at a time, display the first person to buzz in and also display subsequent player-numbers that buzz in. I find this to be confusing, but there it is. 
- **Buzz in pause** (Default Off): If on, allows players to buzz in when the game is paused. Otherwise, the system will play the "invalid" sound. 
- **Buzzer lockout** (Default Off): I should have really named this "Buzz in once per round", but there's only 14 characters to work with on the display. If this is turned on, a player may only buzz in once per round. Once they buzz in, they're locked out until the clock is restarted. 
- **Beep Lastten** (Default Off): If turned on, we'll play a beep and flash the LEDs on the buzzers during the last ten seconds of the game. TODO: allow people to use a custom MP3 here instead of the 1khz test-tone. 
- **Factory Reset** Resets the system to all default values. It will blink once when you hit select, that resets the game. 

### Setup Internals

If you change the code, pay close attention to the `CONFIG_VERSION` and `CONFIG_START` #defines.

We try to read a few bytes from the EEPROM. If they match the `CONFIG_VERSION`
string we assume that a valid configuration struct exists at that location 
of the EEPROM. 

If that location contains a differently sized struct or garbage, we'll use that as the config, so 
it's vital that you change the config version string if any changes happen to the configuration struct. 

If you'd like to change the defaults, edit the `resetConfig()` function.

# Design Notes

## Sofware

This software uses the VS1053 libraries and RGBLCDSHield libraries from Adafruit. It's also quite large. You will absolutely need a ATMEGA32 to run it.

At last compile we were very close to the edge of what the board could do, at around 80% usage. Don't panic about the "low memory messages" out of the compiler. We never use more than a hundred bytes or so for local variables and are careful to not allocate memory we don't have. No crashes yet!

This is what a normal compile looks like:
```
Sketch uses 25,042 bytes (81%) of program storage space. Maximum is 30,720 bytes.
Global variables use 1,654 bytes (80%) of dynamic memory, leaving 394 bytes for local variables. Maximum is 2,048 bytes.
Low memory available, stability problems may occur.
```

## Hardware

We love Adafruit. Buy some stuff. 

1. Adafruit Music Maker card (Uses digital pins 11,12,13)
- https://learn.adafruit.com/adafruit-music-maker-shield-vs1053-mp3-wav-wave-ogg-vorbis-player/overview
1. Adafruit Display/button board (2 wire, i2c)
https://www.adafruit.com/products/772
1. CAT 5 Patch Shield https://www.adafruit.com/product/256
1. Arduino Uno or Dumiellanova (32k AVR required!) https://www.adafruit.com/products/50
1. Cat 5 connectors and crimpers
1. Use Stacking headers on all Arduino shields! You'll need an extra set to lift the display board up from the patch panel. https://www.adafruit.com/products/85

You'll need to solder and build all of these boards (except the Arduino.) 

There is also some interconnect work to be done. The Music Maker's GPIO pins 4 though 7 get connected to the patch shield so that the LEDs will work. On the patch shield, you have to wire the appropriate Arduino digital pins to the player buzzer switches. See the pinout list below. 

The optimal Arduino stack is:

```
Display Backpack (top)
Set of extra stacking headers into patch shield
Patch Shield
VS1053 Shield
Arduino (bottom)
```

## Audio connections

The VS1053 is hard wired, so you don't get to choose what Arduino pins are used.

MCS,DCS,CSC,DREQ (3,4,6,7)
  -- there are 7 GPIO pins on board that are extra. woooo
  -- they have onboard 100K Resistors
  -- don't use GPIO 1 - it will boot into midi mode if tied high

  There are 14 pins on the Arduino UNO, but we're using most of them.

  0,1 TX RX
  3-7 used for music maker
  11-13 used for music maker

## Player Buzzers and LEDs

LEDs for PLAYERS 1-4 Connected to GPIO 4-7 on music maker card
Player switches connect to digital pins as shown in list below 

## Display / Control

Analog4 and Analog5 go to the display. 

## RJ 45 connections

We're using a patch shield here. you'll have to solder wires between
the Arduino pins and the patch panel pins. See the list below. Each
Switch gets a ground and a Arduino PIN. We're using the internal
100k Pull up resistors on the Arduino so you don't have to solder
in resistors. Less work. 

Per jack,  Cat 5 Jack pins 1+2 go to LEDS in the buzzers. 
Cat 5 Jack pins 4+5 go to the Switch.

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
<soundsetnumber>-<soundname>.mp3
```

For example: 
```
1-buzz.mp3    Default buzzer sound, set 1
1-inv.mp3     Invalid buzzer sound (if someone buzzes in during pause), set 1     
1-p1.mp3      Player 1 through 4 unique buzzer sounds (if unique is turned on), set 1
1-p2.mp3          
1-p3.mp3
1-p4.mp3
1-tu.mp3      Time's up sound played when clock runs out
```

You can have up to 256 soundsets, just update the `#define` for `MAX_SOUNDSETS` 

Please check and double check your soundset files. The current configuration is to stop 
the game cold and enter a tight loop if a file cannot be located. 

This was a design decision I made to force myself to make proper sound sets. You might want 
to alter this behavior to just not play a sound. Up to you, it's open source!
