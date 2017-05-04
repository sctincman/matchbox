#include <Adafruit_DotStar.h>
#include <avr/pgmspace.h>         // needed for PROGMEM
#include <avr/power.h> // ENABLE THIS LINE FOR GEMMA OR TRINKET

#define NUMPIXELS 8

// Here's how to control the LEDs from any two pins:
#define DATAPIN    0
#define CLOCKPIN   2

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

void setup() {
#if (F_CPU == 16000000)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  pinMode(4, OUTPUT);
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

uint8_t brightness = 255; //0-255
volatile uint8_t program = 1;

void nextProgram() {
  program = ++program % 4;
}

void updateLEDs()
{
  static uint8_t next_color[3] = {0,0,0};
  static int8_t color_increment[3] = {7,5,3};

  if (program == 0)
  {
    //set each pixel to black/off
    for (uint8_t i = 0; i < NUMPIXELS; ++i)
      strip.setPixelColor(i, 0);
    
    strip.show(); //update the LED strip with new colors
    
    return; //exit the routine
  }

  brightness = 255 / program;

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

const uint32_t led_interval = 10;
const uint32_t button_interval = 10;

void loop() {
  static uint32_t previous_millis_led = 0;
  static uint32_t previous_millis_button = 0;

  uint32_t current_millis = millis();

  if (current_millis - previous_millis_led >= led_interval)
  {
    previous_millis_led = millis();
    updateLEDs();
  }

  if (current_millis - previous_millis_button >= button_interval)
  {
    previous_millis_button = millis();
    nextProgram();
  }
}
