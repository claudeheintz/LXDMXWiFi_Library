#include "wifi_connection_info.h"
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>

// properties
int sd_status = 0;
char ssid[SSID_MAX_LENGTH];
char password[SSID_MAX_LENGTH];
char nodename[NODE_NAME_MAX_LENGTH];
char nodeid[NODE_ID_MAX_LENGTH];
int _universe = 0;

WebServer server(80);

// ******************* accessors *******************

char* wifi_ssid() {
  return ssid;
}

char* wifi_password() {
  return password;
}

char* node_name() {
  return nodename;
}

char* node_id() {
  return nodeid;
}

int universe() {
  return _universe;
}

void setUniverse( int u ) {
  _universe = u;
}

/******************* SD Card Config File Functions *******************/

/*
 * init_sdcard
 *     -initialize the SD Card interface
 *     -read the "wioterminal.wificonfig" file if it can be opened
 *     -return number of lines in the file if successful
 *     -return -1 if can't read file
 *     -return -2 if SD card cannot be initialized
 */
int init_connection_info() {
  nodename[0] = 0;            //default to 0 length
  if ( sd_status == 0 ) {
    if ( SD.begin(SDCARD_SS_PIN, SDCARD_SPI) ) {
      read_wifi_config();
    } else {
      sd_status = -2;
    }
  }
  if ( sd_status < 0 ) {
    // fill in default values
    strncpy( ssid,     WIFI_DEFAULT_SSID,     SSID_MAX_LENGTH      );
    strncpy( password, WIFI_DEFAULT_PASSWORD, SSID_MAX_LENGTH      );
    strncpy( nodename, DEFAULT_NODE_NAME,     NODE_NAME_MAX_LENGTH );
    strncpy( nodeid  , DEFAULT_NODE_ID,       NODE_ID_MAX_LENGTH   );
  }
  return sd_status;
}

/*
 * read_wifi_config reads the contents of wioterminal.wificonfig into wifi_config_buffer
 *    -assumes that file contains one string per line
 *    -converts newlines into string termination
 *    -ignores carriage returns
 *    -sets sd_status to -1 if error
 *    -otherwise sd_status will have positive length of buffer
 */

void read_wifi_config() {
  File myFile = SD.open("wioterminal.wificonfig", FILE_READ);
  if ( myFile ) {
    uint8_t j = 0;
    uint8_t ln = 1;
    char wifi_config_buffer[WIFI_CONFIG_FILE_MAX_LINE];
    while (myFile.available()) {
      wifi_config_buffer[j] = myFile.read();
      if ( wifi_config_buffer[j] == '\n' ) {
        wifi_config_buffer[j] = 0;                     // replace line end with string termination
        ln = processLine(ln, wifi_config_buffer);      // extract current string
        j = 0;                                         // reset index
      } else if ( wifi_config_buffer[j] != '\r' ) {    // ignore carriage returns
        j++;
      }
      if ( j >= WIFI_CONFIG_FILE_MAX_LINE ) {
        #ifdef ENABLE_DEBUG_PRINT
        Serial.print("File line ");
        Serial.print(ln);
        Serial.println(" is too long.");
        #endif
        sd_status = -1;
        return;
      }
    }
    myFile.close();
    if ( j > 0 ) {
        wifi_config_buffer[j] = 0;  // insure string termination if no final \n
        ln = processLine(ln, wifi_config_buffer);
    }

    sd_status = ln;
    
  } else {  // can't open file
    
    #ifdef ENABLE_DEBUG_PRINT
    Serial.println("Could not open wioterminal.wificonfig");
    #endif
    sd_status = -1;
  }
}

int processLine(int line_number, char* data) {
  if ( line_number == 1) {
    strncpy(ssid, data, SSID_MAX_LENGTH);
    return 2;
  }
  if ( line_number == 2) {
    strncpy(password, data, SSID_MAX_LENGTH);
    return 3;
  }
  if ( line_number == 3) {
    strncpy(nodename, data, NODE_NAME_MAX_LENGTH);
    return 4;
  }
  if ( line_number == 4) {
    strncpy(nodeid, data, NODE_ID_MAX_LENGTH);
    return 5;
  }
  if ( line_number == 5) {
    setUniverse( atoi(data) );
    return 6;
  }

  return line_number;
}


boolean saveSettings() {
  File myFile = SD.open("wioterminal.wificonfig", FILE_WRITE);
    if ( myFile ) {
      myFile.println(ssid);
      myFile.println(password);
      myFile.println(nodename);
      myFile.println(nodeid);
      myFile.println(_universe);
      myFile.close();
      return true;
    }
  return false;
}

void init_access_point_and_server() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(CONFIG_AP_SSID);

  server.on("/", handleRoot);
  server.on("/form", handleForm);
  server.onNotFound(handleNotFound);
  server.begin();
}

void check_server() {
  server.handleClient();
}

void handleRoot() {
  char temp[800];

  snprintf(temp, 800,
"<html>\
  <head>\
    <title>Wio Terminal Wifi Setup</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Wio Terminal WiFi Setup</h1>\
    <FORM action='/form' method=post>\
    <p>Your Network&apos;s Name (SSID):</p>\
    <INPUT name='ssid' size=40 value='%s'>\
    <p>Password:</p>\
    <INPUT name='password' type='password' size=40 value='%s'>\
    <p>Node Name:</p>\
    <INPUT name='nodename' size=40 value='%s'>\
    <p>Node ID:</p>\
    <INPUT name='nodeid' size=10 value='%s'>\
    <p>Universe (1-255):</p>\
    <INPUT name='universe' size=10 value='%i'>\
    <p> </p>\
    <INPUT type='submit' name='submitconf' value='Submit'>\
    </FORM>\
  </body>\
</html>",
  ssid, password, nodename, nodeid, (_universe + 1) );
  
  server.send(200, "text/html", temp);
}

void handleNotFound() {
  String message = "404 Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleForm() {
  if (server.method() == HTTP_POST) {
    strncpy(ssid, server.arg("ssid").c_str(), SSID_MAX_LENGTH);\
    strncpy(password, server.arg("password").c_str(), SSID_MAX_LENGTH);
    strncpy(nodename, server.arg("nodename").c_str(), NODE_NAME_MAX_LENGTH);
    strncpy(nodeid, server.arg("nodeid").c_str(), NODE_ID_MAX_LENGTH);
    setUniverse( server.arg("universe").toInt() - 1 );    // universe is zero based UI is 1 based
    if ( _universe < 0 ) {
      _universe = 0;
    } else if ( _universe > 255 ) {
      _universe = 255;
    }

    const char* result;
    if ( saveSettings() ) {
      result = "Saved!";
    } else {
      result = "file error";
    }

    char temp[600];

    snprintf(temp, 600,
    
"<html>\
  <head>\
    <meta http-equiv='refresh' content='1;url=/'/>\
    <title>Wio Terminal Wifi Setup</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>%s</h1>\
  </body>\
</html>",
result);
  
    server.send(200, "text/html", temp);
  } // <- method == POST
}
