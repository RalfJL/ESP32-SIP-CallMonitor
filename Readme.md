# ESP32-SIP-CallMonitor
A call monitor on ESP32 using SIP ( connect via WiFi to a Fritz!Box)

This sketch connects via WiFi as a SIP telephone e.g. to a Fritz!Box. As soon as the Fritz!Box receives a call for that number, the ESP32 will start a Lightshow on a WS2811 LED Strip.

Please Note: SIP is only for signaling. It can NOT answer calls or play any sound. So this sketch does not answer or hangup on any call. You are very save to answer the call with another telephone.

# Installation
1. Install a SIP Telephone on your Fritz!Box
2. add numbers to that Telephone which you want to receive notifications
3. Add WiFi SSID and passwords and SIP User and Password to the sketch
4. Compile the sketch and push it on the ESP32
5. Call the number and see if the lightshow starts 

Enjoy
