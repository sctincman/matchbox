#include <Adafruit_DotStar.h>
#include <avr/pgmspace.h>         // needed for PROGMEM
#include <SPI.h>

#include "pitches.h"

//#define FANFARE
//#define CRYSTAL_GEMS
#define GIANT_WOMAN
#include "songs.h"

const uint32_t tone_space = 25; //min delay before next note
uint32_t next_note_delay = 25; //min delay before tone routine runs again

#define PIEZO_PIN 12

void playNextNote()
{
  static uint32_t length = 20;
  static uint16_t count = 0;

  if (count >= notecount)
    count = 0;

  noTone(PIEZO_PIN);
  if (pgm_read_word(&note_pitch[count]) != 0)
    tone(PIEZO_PIN, pgm_read_word(&note_pitch[count]), pgm_read_byte(&note_durs[count])*length);
  next_note_delay = pgm_read_byte(&note_durs[count]) * length + tone_space;

  ++count;
}


#define NUMPIXELS 11 // Number of RGB LEDs

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BRG);

void setup() {
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

uint8_t brightness = 128; //0-255

void updateLEDs()
{
  static uint8_t next_color[3] = {0, 0, 0};
  static int8_t color_increment[3] = {7, 5, 3};

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

const uint32_t led_interval = 20; //min delay in ms before LED routine runs again

void loop() {
  static uint32_t previous_millis_led = 0; //when LED routine ran last
  static uint32_t previous_millis_note = 0; //when tone routine ran last

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
}
