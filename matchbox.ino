#include <Adafruit_DotStar.h>
#include <avr/pgmspace.h>         // needed for PROGMEM
//#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET

#include "pitches.h"

volatile uint32_t toggle_count;
// TrinketTone:
// Generate a square wave on a given frequency & duration
// Call with frequency (in hertz) and duration (in milliseconds).
// Uses Timer1 in CTC mode.  Assumes PB1 already in OUPUT mode.
// Generated tone is non-blocking, so return immediately
// returns while tone is playing.
void TrinketTone (uint16_t frequency, uint32_t duration)
{
  // scan through prescalars to find the best fit
  uint32_t ocr = (F_CPU/frequency) >> 1;
  uint8_t  prescalarBits = 1;
  while (ocr>255)
  {
    ++prescalarBits;
    ocr = ocr >> 1; 
  }

  // Calculate note duration in terms of toggle count
  // Duration will be tracked by timer1 ISR
  toggle_count = frequency * duration / 500;
  
  OCR1C = ocr - 1;   // Set the OCR 
  bitSet(TIMSK, OCIE1A);      // enable interrupt
  TCCR1 = 0x90 | prescalarBits; // CTC mode; toggle OC1A pin; set prescalar
}

// Timer1 Interrupt Service Routine:
// Keeps track of note duration via toggle counter
// When correct time has elapsed, counter is disabled
ISR(TIMER1_COMPA_vect)
{
  if (toggle_count != 0)             // done yet?
    --toggle_count;                  // no, keep counting
  else                               // yes,
    TCCR1 = 0x90;                    // stop the counter
}

//#define FANFARE
//#define CRYSTAL_GEMS
#define GIANT_WOMAN
#include "songs.h"

const uint32_t tone_space = 25; //min delay before next note
uint32_t next_note_delay = 25; //min delay before tone routine runs again

void playNextNote()
{
  static uint32_t length = 20;
  static uint16_t count = 0;

  if (count >= notecount)
    count = 0;

  if(pgm_read_word(&note_pitch[count]) != 0)
    TrinketTone(pgm_read_word(&note_pitch[count]), pgm_read_byte(&note_durs[count])*length);
  next_note_delay = pgm_read_byte(&note_durs[count]) * length + tone_space;

  ++count;
}


#define NUMPIXELS 4 // Number of RGB LEDs

// Here's how to control the LEDs from any two pins:
#define DATAPIN    0
#define CLOCKPIN   2

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

void setup() {
#if (F_CPU == 16000000)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  pinMode(1, OUTPUT);
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

uint8_t brightness = 255; //0-255

void updateLEDs()
{
  static uint8_t next_color[3] = {0,0,0};
  static int8_t color_increment[3] = {7,5,3};

  for(uint8_t i=0; i<3; ++i)
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

  for(uint8_t i=NUMPIXELS-1; i > 0; --i)
    strip.setPixelColor(i, strip.getPixelColor(i-1));
  strip.setPixelColor(0, next_color[1], next_color[0], next_color[2]);
  strip.show();
}

const uint32_t led_interval = 10; //min delay in ms before LED routine runs again

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
