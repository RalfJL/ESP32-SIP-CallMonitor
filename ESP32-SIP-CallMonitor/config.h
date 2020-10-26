/*
 * configuration parameters 
 */

#ifndef CONFIG_H
#define CONFIG_H

//
// WiFi parameters
//
// WiFi SSID'S and passwords
// the strongest WiFi station will be used
const char* ssid[] = {"FirstSSID", "SecondSSID", "ThirdSSID"};
const char* pw[] = {"PWSID1", "PWSID2", "PWSID3"};
int ssid_count = sizeof(ssid) / sizeof(ssid[0]);

// Sip parameters
const char *SipIP       = "192.168.178.1";        // IP of the FRITZ!Box
const int   SipPORT     = 5060;                 // SIP port of the FRITZ!Box
const char *SipUSER     = "sipUser";           // SIP-Call username at the FRITZ!Box
const char *SipPW      = "sipUserPW";        // SIP-Call password at the FRITZ!Box
// SIP Registration parameters
const int SipEXPIRES   = 600;                   // registration expires in seconds; renew after SipEXPIRES/2



#endif
