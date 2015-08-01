How to build the buzzers
------------------------

The buzzers for this game are essentially a long length of CAT5 cable
with a momentary pushbutton and LED soldered to one end.

The CAT5 cable is terminated with a CAT5 connector which I wired as a TIA568B becuse that's how I'm used to making cables. That pinout is:

TIA568B
1 Orange/White
2 Orange
3 Green/White
4 Blue
5 Blue/White
6 Green
7 Brown/White
8 Brown

At the other end of the cable, Use an 8" long piece of 1/2" tubing as
a handle and protective cover. This works best if you use round
momentary switches that can sit inside of the tubing snugly.

Blue(4) and Blue/White(5) are solderd to the momentary switch. 
Orange(2) and Orange/White(1) are soldered to a high-brightness Blue LED.

GPIO pins on the VS1053 board are tied to 3.3v and are active
LOW. Sending LOW out the GPIO pin in software turns the LED on. 

No current-limiting resistor is used on the LED beacuse Blue LEDs are
around a 3v voltage drop and our GPIO pins can only deliver 3v. You
can add a resistor as needed if you wish. I didn't. If you are using a
different color LED, you may want to add a 50 or 80 ohm resistor.

Repeat this four times.

