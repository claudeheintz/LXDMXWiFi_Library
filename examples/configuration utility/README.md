# ESP-DMX Configuration Utility

The examples, ESP-DMX, WiFi2DMX and Mkr1000-WiFiDMX include the ability to remotely set WiFi connection and DMX protocol parameters.  The way that is accomplished is by sending the ESP8266 or Mkr1000 running the sketch a special packet containing the config data.  This is sent using the Art-Net or sACN network connection and is received using that post.  It is not, however, an Art-Net or E1.31 packet.  Non-protocol packets are ignored so it is convenient to send the ESP-DMX packet to the connection, eliminating the need to open and check another port.

ESP-DMX packets have three operations query, data and upload.  A query packet is sent, much like an Art-Net poll, to ask the receiving device to reply with a data packet containing its config settings.  If broadcast/multicast, the query packet can provoke replies from multiple devices on the network.  The third type of message should be sent only to a specific device's address.  It contains config settings that the ESP8266 copies to its EEPROM memory (or Mkr1000 into flash).

After receiving an upload changing the WiFi connection settings, it is necessary to re-start the device running the sketch.

In order to guarantee that a device set to log into a specific WiFi network can be configured even if that network is unavailable, the sketch checks to see if pin 16 is low when it starts up.  If this is the case, it uses its default access point WiFi connection settings.  That way, a computer can connect and remotely configure the device.

There are two versions of the configuration utility, one in python and one in Java.  The Java version is provided as source code, a jar file and contained within application wrappers for Windows and OS X.