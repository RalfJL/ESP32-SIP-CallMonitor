

#include "myFastLED.h"

CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS2];
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void startFastLED() {
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // Single LED on the TOP
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds2, NUM_LEDS2).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}



//
// MainLED
//
void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for ( int i = 0; i < 8; i++) {
    leds[beatsin16( i + 7, 0, NUM_LEDS - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

typedef void (*SimplePatternList[])();
SimplePatternList animation = { rainbow, rainbowWithGlitter, sinelon, juggle, bpm };


MainLED::MainLED() {
  //animation = { rainbow, rainbowWithGlitter, sinelon, juggle, bpm };
}


void MainLED::on() {
  _call_is_signaled = true;
}

void MainLED::off() {
  _call_is_signaled = false;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED[0].showLeds(0);
}

void MainLED::loop() {
  static bool state_before = false;
  if ( _call_is_signaled ) {
    state_before = true;
    //log_d("Iscalling");
    // continue animation
    //sinelon();
    //bpm();
    //juggle();
    (animation[_animation])();
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
  } else {
    if ( state_before ){
      // turn off LED
      state_before = _call_is_signaled;
      off();
    }
  }
  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}
