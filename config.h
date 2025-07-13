#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ETH.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

struct Config {
  uint16_t tcp_port;
  uint16_t telnet_port;
  long baud_rate;
  String api_url;
  bool ssl_enabled;
  bool aes_enabled;
  String device_name;
  String oempark_address; // Direccion del modulo OEMPARK (ej: "02")
};

extern Config config;

void loadConfiguration();
void saveConfiguration();
void setupEthernet();

#endif