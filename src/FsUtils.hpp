#pragma once
#ifndef FSUTILS_H
#define FSUTILS_H
#include <Arduino.h>
// #include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <Preferences.h>
#include <WiFi.h>
#include "esp_mac.h" // required - exposes esp_mac_type_t values
#include "esp_chip_info.h"
#include "esp_system.h"
#include <ArduinoJson.h>
#include "Sensor.hpp"

using namespace std;

#define PREF_NAMESPACE "settings"

class FsUtils
{
private:
public:
    // Preferences prrference;
    FsUtils();
    ~FsUtils();
    int split(String data, char delimiter, String *dst);

    bool prefClear();
    String getPrefString(String key, String def);
    bool setPrefString(String key, String str);
    bool getPrefIPA(String key, int *adr);

    bool setPrefULong(String key, u64_t v);
    u64_t getPrefULong(String key, u64_t def);

    String getBoardName(String def);
    bool setBoardName(String bn);
    String getDefaultMacAddress();
    String EPS_Status_json();

    String GetSerialReadAll();
    JsonDocument GetJsonData(String tx, bool *ok);
    bool JsonCMDCheck(JsonDocument t, JsonDocument *r);
    bool Wifi_Begin(const char *ssid, const char *password);
};

#endif
