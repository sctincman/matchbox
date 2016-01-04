#include <Adafruit_DotStar.h>
#include <avr/pgmspace.h>         // needed for PROGMEM
#include <SPI.h>

/*
  These defines control which song is included
  Un-comment the one you wish to use as the main song, and comment out the other(s)
  NOTE: For the larger book version, the FF "fanfare" is included as a second song
  and prefixed by "fanfare_"
 */

//#define CRYSTAL_GEMS
#define GIANT_WOMAN

#include "songs.h" // include file with song definitions

const uint32_t tone_space = 25; // min spacing in ms between subsequent notes
uint32_t next_note_delay = 25; // min delay in ms before tone routine runs again
volatile bool alternate_song = false; // flag that determines which song to play

#define PIEZO_PIN 12 // The pulse-width-modulation pin is the piezo buzzer attached to

/*
  This routine will play the next note in the sequence, and set the proper delay before it
  should be called again.
  Note: static variables are initialized once, and retain value between calls.
 */
void playNextNote() 
{
  static uint32_t length = 20; // duration of a 64th note
  static uint16_t count = 0; // the current note to play

  // Are we playing the alternate song? Then read from "fanfare_note_[durs/pitch]"
  // Otherwise, read from the default song
  if (alternate_song)
  {
    if (count >= fanfare_notecount) // have we reached the end of the song? then reset the counter
      count = 0;

    noTone(PIEZO_PIN); // make sure we free the pin to be used again

    // read the current note, if it's not a rest note, then play the desired pitch on the piezo pin for the specified duration
    if (pgm_read_word(&fanfare_note_pitch[count]) != REST)
      tone(PIEZO_PIN, pgm_read_word(&fanfare_note_pitch[count]), pgm_read_byte(&fanfare_note_durs[count]) * length);

    // set the delay before this should be called again by the length of the note, and the spacing between notes
    next_note_delay = pgm_read_byte(&fanfare_note_durs[count]) * length + tone_space;
  }
  else
  {
    if (count >= notecount)
      count = 0;

    noTone(PIEZO_PIN);
    if (pgm_read_word(&note_pitch[count]) != 0)
      tone(PIEZO_PIN, pgm_read_word(&note_pitch[count]), pgm_read_byte(&note_durs[count])*length);
    next_note_delay = pgm_read_byte(&note_durs[count]) * length + tone_space;
  }

  ++count; //increment the counter, so the next note is used in the next call
}


#define NUMPIXELS 11 // Number of RGB LEDs in the strip

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BRG); //initialize the strip, using hardware SPI

uint8_t brightness = 128; //0-255, scales the intensity of the LEDs
volatile bool reset_leds = 0; // flag to determine if LEDs should be blanks

/*
  This procedure will update the LED strip.
  The current sequence it implements here is to push a color onto the strip.
  Each call will move the color on each LED to the next LED.
  The new color pushed onto the strip will be an RGB value, with each channel between
  0 and "brightness"
 */
void updateLEDs()
{
  // an array holding the next color to show [R, G, B]
  static uint8_t next_color[3] = {0, 0, 0};
  // how much to change next_color by [R, G, B]
  static int8_t color_increment[3] = {7, 5, 3};
  
  if (reset_leds)
  {
    //set each pixel to black/off
    for (uint8_t i = 0; i < NUMPIXELS; ++i)
      strip.setPixelColor(i, 0);
      
    reset_leds = false; //make sure to reset this flag
    
    strip.show(); //update the LED strip with new colors
    
    return; //exit the routine
  }

  /* change each channel in next_color
     Seems complicated, but end result is each channel osciallates between 0 and brightness
     The complication is testing for sign and preventing overrun...
   */
  for (uint8_t i = 0; i < 3; ++i)
  {
    uint8_t diff = brightness - next_color[i];
    int8_t inc = color_increment[i];
    if (inc > 0 && diff < inc)
    {
      next_color[i] = brightness;
      color_increment[i] = -inc;
    }
    else if (inc < 0 && -inc >  next_color[i])
    {
      next_color[i] = 0;
      color_increment[i] = -inc;
    }
    else
      next_color[i] += inc;
  }

  //move color down one position on the strip!
  for (uint8_t i = NUMPIXELS - 1; i > 0; --i)
    strip.setPixelColor(i, strip.getPixelColor(i - 1));

  //set the first LED to "next_color"
  strip.setPixelColor(0, next_color[1], next_color[0], next_color[2]);
  strip.show(); // update strip to show new colors
}

// which pins the buttons are attached to
#define BUTTON1_PIN 0
#define BUTTON2_PIN 2
#define BUTTON3_PIN 3

/* Interrupt routine
   We attach this to a button interrupt later, whenever button is pressed, this routine is called
   Simply sets the "reset_leds" flag to true.
   NOTE: When using interrupts, the variables that are read/written to must be declared with "volatile"
 */
void resetLEDs ()
{
  reset_leds = true;
}

/* The Setup routine!
   This is called once upon startup, and a good place to initialize things.
   Here, we just initialize the strip (has it's own startup routine we need to call)
   and also set the pins we're using to the proper mode, and attach interrupts!
 */
void setup() {
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP

  /* Change the first button to "input, with internal pullup"
     This means the pin will be held at HIGH voltage, and we can trigger it by connecting to LOW/"ground"
   */
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  attachInterrupt(2, resetLEDs, FALLING); //interrupt 2 is the corresponding intr for PIN 0..., set to run the interrupt once when changes from HIGH to LOW (aka, will run when button is pressed) 

  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
}

/*
  Constants used when multitasking, the delay before the specific routine should run again
  Refer to https://learn.adafruit.com/multi-tasking-the-arduino-part-1/overview for more in-depth info
 */
const uint32_t led_interval = 20; //min delay in ms before LED routine runs again
const uint32_t button_interval = 20;

/*
  The Loop! This is run after the setup routine, and called continuously.
 */
void loop() {
  static uint32_t previous_millis_led = 0; //when LED routine ran last
  static uint32_t previous_millis_note = 0; //when tone routine ran last
  static uint32_t previous_millis_button = 0; //when button routine ran last

  uint32_t current_millis = millis(); //what is the current time?

  // Has enough time passed to re-run the LED routine? then run and update last-run time
  if (current_millis - previous_millis_led >= led_interval)
  {
    previous_millis_led = millis();
    updateLEDs();
  }

  if (current_millis - previous_millis_note >= next_note_delay)
  {
    previous_millis_note = millis();
    playNextNote();
  }

  if (current_millis - previous_millis_button >= button_interval)
  {
    previous_millis_button = millis();
    if (digitalRead(BUTTON2_PIN) == LOW)
      reset_leds = true;
    else if (digitalRead(BUTTON3_PIN) == LOW)
      alternate_song = true;
    else
      alternate_song = false;
  }
}
