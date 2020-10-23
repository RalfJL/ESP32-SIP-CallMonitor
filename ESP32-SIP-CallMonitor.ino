/*
    Optical ring on incoming calls.
    This sketch registers itself as a SIP telefon.
    An incoming call will result in a light show done by a WS2811

     Copyright (c) 2017 Ralf Lehmann. All rights reserved.

   This file is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   10.2020 initial version
*/

#include "config.h"

#include "espWIFI.h"

#include "ArduinoSIP.h"
char acSipIn[2048];
char acSipOut[2048];
Sip aSip(acSipOut, sizeof(acSipOut));

#include "myFastLED.h"

// for periodic refresh of the registration
#include <Ticker.h>
Ticker reregister;

void _reregister() {
  if (checkWiFi()) {
    aSip.Subscribe();
  }
}

time_t lastRegis = 0;                           // for registration refresh


#define NewCallTimeout 40
time_t ledRuntime;
time_t ledLastRun;
time_t newCall = 0;
char lastCallfrom[256];

void callCallback(const char * from) {
  log_d("Do a call callback from: %s", from);
  // check if it is a new call or if the NewCallTimeout has passed
  if ( !newCall || (millis() - newCall > 1000 * NewCallTimeout)) {
    log_d("Call timeout has passed: %s", from);
    strncpy(lastCallfrom, from, 256);
    ledRuntime = 20;
    ledLastRun = newCall = millis();
    return;
  }
  if ( strncmp(lastCallfrom, from, 256)) {
    log_d("New call from: %s", from);
    strncpy(lastCallfrom, from, 256);
    ledRuntime = 20; // run for 10 seconds
    ledLastRun = newCall = millis();
  }
}



void setup()
{
  Serial.begin(115200);

  // start WiFi
  if ( startWiFi() ) {
    log_d("WiFi connected; starting SIP");
    aSip.Init(SipIP, SipPORT, WiFiIP, SipPORT, SipUSER, SipPW, SipEXPIRES, 15);
    aSip.Subscribe(); // register to receive call notification
    aSip.setCallCallback(callCallback);
  }
  // start FastLED
  startFastLED();
  reregister.attach(SipEXPIRES / 2, _reregister);
}


void loop(void)
{
  //  // Reregister to SIP telefon
  //  if ( (millis() - lastRegis) / 1000 > SipEXPIRES / 2) {
  //    lastRegis = millis();
  //    aSip.Subscribe();
  //  }
  // SIP processing incoming packets
  aSip.Processing(acSipIn, sizeof(acSipIn));

  // if we received a call notification we start the lightshow
  if ( ledRuntime) {
    if ( millis() - ledLastRun > 1000) {
      ledLastRun = millis();
      ledRuntime--;
    }
    //sinelon();
    //bpm();
    juggle();
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  } else {
    FastLED.clear();
    FastLED.show();
  }
}
