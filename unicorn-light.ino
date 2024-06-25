
#define FASTLED_FORCE_SOFTWARE_SPI
#define NO_CLOCK_CORRECTION 1
#include <stdint.h>
#include <FastLED.h> //v3.5.0
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define NUM_LEDS 2
CRGB leds[NUM_LEDS];
#define DATA_PIN 0
uint32_t startTime;
uint32_t lowTime = 0;
uint8_t color = 0;

void setup() {
  // Power Saving setup
  for (byte i = 0; i < 6; i++) {
    pinMode(i, INPUT);      // Set all ports as INPUT to save energy
    digitalWrite (i, LOW);  //
  }
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP); //set USB D- to Vcc to lower leakage

  wdt_disable();
  // disable ADC
  ADCSRA = 0;
   // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);  // turn on brown-out enable select
  MCUCR = bit (BODS);        // this must be done within 4 clock cycles of above

  startTime = millis();
}

ISR(PCINT0_vect) {
    // This is called when the interrupt occurs, but we don't need to do anything in it
}

void enterSleep()
{
  lowTime = 0;
  FastLED.showColor(0);
  delay(1000);
  power_all_disable();
  noInterrupts();
  GIMSK |= _BV(PCIE);   // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT2); // Use PB0 (was PB3) as interrupt pin
  GIFR = bit (INTF0) | bit(PCIF);  // clear interrupt flags
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  interrupts();
  sleep_cpu ();              // sleep within 3 clock cycles of above
  //wake path
  sleep_disable();
  PCMSK &= ~_BV(PCINT2); //clear int enable
  GIFR = bit (INTF0) | bit(PCIF);  // clear flag
  power_all_enable();
  startTime = millis();
}

#define MINS(_x)  ((uint32_t)(_x)*1000UL*60UL)

void loop() {
  delay(300);

  if (millis() - startTime > MINS(30))
  {
    enterSleep();
  }

  if (digitalRead(2) == 0)
  {
    if (lowTime == 0)
    {
      lowTime = millis();
    }
    else if (millis() - lowTime > 500)
    {
      enterSleep();
    }
  }
  else
  {
    FastLED.showColor(CHSV(color++,255,255),0xff/2);
    lowTime = 0;
  }
}
