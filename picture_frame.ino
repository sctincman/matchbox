#include <Adafruit_DotStar.h>
//#include <avr/pgmspace.h>         // needed for PROGMEM
//#include <SPI.h>

#define NUMPIXELS 8 // Number of RGB LEDs in the strip

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BRG); //initialize the strip, using hardware SPI

uint8_t brightness = 200; //0-255, scales the intensity of the LEDs
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
#define BUTTON_PIN 4
#define BUTTON_INT 4
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  //attachInterrupt(BUTTON_INT, resetLEDs, FALLING); //interrupt 2 is the corresponding intr for PIN 0..., set to run the interrupt once when changes from HIGH to LOW (aka, will run when button is pressed) 
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
  static uint32_t previous_millis_button = 0; //when button routine ran last

  uint32_t current_millis = millis(); //what is the current time?

  // Has enough time passed to re-run the LED routine? then run and update last-run time
  if (current_millis - previous_millis_led >= led_interval)
  {
    previous_millis_led = millis();
    updateLEDs();
  }

  if (current_millis - previous_millis_button >= button_interval)
  {
    previous_millis_button = millis();
    if (digitalRead(BUTTON_PIN) == LOW)
      reset_leds = !reset_leds;
  }
}
