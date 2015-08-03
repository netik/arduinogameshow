/* Arduino Game Show
 * John Adams <jna@retina.net> 7/2015
 *
 * TODO's
 *      - is time up when all buzzers in? (not worth a config opt.)
 */

// include the library code:                                                                                                        
#include <avr/pgmspace.h>
#include <Wire.h>
#include <EEPROM.h>    // to be included only if defined EEPROM_SUPPORT                                                             
#include <SD.h>
#include <SPI.h>

#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_VS1053.h>

/* state machine for events */
#define STATE_NEWGAME 0
#define STATE_RUNNING 1
#define STATE_PAUSED 2
#define STATE_SETUP 3
#define STATE_INSETUP 4
#define STATE_EDITING 5
#define STATE_OUTOFTIME 6
#define STATE_BUZZED_IN 10

/* winning player blink speed */
#define DISPLAYBLINK 1500
#define DEFAULT_MAXTIME 300

// debounce time in mS                                                                                                              
#define DEBOUNCE 10

// delay after switch hit/fake debounce                                                                                             
#define F_DEBOUNCE 200

// Audio shield                                                                                                                     
#define ASHIELD_RESET -1
#define ASHIELD_CS     7
#define ASHIELD_DCS    6
#define ACARDCS 4
#define ADREQ 3

// starting GPIO for player LEDs
#define GPIO_PLAYER    4 

// These #defines make it easy to set the backlight color if using an RGB display                                                   
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

// sounds
#define MAX_SOUNDSETS 2
#define SND_BUZZ    0
#define SND_INVALID 1
#define SND_TIMEUP  2

// player digital IO
#define NUMBUTTONS 4
byte player_buttons[] = {2, 5, 8, 9};
byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS], buzzedin[NUMBUTTONS];
byte lastplayer = -1;

/* configuration and setup */
#define MOVECURSOR 1  // constants for indicating whether cursor should be redrawn
#define MOVELIST 2  // constants for indicating whether cursor should be redrawn

// ID of the settings block
#define CONFIG_VERSION "gs5"
// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// config struct
typedef struct {
  char version[4];
  int maxtime;
  int autonext;
  int soundset;
  boolean unique_sounds;
  boolean pausebuzz;
  boolean multibuzz;
  boolean roundlockout;
  boolean lastten;
} configstr_t;

configstr_t gconfig;

/* game state */
byte current_state = STATE_NEWGAME;
int timeleft = DEFAULT_MAXTIME;

/* master clock */
static unsigned long nexttick;
static unsigned long advanceat;

/* UI */
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
/* Sound */
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(ASHIELD_RESET, ASHIELD_CS, ASHIELD_DCS, ADREQ, ACARDCS);

/* support functions */
char *padstr(char *s1, const char *s2, size_t n)
{
  char *s = s1;
  int maxn = n;

  while (n > 0 && *s2 != '\0') {
    *s++ = *s2++;
    --n;
  }
  while (n > 0) {
    *s++ = ' ';
    --n;
  }

  s1[maxn] = '\0';
  return s1;
}

void setlcd(char *row1, char *row2, byte clear) {
  char myrow1[17];
  char myrow2[17];

  /* put a string to the LCD with full clearing, this will give us a blink. */
  if (clear == 1) {
    lcd.clear();
    strcpy(myrow1, row1);
    strcpy(myrow2, row2);
  } else {
    // pad strings out to 16 chars - we're going to overwrite.
    padstr(myrow1, row1, 16);
    padstr(myrow2, row2, 16);
  }

  lcd.setCursor(0, 0);
  lcd.print(myrow1);
  lcd.setCursor(0, 1);
  lcd.print(myrow2);
}

void time_to_s(char *s, int t, char *state) {
  /* pring a time in seconds to a string with proper padding. */
  int hh, mm, ss;

  hh = t / 3600;
  mm = (t - (hh * 3600)) / 60;
  ss = (t - (mm * 60) - (hh * 3600));

  if (hh < 10) {
    s[0] = '0';
    s[1] = (char) 48 + hh;
  } else {
    s[0] = (char) 48 + hh / 10;
    s[1] = (char) 48 + (hh - (hh / 10) * 10);
  }
  s[2] = ':';

  if (mm < 10) {
    s[3] = '0';
    s[4] = (char) 48 + mm;
  } else {
    s[3] = (char) 48 + mm / 10;
    s[4] = (char) 48 + (mm - (mm / 10) * 10);
  }
  s[5] = ':';

  if (ss < 10) {
    s[6] = '0';
    s[7] = (char) 48 + ss;
  } else {
    s[6] = (char) 48 + ss / 10;
    s[7] = (char) 48 + (ss - (ss / 10) * 10);
  }
  s[8] = '\0';

  // cat the string
  strcat(s, state);
}

void resetConfig() {

  // defaults
  strcpy(gconfig.version, CONFIG_VERSION);
  gconfig.maxtime = DEFAULT_MAXTIME;
  gconfig.autonext = -1;
  gconfig.soundset = 1;
  gconfig.unique_sounds = 0;
  gconfig.pausebuzz = 0;
  gconfig.multibuzz = 0;
  gconfig.roundlockout = 0;
  gconfig.lastten = 0;
}
/* start of game logic */
void loadConfig() {
  Serial.println(F("try to load config"));
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]) {
    Serial.println(F("config found (max, autonext, soundset, unique, pauzebuzz, multibuzz, roundlockout, lastten"));

    for (unsigned int t = 0; t < sizeof(gconfig); t++)
      *((char*)&gconfig + t) = EEPROM.read(CONFIG_START + t);

    Serial.println(gconfig.maxtime);
    Serial.println(gconfig.autonext);
    Serial.println(gconfig.soundset);
    Serial.println(gconfig.unique_sounds);
    Serial.println(gconfig.pausebuzz);
    Serial.println(gconfig.multibuzz);
    Serial.println(gconfig.roundlockout);
    Serial.println(gconfig.lastten);

  } else {
    Serial.println(F("couldn't load config"));
  }

}


void setup() {
  byte i;
  resetConfig();
  Serial.begin(9600);
  loadConfig();

  // Make input & enable pull-up resistors on switch pins
  for (i = 0; i < NUMBUTTONS; i++) {
    pinMode(player_buttons[i], INPUT);
    digitalWrite(player_buttons[i], HIGH);
  }

  /* light 'em up */
  lcd.setBacklight(RED);

  // init the music player
  if (! musicPlayer.begin()) { // init the music player
    setlcd("VS1053 Fail", "Check Audio", 1);
    // i die.
    while (1);
  }

  // optional
  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(10, 10);
  lcd.begin(16, 2);
  
  if (! musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT))
    Serial.println(F("DREQ pin is not an interrupt pin"));

  if (!SD.begin(ACARDCS)) {
    setlcd("NO SD CARD", "Insert SD Card", 1);

    delay(1000);
    
    while (1);  // don't do anything more
  }

  Serial.println("SD OK!");;
  Serial.println(F("VS1053 found"));

  set_player_leds(LOW);
    
  // Print a message to the LCD.
  setlcd(" Game Show v1.0", " jna@retina.net", 1 );

  delay(2000);
  show_first_screen();
  Serial.println(F("system up."));
}

void show_first_screen() {
  char line[17];
  line[0] = 0x7e;
  line[1] = '\0';
  strcat(line, " SETUP  v START");
  setlcd(line, "", 1);
}

void playsound(byte type,byte player) {
  char filename[30];

  filename[0] = (char) gconfig.soundset + 48;
  filename[1] = '-';
  filename[2] = '\0';
 
  switch (type) {
    case SND_BUZZ:
      if (gconfig.unique_sounds) { 
        filename[2] = 'p';
        filename[3] = (char) player + 49;
        filename[4] = '\0';
        strcat(filename, ".mp3");       
      } else {
        strcat(filename, "buzz.mp3");
      }
      break;
    case SND_INVALID:
      strcat(filename, "inv.mp3");
      break;
    case SND_TIMEUP:
      strcat(filename, "tu.mp3");
      break;
  }
 
  Serial.println(filename);
  
  if (musicPlayer.playingMusic) {
    // refuse to play if we are already playing.
    // only two cases of this 
    //   1) two people buzz in at same time
    //   2) person buzzes in at time up (oh well.) 
    return; 
  }
  
  if (! musicPlayer.startPlayingFile(filename)) {
    setlcd("File not found", filename, 1);
    delay(1000);
    while (1);
  }
}

void buzzedin_s(char *dst) {
  // return a string representing who has buzzed in (. = buzzedin, N = not yet)
  for (int i = 0; i < NUMBUTTONS; i++) {
    if (buzzedin[i] == 1) {
      dst[i] = '.';
    } else {
      dst[i] = 49 + i;
    }
  }
  dst[NUMBUTTONS] = '\0';
}
void check_players() {
  static long lasttime;
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];

  byte index;

  if (millis() < lasttime) { // we wrapped around, lets just try again
    lasttime = millis();
  }

  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return;
  }

  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();

  for (index = 0; index < NUMBUTTONS; index++) {
    justpressed[index] = 0;
    justreleased[index] = 0;

    currentstate[index] = digitalRead(player_buttons[index]);   // read the button

    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
        // just pressed
        justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
        // just released
        justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }

    previousstate[index] = currentstate[index];   // keep a running tally of the player_buttons
  }
}

/*
 * Game play sequence
 *
 * Gamemaster hits UP or DOWN
 * Game starts
 * Player buzzes in, clock stops, all buzzers locked out.
 * Gamemaster releases game state with SELECT
 *
 */

void handle_gm_buttons() {
  int val;
  char status_s[17];
  char time_s[17];

  /* handle DOWN button - start of game */
  uint8_t buttons = lcd.readButtons();

  if ( ((buttons & BUTTON_DOWN) &&
        (current_state == STATE_NEWGAME || current_state == STATE_OUTOFTIME)) ||
       ((buttons & BUTTON_UP) &&
        (current_state == STATE_PAUSED)) ) {
    // We are starting a game.
    current_state = STATE_RUNNING;
    timeleft = gconfig.maxtime;
    time_to_s(time_s, timeleft, " RUNNING");
    setlcd("Starting Game...", time_s, 1);
    Serial.println(F("Game starting..."));
    
    set_player_leds(HIGH);

    for (int i = 0; i < NUMBUTTONS; i++) {
      buzzedin[i] = 0;
    }
    nexttick = millis() + 1000;
  }

  
  /* handle RIGHT button */
  if ((buttons & BUTTON_RIGHT) && (current_state == STATE_PAUSED || current_state == STATE_NEWGAME || current_state == STATE_OUTOFTIME)) {
    current_state = STATE_SETUP;
  }

  if ((buttons & BUTTON_SELECT) && (current_state == STATE_PAUSED || current_state == STATE_BUZZED_IN)) {
    // SELECT to unpause the game  or continue after a buzzer.
    current_state = STATE_RUNNING;
    set_player_leds(HIGH); // clear LEDs
    delay(F_DEBOUNCE);
  } else {
    if (buttons & BUTTON_SELECT && current_state == STATE_RUNNING) {
      // SELECT pauses the game
      current_state = STATE_PAUSED;
      time_to_s(time_s, timeleft, " PAUSED");
      strcpy(status_s, "SEL=Go      ");
      buzzedin_s(status_s + 12);
      setlcd(status_s, time_s, 0);
    }
  }
}

void inputBool(char *title, boolean *value) {
  boolean stillSelecting = true;
  char numstr[17];

  if (*value) {
    strcpy(numstr, "On");
  } else {
    strcpy(numstr, "Off");
  }
  setlcd(title, numstr, 1);
  lcd.blink();
  lcd.setCursor(0, 1);
  lcd.cursor();

  while (stillSelecting == true) {
    uint8_t buttons = lcd.readButtons();
    if (buttons) {
      if (buttons & BUTTON_LEFT) {
        stillSelecting = false;
        lcd.noBlink();
        lcd.noCursor();
      }
      if ((buttons & BUTTON_UP) || (buttons & BUTTON_DOWN)) {
        *value = !(*value);
        if (*value) {
          strcpy(numstr, "On");
        } else {
          strcpy(numstr, "Off");
        }

        setlcd(title, numstr, 1);
      }
    }
    delay(F_DEBOUNCE);
  }
}


void make_value_str(char *str, int *val, boolean astime) {

  if (*val == -1) {
    strcpy(str, "Off");
  } else {
    if (astime) {
      time_to_s(str, *val, "");
    } else {
      itoa(*val, str, 10);
    }
  }


}
void inputInt(char *title, int *value, int minval, int maxval, int stepval, boolean as_time) {
  boolean stillSelecting = true;
  char numstr[17];

  make_value_str(numstr, value, as_time);
  setlcd(title, numstr, 1);
  lcd.blink();
  lcd.setCursor(0, 1);
  lcd.cursor();

  while (stillSelecting == true) {
    uint8_t buttons = lcd.readButtons();
    if (buttons) {
      if (buttons & BUTTON_LEFT) {
        stillSelecting = false;
        lcd.noBlink();
      }
      if ((buttons & BUTTON_UP) && (*value < maxval)) {
        *value = *value + stepval;
        make_value_str(numstr, value, as_time);
        setlcd(title, numstr, 1);
        lcd.setCursor(0, 1);
      }
      if ((buttons & BUTTON_DOWN) && (*value > minval)) {
        *value = *value - stepval;
        make_value_str(numstr, value, as_time);

        setlcd(title, numstr, 1);
        lcd.setCursor(0, 1);
      }
    }
    delay(F_DEBOUNCE);

  }
}


void saveConfig() {
  for (unsigned int t = 0; t < sizeof(gconfig); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&gconfig + t));

  Serial.println(F("wrote config"));
}


void handleSetup() {

  byte topItemDisplayed = 0;  // stores menu item displayed at top of LCD screen
  byte cursorPosition = 0;  // where cursor is on screen, from 0 --> totalRows.
  byte totalRows = 2;  // total rows of LCD
  byte totalCols = 16;  // total columns of LCD

  unsigned long timeoutTime = 0;  // this is set and compared to millis to see when the user last did something.
  const int menuTimeout = 10000; // time to timeout in a menu when user doesn't do anything.

  // redraw = 0 - don't redraw
  // redraw = 1 - redraw cursor
  // redraw = 2 - redraw list
  byte redraw = MOVELIST;  // triggers whether menu is redrawn after cursor move.
  byte i = 0; // temp variable for loops.
  byte totalMenuItems = 8;  //a while loop below will set this to the # of menu items

  // Put the menu items here. Remember, the first item will have a 'position' of 0.
  char* menuItems[] = {
  // 0123456789012
    "Max Time",    // maximum amount of time
    "Autonext",    // automatically start the clock after these many seconds after a buzzin
    "Soundset",    // which sound set to use
    "Unique Sounds",  // unique sounds per player or use the default
    "Multibuzz",      // if multiple people can buzz in (this is confusing for game master)
    "Buzz in Pause",  // if you can buzz in while the clock is paused
    "Buzzer Lockout", // if buzzer can only try once per round
    "Beep LastTen",   // if we should beep during last ten seconds.
    "Factory Reset"
  };

  boolean stillSelecting = true;  // set because user is still selecting.

  lcd.clear();  // clear the screen so we can paint the menu.

  timeoutTime = millis() + menuTimeout; // set initial timeout limit.
  while (stillSelecting == true) {
    uint8_t buttons = lcd.readButtons();
    if (buttons) {
      if (buttons & BUTTON_UP) {
        timeoutTime = millis() + menuTimeout;  // reset timeout timer
        //  if cursor is at top and menu is NOT at top
        //  move menu up one.
        if (cursorPosition == 0 && topItemDisplayed > 0)  //  Cursor is at top of LCD, and there are higher menu items still to be displayed.
        {
          topItemDisplayed--;  // move top menu item displayed up one.
          redraw = MOVELIST;  // redraw the entire menu
        }

        // if cursor not at top, move it up one.
        if (cursorPosition > 0)
        {
          cursorPosition--;  // move cursor up one.
          redraw = MOVECURSOR;  // redraw just cursor.
        }

      }

      if (buttons & BUTTON_DOWN) {
        timeoutTime = millis() + menuTimeout;  // reset timeout timer
        // this sees if there are menu items below the bottom of the LCD screen & sees if cursor is at bottom of LCD
        if ((topItemDisplayed + (totalRows - 1)) < totalMenuItems && cursorPosition == (totalRows - 1))
        {
          topItemDisplayed++;  // move menu down one
          redraw = MOVELIST;  // redraw entire menu
        }
        if (cursorPosition < (totalRows - 1)) // cursor is not at bottom of LCD, so move it down one.
        {
          cursorPosition++;  // move cursor down one
          redraw = MOVECURSOR;  // redraw just cursor.
        }
      }

      if (buttons & BUTTON_SELECT) {
        timeoutTime = millis() + menuTimeout; // reset timeout timer
        switch (topItemDisplayed + cursorPosition) // adding these values together = where on menuItems cursor is.
        {
          // put code to be run when specific item is selected in place of the Serial.print filler.
          // the Serial.print code can be removed, but DO NOT change the case & break structure.
          // (Obviously, you should have as many case instances as you do menu items.)
          case 0:  // Max Time
            inputInt(menuItems[0], &gconfig.maxtime, 0, 3600, 30, 1);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 1:  // Autonext
            inputInt(menuItems[1], &gconfig.autonext, -1, 60, 1, 1);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 2:  // Soundset
            inputInt(menuItems[2], &gconfig.soundset, 1, MAX_SOUNDSETS, 1, 0);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 3:  // Unique Sounds
            inputBool(menuItems[3], &gconfig.unique_sounds);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 4:  // Multibuzz
            inputBool(menuItems[4], &gconfig.multibuzz);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 5:  // Buzzpaused
            inputBool(menuItems[5], &gconfig.pausebuzz);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 6:  // Lockout
            inputBool(menuItems[6], &gconfig.roundlockout);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 7:  // Beep during last ten
            inputBool(menuItems[7], &gconfig.lastten);
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.
            break;

          case 8:
            resetConfig();
            saveConfig();
            redraw = MOVELIST;  // redraw entire menu
            buttons = false; // prevent exit.

        }
      }

      if (buttons & BUTTON_LEFT) {
        stillSelecting = false;
        current_state = STATE_NEWGAME;
        show_first_screen();

        Serial.println(F("exit_setup"));
        saveConfig();
        return;
      }
      delay(F_DEBOUNCE);

    }

    switch (redraw) { //  checks if menu should be redrawn at all.
      case MOVECURSOR:  // Only the cursor needs to be moved.
        redraw = false;  // reset flag.
        if (cursorPosition > totalMenuItems) // keeps cursor from moving beyond menu items.
          cursorPosition = totalMenuItems;
        for (i = 0; i < (totalRows); i++) { // loop through all of the lines on the LCD
          lcd.setCursor(0, i);
          lcd.print(" ");                      // and erase the previously displayed cursor
          lcd.setCursor((totalCols - 1), i);
          lcd.print(" ");
        }
        lcd.setCursor(0, cursorPosition);     // go to LCD line where new cursor should be & display it.
        lcd.write(0x7e);
        lcd.setCursor((totalCols - 1), cursorPosition);
        lcd.write(0x7f);
        break;  // MOVECURSOR break.

      case MOVELIST:  // the entire menu needs to be redrawn
        redraw = MOVECURSOR; // redraw cursor after clearing LCD and printing menu.
        lcd.clear(); // clear screen so it can be repainted.
        if (totalMenuItems > ((totalRows - 1))) { // if there are more menu items than LCD rows, then cycle through menu items.
          for (i = 0; i < (totalRows); i++) {
            lcd.setCursor(1, i);
            lcd.print(menuItems[topItemDisplayed + i]);
          }
        }
        else { // if menu has less items than LCD rows, display all available menu items.
          for (i = 0; i < totalMenuItems + 1; i++) {
            lcd.setCursor(1, i);
            lcd.print(menuItems[topItemDisplayed + i]);
          }
        }
        break;  // MOVELIST break
    }

    if (timeoutTime < millis()) { // user hasn't done anything in awhile
      stillSelecting = false;  // tell loop to bail out.
      show_first_screen();
    }
  }
  current_state = STATE_NEWGAME;
}

void update_clock()
{
  char time_s[17];
  char status_s[17];

  /* clock */
  if ( (long)( millis() - nexttick ) >= 0 )
  {
    /* update DISPLAY only we have to for performance */
    if (current_state == STATE_RUNNING) {
      time_to_s(time_s, timeleft, " RUNNING ");
      strcpy(status_s, "SEL=Pause    ");
      buzzedin_s(status_s + 12);
      setlcd(status_s, time_s, 0);
    }

    /* TODO: Handle millis() rollover, or, maybe we don't care because its 50 days in new arduinos */
    nexttick += 1000;  // do it again 1 second later
    if ((timeleft <= 10) && (gconfig.lastten == 1) && (timeleft != 0) && (current_state == STATE_RUNNING)) { 
      set_player_leds(LOW);
      musicPlayer.sineTest(0x44, 250);
      set_player_leds(HIGH);

    }
    if ((timeleft <= 0) && (current_state != STATE_OUTOFTIME)) {
      current_state = STATE_OUTOFTIME;
      playsound(SND_TIMEUP,-1); 
      time_to_s(time_s, timeleft, " TIMEUP");
      setlcd("DOWN=Play Again", time_s, 0);
    }

    if (current_state == STATE_RUNNING) {
      timeleft--;
    }

  }
}

void handleBuzzedIn() {
  char time_s[17];
  char playerstr[17];

  for (int i = 0; i < NUMBUTTONS; i++) {
    if (justpressed[i] == 1) {
      lastplayer = i;
      buzzedin[i] = 1;
    }
  }

  if (millis() % DISPLAYBLINK < (DISPLAYBLINK / 2)) {
    strcpy(playerstr, "*** PLAYER x ***");
    playerstr[11] = (char) 49 + lastplayer;
  } else {
    strcpy(playerstr, "                ");
  }

  if (gconfig.autonext > -1) {
    time_to_s(time_s, ((advanceat - millis()) / 1000), " AUTO");
  } else { 
    time_to_s(time_s, timeleft, " BUZZ");
  }
  setlcd(playerstr, time_s, 0);

}

/* Player LED functions */
void set_player_leds(boolean state) { 
  byte i;
  for (i=0; i < 4; i++) { 
   musicPlayer.GPIO_pinMode(GPIO_PLAYER + i, OUTPUT);
   musicPlayer.GPIO_digitalWrite(GPIO_PLAYER + i, state);
  }
}

void solo_player_led(byte playerid) { 
  set_player_leds(HIGH);
  musicPlayer.GPIO_digitalWrite(GPIO_PLAYER + playerid, LOW);

}

void set_buzzed_in() {
  // Act on check players data.
  for (byte i = 0; i < NUMBUTTONS; i++) {
    if (justpressed[i]) {
      if (gconfig.roundlockout) {
        if (buzzedin[i]) {
          return;
        }
      }
      advanceat = millis() + (gconfig.autonext * 1000);
      current_state = STATE_BUZZED_IN;

      buzzedin[i] = 1;
      solo_player_led(i);
      playsound(SND_BUZZ, i);
    }
  }
}

void display_last_buzz()
{
  // updates the lower right hand corner with the last buzz
  for (byte i = 0; i < NUMBUTTONS; i++) {
    if (justpressed[i]) {
      lcd.setCursor(9, 0);
      lcd.write('P');
      lcd.write((char)49 + i);
    }
  }
}

void fail_last_buzz()
{
  // updates the lower right hand corner with the last buzz
  for (byte i = 0; i < NUMBUTTONS; i++) {
    if (justpressed[i]) {
      playsound(SND_INVALID,-1);
    }
  }
}

void loop() {

  /* Take care of the GM first */
  handle_gm_buttons();

  /* take care of the players */
  switch (current_state) {
    case STATE_NEWGAME:
      // das blinkenlights if we are inbetween rounds!
      solo_player_led((millis()/100) % 4);
      delay(150);
      break; 
    case STATE_SETUP:
    case STATE_INSETUP:
      handleSetup();
      break;
    case STATE_RUNNING:
      check_players();
      set_buzzed_in(); // if any...
      break;
    case STATE_PAUSED:
      check_players();
      if (gconfig.pausebuzz) {
        // if the game is paused and pausebuzz is enabled then we don't
        // change the locked-in state, we just update the status to show who
        // last hit the buzzer.
        display_last_buzz();
      } else  {
        fail_last_buzz();
      }
      break;
    case STATE_BUZZED_IN:
      handleBuzzedIn();

      if (gconfig.multibuzz) {
        check_players();
        set_buzzed_in();
      }

      if (gconfig.autonext > 0) {
        if (millis() > advanceat) {
          current_state = STATE_RUNNING;
          set_player_leds(HIGH); // clear LEDs
          Serial.println(F("autonext release "));
        }
      }
      break;
  }

  update_clock();

}


