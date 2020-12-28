#include <TFT_eSPI.h>

#ifndef wifi_connection_h
#define wifi_connection_h 1

/*
 *    Default SSID and Passwords are defined below
 *    These can be read from the first two lines of a file, "wioterminal.wificonfig"
 *    on a SD card.
 */

#define WIFI_DEFAULT_SSID     "Wifi Network"
#define WIFI_DEFAULT_PASSWORD "password"

// This is the access point created by the Wio Terminal in config mode
#define CONFIG_AP_SSID        "WioTerminalSetup"

// This is the name of the Art-Net node
#define DEFAULT_NODE_NAME  "Wio Terminal Art-Net"
#define DEFAULT_NODE_ID    "WIO-DMX"

// These are sizes of string buffers
#define WIFI_CONFIG_FILE_MAX_LINE 80
#define SSID_MAX_LENGTH           32
#define NODE_NAME_MAX_LENGTH      64
#define NODE_ID_MAX_LENGTH        18

// property accessors
char* wifi_ssid();
char* wifi_password();
char* node_name();
char* node_id();
int universe();
void setUniverse( int u );

// SD Card

int init_connection_info();
void read_wifi_config();
int processLine(int line_number, char* data);
boolean saveSettings();

// AP & Server

void init_access_point_and_server();
void check_server();

void handleRoot();
void handleNotFound();
void handleForm();

#endif
