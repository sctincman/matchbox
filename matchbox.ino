#include <Adafruit_DotStar.h>
#include <avr/pgmspace.h>         // needed for PROGMEM
#include <SPI.h>

#include "pitches.h"

#define FANFARE
//#define CRYSTAL_GEMS
#define GIANT_WOMAN
#include "songs.h"

const uint32_t tone_space = 25; //min delay before next note
uint32_t next_note_delay = 25; //min delay before tone routine runs again
volatile bool alternate_song = false;

#define PIEZO_PIN 12

void playNextNote()
{
  static uint32_t length = 20;
  static uint16_t count = 0;

  if (alternate_song)
  {
    if (count >= fanfare_notecount)
      count = 0;

    noTone(PIEZO_PIN);
    if (pgm_read_word(&fanfare_note_pitch[count]) != 0)
      tone(PIEZO_PIN, pgm_read_word(&fanfare_note_pitch[count]), pgm_read_byte(&fanfare_note_durs[count])*length);
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

  ++count;
}


#define NUMPIXELS 11 // Number of RGB LEDs

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BRG);

uint8_t brightness = 128; //0-255
volatile bool reset_leds = 0;

void updateLEDs()
{
  static uint8_t next_color[3] = {0, 0, 0};
  static int8_t color_increment[3] = {7, 5, 3};

  if (reset_leds)
  {
    for (uint8_t i = 0; i < NUMPIXELS; ++i)
      strip.setPixelColor(i, 0);
      
    reset_leds = false;
    
    strip.show();
    
    return;
  }
  
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

  for (uint8_t i = NUMPIXELS - 1; i > 0; --i)
    strip.setPixelColor(i, strip.getPixelColor(i - 1));
  strip.setPixelColor(0, next_color[1], next_color[0], next_color[2]);
  strip.show();
}

#define BUTTON1_PIN 0
#define BUTTON2_PIN 2
#define BUTTON3_PIN 3

void resetLEDs ()
{
  reset_leds = true;
}

void setup() {
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  attachInterrupt(2, resetLEDs, LOW);

  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
}

const uint32_t led_interval = 20; //min delay in ms before LED routine runs again
const uint32_t button_interval = 20;

void loop() {
  static uint32_t previous_millis_led = 0; //when LED routine ran last
  static uint32_t previous_millis_note = 0; //when tone routine ran last
  static uint32_t previous_millis_button = 0; //when button routine ran last

  uint32_t current_millis = millis();

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
