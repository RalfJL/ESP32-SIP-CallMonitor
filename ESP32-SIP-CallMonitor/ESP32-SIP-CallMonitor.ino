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
MainLED ML;

char lastCallfrom[256];

void callCallback(const char * from) {
  log_d("Received a call from: %s", from);
  ML.on();
}

void cancelCallback(const char * from){
  log_d("Call canceled from: %s", from);
  ML.off();
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
    aSip.setCancelCallback(cancelCallback);
  }
  // start FastLED
  startFastLED();
}


void loop(void)
{
  // SIP processing incoming packets
  aSip.loop(acSipIn, sizeof(acSipIn)); // handels incoming calls and reregisters

  ML.loop(); // keeps animation alive; and adds a delay

}
