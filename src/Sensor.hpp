#pragma once
#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <Arduino.h>

#define SERVER_PORT 12345
#define MyIP_KEY "myip"
#define SERVER_IP_KEY "serverip"
#define GATWAY_IP_KEY "gatewayadr"
#define BOARD_KEY "boardname"

// 子機データ構造
struct SensorData
{
    String id;
    String ip;      // IPアドレス（4バイト）
    String timeStr; // 受信した日時（文字列）
    float temp;
    float hum;
    float pres;
    int IsConnected; // 接続状態
};

#endif