/*
  config.h - ESP3D configuration file

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with This code; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

//FEATURES - comment to disable //////////////////////////////////////////////////////////

//WiFi setup station as default, use AP mode first if not done
//Note: need both defined to enable it
//#define STATION_WIFI_SSID "*********"
//#define STATION_WIFI_PASSWORD "*********"
//you can also use a different config file for SSID/password
//this file is ignored by github
#if defined __has_include
#  if __has_include ("myconfig.h")
#    include "myconfig.h"
#  endif
#endif

//SERIAL_COMMAND_FEATURE: allow to send command by serial
#define SERIAL_COMMAND_FEATURE

//AUTHENTICATION_FEATURE: protect pages by login password
//#define AUTHENTICATION_FEATURE

//WIFI_FEATURE : enable WIFI function
#define WIFI_FEATURE

//ETH_FEATURE : enable Ethernet function
//#define ETH_FEATURE

//BLUETOOTH_FEATURE : enable BT Serial function
//#define BLUETOOTH_FEATURE

//HTTP_FEATURE : enable HTTP function
#define HTTP_FEATURE

//TELNET_FEATURE : enable Telnet function
#define TELNET_FEATURE

//WS_DATA_FEATURE: allow to connect serial from Websocket
//#define WS_DATA_FEATURE

//DISPLAY_DEVICE: allow screen output
//OLED_I2C_SSD1306          1
//OLED_I2C_SSDSH1106        2
//TFT_SPI_ILI9341_320X240   3
//TFT_SPI_ILI9488_480X320 4
//#define DISPLAY_DEVICE TFT_SPI_ILI9488_480X320

//UI_TYPE_BASIC      1
//UI_TYPE_ADVANCED   2
#define DISPLAY_UI_TYPE UI_TYPE_ADVANCED

//UI_COLORED         1
//UI_MONOCHROME      2
#define DISPLAY_UI_COLOR UI_COLORED

//BUZZER_DEVICE: allow to connect passive buzzer
//#define BUZZER_DEVICE

#if defined (DISPLAY_DEVICE)
//for ILI9143 edit User_Setup.h of TFT_eSPI library
#if (DISPLAY_DEVICE == OLED_I2C_SSD1306) || (DISPLAY_DEVICE == OLED_I2C_SSDSH1106)
#define DISPLAY_I2C_PIN_SDA         4
#define DISPLAY_I2C_PIN_SCL         15
#define DISPLAY_I2C_PIN_RST         16 //comment if not applicable
#define DISPLAY_I2C_ADDR            0x3c
#endif //(DISPLAY_DEVICE == OLED_I2C_SSD1306) || (DISPLAY_DEVICE == OLED_I2C_SSDSH1106)
#define DISPLAY_FLIP_VERTICALY      1 //comment to disable
#if (DISPLAY_DEVICE == TFT_SPI_ILI9341_320X240) || (DISPLAY_DEVICE == TFT_SPI_ILI9488_480X320)
#define DISPLAY_TOUCH_DRIVER        XPT2046_SPI
#define DISPLAY_LED_PIN             33  //-1 if none
#endif //(DISPLAY_DEVICE == TFT_SPI_ILI9341_320X240) || (DISPLAY_DEVICE == TFT_SPI_ILI9488_480X320)
#endif //DISPLAY_DEVICE

//INPUT_DEVICE: allow input
//ROTARY_ENCODER        1
//#define INPUT_DEVICE ROTARY_ENCODER

//DHT_DEVICE: send update of temperature / humidity based on DHT 11/22
//#define DHT_DEVICE

#ifdef BUZZER_DEVICE
#define ESP3D_BUZZER_PIN 15
#endif //BUZZER_DEVICE

#ifdef DHT_DEVICE
#define ESP3D_DHT_PIN 22
//USE_CELSIUS
//USE_FAHRENHEIT
#define DHT_UNIT USE_CELSIUS
#endif //DHT_DEVICE

//PIN_RESET_FEATURE : allow to reset settings by setting low a pin
//#define PIN_RESET_FEATURE
#if defined (PIN_RESET_FEATURE)
#define ESP3D_RESET_PIN 2
#endif //PIN_RESET_FEATURE

//SD_DEVICE: to access SD Card files directly instead of access by serial using printer Board FW
//ESP_SD_NATIVE               1 //esp32 / esp8266
//ESP_SDIO                    2 //esp32 only
//ESP_SDFAT                   3 //esp8266 (same as native) / esp32
//#define SD_DEVICE    ESP_SDFAT

//pin if reader has insert detection feature
//let -1 or comment if none
#define ESP_SD_DETECT_PIN       -1
//value expected for ESP_SD_DETECT_PIN
#define ESP_SD_DETECT_VALUE      1

//FILESYSTEM_FEATURE: to host some files on flash
//ESP_SPIFFS_FILESYSTEM       0
//ESP_FAT_FILESYSTEM          1
//ESP_LITTLEFS_FILESYSTEM     2
#define FILESYSTEM_FEATURE ESP_SPIFFS_FILESYSTEM

//Allows to mount /FS and /SD under / for FTP server
#define GLOBAL_FILESYSTEM_FEATURE

//FTP_FEATURE : enable FTP feature
//FS_ROOT        mount all FS
//FS_FLASH       mount Flash FS
//FS_SD          mount SD FS
//FS_USBDISK     mount USB disk FS
#define FTP_FEATURE  FS_ROOT

//DIRECT_PIN_FEATURE: allow to access pin using ESP201 command
#define DIRECT_PIN_FEATURE

//TIMESTAMP_FEATURE: set time system
#define TIMESTAMP_FEATURE

//FILESYSTEM_TIMESTAMP_FEATURE: display last write time from Flash files
#define FILESYSTEM_TIMESTAMP_FEATURE

//FILESYSTEM_TIMESTAMP_FEATURE:display last write time from SD files
//#define SD_TIMESTAMP_FEATURE

//MDNS_FEATURE: this feature allow  type the name defined
//in web browser by default: http:\\esp8266.local and connect
//need `bonjour` protocol on windows
#define MDNS_FEATURE

//SSDP_FEATURE: this feature is a discovery protocol, supported on Windows out of the box
#define SSDP_FEATURE

//CAPTIVE_PORTAL_FEATURE: In SoftAP redirect all unknow call to main page
#define CAPTIVE_PORTAL_FEATURE

//OTA_FEATURE: this feature is arduino update over the air
//#define OTA_FEATURE

//WEB_UPDATE_FEATURE: allow to flash fw using web UI
#define WEB_UPDATE_FEATURE

//NOTIFICATION_FEATURE : allow to push notifications
#define NOTIFICATION_FEATURE

//For ESP8266 Only, it define which secure client to use AXTls or BearSSL
//#define USING_AXTLS

//if not using AXTLS need to decrease size of packet to not be OOM
#define BEARSSL_MFLN_SIZE   512
#define BEARSSL_MFLN_SIZE_FALLBACK  4096

//CAMERA_DEVICE: Enable the support of connected camera
//CAMERA_MODEL_CUSTOM           0 //Edit the pins in include/pins.h
//CAMERA_MODEL_ESP_EYE          1
//CAMERA_MODEL_M5STACK_PSRAM    2
//CAMERA_MODEL_M5STACK_WIDE     3
//CAMERA_MODEL_AI_THINKER       4 e.g. used by ESP32-CAM
//CAMERA_MODEL_WROVER_KIT       5
//#define CAMERA_DEVICE CAMERA_MODEL_AI_THINKER
//#define CAMERA_DEVICE_FLIP_VERTICALY  //comment to disable
//#define CAMERA_DEVICE_FLIP_HORIZONTALY//comment to disable
#define CUSTOM_CAMERA_NAME "ESP32-CAM"

//Allow remote access by enabling cross origin access
//check https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS
//this should be enabled only in specific cases
//like show the camera in web page different than device web server
//if you do not know what is that then better left it commented
//#define ESP_ACCESS_CONTROL_ALLOW_ORIGIN

//ESP_GCODE_HOST_FEATURE : allow to send GCODE with ack
#define ESP_GCODE_HOST_FEATURE

//ESP_AUTOSTART_SCRIPT : to do some actions / send GCODE at start, need ESP_GCODE_HOST_FEATURE enabled
//can be  a line od several GCODES separated by `\n` e.g. "M21\nM117 SD mounted\n"
//can be  a file name, if exists, commands inside will be processed, e.g "/FS:/autostart.esp"
//#define ESP_AUTOSTART_SCRIPT "M117 Mouning SD\nM21\n"

//ESP_LUA_INTERPRETER_FEATURE : add lua scripting feature
//#define ESP_LUA_INTERPRETER_FEATURE

//Extra features /////////////////////////////////////////////////////////////////////////
/************************************
 *
 * DEBUG
 *
 * **********************************/
//Do not do this when connected to printer !!!
//be noted all upload may failed if enabled
//DEBUG_OUTPUT_SERIAL0 1
//DEBUG_OUTPUT_SERIAL1 2
//DEBUG_OUTPUT_SERIAL2 3
//DEBUG_OUTPUT_TELNET  4
//DEBUG_OUTPUT_WEBSOCKET  5
//#define ESP_DEBUG_FEATURE DEBUG_OUTPUT_SERIAL0

#ifdef ESP_DEBUG_FEATURE
#define DEBUG_ESP3D_OUTPUT_PORT  8000
#endif //ESP_DEBUG_FEATURE

#if defined (DISPLAY_DEVICE) && (DISPLAY_UI_TYPE == UI_TYPE_ADVANCED)
//allows to use [ESP216]SNAP to do screen capture
#define DISPLAY_SNAPSHOT_FEATURE
#define AUTO_SNAPSHOT_FEATURE
#endif //DISPLAY_DEVICE

/************************************
 *
 * Serial Communications
 *
 * **********************************/
//which serial ESP use to communicate to printer (ESP32 has 3 serials available, ESP8266 only 2)
//USE_SERIAL_0 for ESP8266/32
//USE_SERIAL_1 for ESP8266/32
//USE_SERIAL_2 for ESP32 Only
#define ESP_SERIAL_OUTPUT USE_SERIAL_0

//Serial rx buffer size is 256 but can be extended
#define SERIAL_RX_BUFFER_SIZE 512

/************************************
 *
 * Settings
 *
 * **********************************/
//SETTINGS_IN_EEPROM 0
//SETTINGS_IN_PREFERENCES 1
#define ESP_SAVE_SETTINGS SETTINGS_IN_EEPROM

/************************************
 *
 * Customize ESP3D
 *
 * **********************************/
#if defined( ARDUINO_ARCH_ESP8266)
#define ESP_MODEL_NAME "ESP8266"
#define ESP_MODEL_URL "http://espressif.com/en/products/esp8266/"
#endif //ARDUINO_ARCH_ESP8266
#if defined( ARDUINO_ARCH_ESP32)
#define ESP_MODEL_NAME "ESP32"
#define ESP_MODEL_URL "https://www.espressif.com/en/products/hardware/esp-wroom-32/overview"
#endif //ARDUINO_ARCH_ESP32
#define ESP_MODEL_NUMBER "ESP3D 3.0"
#define ESP_MANUFACTURER_NAME "Espressif Systems"
#define ESP_MANUFACTURER_URL "http://espressif.com"

#define NOTIFICATION_ESP_ONLINE "Hi, %ESP_NAME% is now online at %ESP_IP%"
#define ESP_NOTIFICATION_TITLE "ESP3D Notification"

#endif //_CONFIGURATION_H
