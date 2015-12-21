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

#define MAX_NOTES 8
const uint32_t arp_duration = 10; //duration of arpeggio note in ms
class SoundMixer
{
  private:
    uint16_t notes[MAX_NOTES];
    uint16_t duration[MAX_NOTES];
    uint8_t number_of_notes;
    uint8_t current_note;
    uint32_t previous_millis;
    bool playing;
    
  public:
    SoundMixer(): number_of_notes(0), playing(false), current_note(0), previous_millis(0)
    {
      
    }

    int8_t playNote(uint16_t frequency, uint16_t length)
    {
      if (number_of_notes >= MAX_NOTES)
        return -1;

      //check for duplicates, keep longer duration
      //break once you find larger freq
      // this is the index of where to insert to maintain order
      uint8_t i;
      for(i=0; i<number_of_notes; ++i)
      {
        if(notes[i] == frequency)
        {
          if (length > duration[i])
            duration[i] = length;
          return 1;
        }
        else if( notes[i] < frequency )
          break;
      }

      //i is now the note's new index, shift the rest as needed
      for(uint8_t j=number_of_notes; j>i; --j)
      {
        notes[j] = notes[j-1];
        duration[j] = duration[j-1];
      }
      notes[i] = frequency;
      duration[i] = length;
      ++number_of_notes;

      return 0;
    }

    void play()
    {

      if (number_of_notes == 0)
      {
        playing = false;
        return;
      }
      uint32_t tone_duration = millis() - previous_millis;

      if (duration[current_note] <= tone_duration)
        removeNote(current_note);
      else
      {
        duration[current_note] -= tone_duration;
        ++current_note;
      }

      if (current_note >= number_of_notes)
        current_note = 0;
        
      TrinketTone(notes[current_note], arp_duration);

      previous_millis = millis();
      playing = true;
    }

    void removeNote(uint8_t index)
    {
      if( index < 0 || index >= number_of_notes)
        return;

      if (index < number_of_notes-1) //if last note, don't do anything (also bounds check the for loop)
      {
        for (uint8_t i=index+1; i<number_of_notes; ++i)
        {
          notes[i-1] = notes[i];
          duration[i-1] = duration[i];
        }
      }
      --number_of_notes;
    }
};

SoundMixer mixer = SoundMixer();

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
    mixer.playNote(pgm_read_word(&note_pitch[count]), pgm_read_byte(&note_durs[count])*length);
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
  pinMode(3, INPUT_PULLUP);

  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

void updateLEDs()
{
  static uint32_t next_color = 0x000000;
  static uint8_t color_increment = 4;
  next_color += color_increment;
  for(int i=NUMPIXELS-1; i > 0; --i)
    strip.setPixelColor(i, strip.getPixelColor(i-1));
  strip.setPixelColor(0, next_color);
  strip.show();
}

const uint32_t led_interval = 10; //min delay in ms before LED routine runs again

void loop() {
  static uint32_t previous_millis_led = 0; //when LED routine ran last
  static uint32_t previous_millis_mixer = 0; //when mixer routine ran last
  static uint32_t previous_millis_note = 0; //when tone routine ran last

  uint32_t current_millis = millis();

  if (current_millis - previous_millis_led >= led_interval)
  {
    previous_millis_led = millis();
    updateLEDs();
  }

  if (current_millis - previous_millis_mixer >= arp_duration)
  {
    previous_millis_mixer = millis();
    mixer.play();
  }

  if (current_millis - previous_millis_note >= next_note_delay)
  {
    previous_millis_note = millis();
    if(digitalRead(3) == LOW  && toggle_count == 0)
      playNextNote();
  }
}
