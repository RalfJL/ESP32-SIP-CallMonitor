/*
   FastLED code
*/

#ifndef MYFASTLED_H
#define MYFASTLED_H

#include <Ticker.h>


#include <FastLED.h>

//#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
//#warning "Requires FastLED 3.1 or later; check github for latest code."
//#endif

FASTLED_USING_NAMESPACE


#define DATA_PIN    12
#define DATA_PIN2   13
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    27
#define NUM_LEDS2    1


#define BRIGHTNESS          30
#define FRAMES_PER_SECOND  120

extern void startFastLED();



class MainLED {
  public:
    MainLED();
    void on();
    void off();
    void loop();

  private:
    int _animation = 3;
    char _from[256];
    time_t _last_call; // last time we signalled a call
    time_t _same_call_timeout = 80;
    int _signaling_duration = 20;
    bool _call_is_signaled;

    void addGlitter( fract8 chanceOfGlitter);
    void rainbow();
    void rainbowWithGlitter();
    void sinelon();
    void bpm();
    void juggle();


    //SimplePatternList animation;

};


#endif
