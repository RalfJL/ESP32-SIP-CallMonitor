/*
   WiFi code
*/

#ifndef ESPWIFIF_H
#define ESPWIFI_H

#include <WiFi.h>
#include <WiFiMulti.h>

char WiFiIP[16];

WiFiMulti WiFiMulti;

boolean startWiFi() {
  log_d("Sizeof ssid: %d", ssid_count);
  for (int i = 0; i < ssid_count; i++) {
    WiFiMulti.addAP(ssid[i], pw[i]);
  }
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    log_d("WiFi connected");
    WiFi.setAutoConnect (true);
    WiFi.setAutoReconnect (true);
    WiFi.softAPdisconnect (true);
    String ip = WiFi.localIP().toString();
    ip.toCharArray(WiFiIP, 16);
    return true;
  }
  log_d("WiFi could not be started");
  return false;
}

boolean checkWiFi() {
  if ( WiFi.status() != WL_CONNECTED ) {
    return startWiFi();
  }
  return true;
}

#endif
