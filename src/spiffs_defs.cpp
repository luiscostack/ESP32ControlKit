#include <config.h>
#include <spiffs_defs.h>

void initSPIFFS() {         
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}