#define SENSOR_ALIVE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
// #include "time.h"

#ifdef SENSOR_ALIVE
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#endif

#ifdef BOARDSEED_XIAO_ESP32S3
#include "LGFX_XIAO_ESP32S3_SPI_ST7789B.hpp"
#else
#include "LGFX_esp32_s3_dev_SPI_ST7789.hpp"
#endif
#include "FsUtils.hpp"
#include "Sensor.hpp"

// クライアントの最大数
#define MAX_CLIENTS 6

// 画面サイズ
#define SCR_WIDTH 240
#define SCR_HEIGHT 320
#define SCR_HEAD_HEIGHT 60
#define SCR_LINE_HEIGHT 42

#define CHILDREN_KEY "children"
#define SCAN_TIMING_KEY "scantiming"

#define BTN_PIN 43

#ifdef BOARDSEED_XIAO_ESP32S3
LGFX_XIAO_ESP32S3_SPI_ST7789B display;
#else
LGFX_esp32_s3_dev_SPI_ST7789 display;
#endif

extern const lgfx::U8g2font lgfxJapanGothic_8F;
extern const lgfx::U8g2font lgfxJapanGothic_12F;
extern const lgfx::U8g2font lgfxJapanGothic_16F;
extern const lgfx::U8g2font lgfxJapanGothic_24F;
extern const lgfx::U8g2font lgfxJapanGothic_28F;
extern const lgfx::U8g2font lgfxJapanGothic_36F;
// 画面描画用の裏画面
lgfx::LGFX_Sprite headbuf(&display);
lgfx::LGFX_Sprite scrbuf(&display);

FsUtils fsu;

#ifdef SENSOR_ALIVE
Adafruit_BMP280 bmp; // I2c 0x77
Adafruit_AHTX0 aht;  // I2c 0x38
#endif

String boardName = "";
String myIPadr = "";
int MyIP[] = {0, 0, 0, 0};      // 固定IPアドレス
int GatewayIP[] = {0, 0, 0, 0}; // 固定IPアドレス

SensorData children[MAX_CLIENTS]; // クライアントのセンサーデータ
SensorData OwnTemp;

bool onled = true;
unsigned long nextTime = 0;
unsigned long blinkTime = 0;
unsigned long screenSaveTime = 0;

bool isScreenSaver = false;

unsigned long scan_timing = 15000;
unsigned long screenSaver_timing = 30000;

int scanIndex = 0;
#ifdef BOARDSEED_XIAO_ESP32S3
#define LEDPIN 21
#else
#define LEDPIN 48
#endif

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
void blinkLED()
{
  if (onled)
  {
    digitalWrite(LEDPIN, LOW);
    onled = false;
  }
  else
  {
    digitalWrite(LEDPIN, HIGH);
    onled = true;
  }
}
// 画面の消去
void DisplayClear()
{
  display.fillScreen(TFT_BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(TFT_WHITE);
  display.setBrightness(80);
}
// 画面へメッセージ
void DisplayPrint(String s)
{
  DisplayClear();
  display.setBrightness(100);
  display.println(s);
  // 10秒表示
}
// ==== ディスプレイ初期化 ====
void setupDisplay()
{
  // 裏画面作成
  headbuf.createSprite(SCR_WIDTH, SCR_HEAD_HEIGHT);
  headbuf.setFont(&lgfxJapanGothic_16F);
  scrbuf.createSprite(SCR_WIDTH, SCR_LINE_HEIGHT);
  scrbuf.setFont(&lgfxJapanGothic_28F);

  display.init();
  display.setRotation(0);
  display.setFont(&lgfxJapanGothic_16F);

  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.println("Booting...");
}
//
// ----------------------------------------------------------------------
String getChildrenIPList()
{
  static String ret = "";
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (ret != "")
    {
      ret += ",";
    }
    ret += children[i].ip;
  }
  return ret;
}
void setChildrenIPList(String ipList)
{
  String cl[MAX_CLIENTS];
  int cnt = fsu.split(ipList, ',', cl);
  if (cnt >= MAX_CLIENTS)
  {
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (children[i].ip != cl[i])
      {
        children[i].id = "";
        children[i].timeStr = "";
        children[i].temp = 0.0;
        children[i].hum = 0.0;
        children[i].pres = 0.0;
        children[i].IsConnected = 0;
      }
      children[i].ip = cl[i];
    }
  }
  else
  {
    Serial.println("ERROR: setChildrenIPList() - not enough IPs");
  }
}
// ----------------------------------------------------------------------
void saveChildren()
{
  String s = "";
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (s != "")
    {
      s += ",";
    }
    s += children[i].ip;
  }
  if (fsu.setPrefString(CHILDREN_KEY, s))
  {
    Serial.println("Save Children OK!");
  }
  else
  {
    Serial.println("Save Children ERROR!");
  }
}
void loadChildren()
{

  String s = fsu.getPrefString(CHILDREN_KEY, "");
  if (s == "")
  {
    // saveChildren();
    return;
  }
  String ss[10];
  int idx = fsu.split(s, ',', ss);
  if (idx >= MAX_CLIENTS)
  {
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      children[i].ip = ss[i];
    }
  }
}
// ----------------------------------------------------------------------
void loadPref()
{
  DisplayPrint("Load Preferences...");
  boardName = fsu.getBoardName("");
  if (boardName == "")
  {
    boardName = fsu.getDefaultMacAddress();
    display.println("Board Name ERROR!");
  }
  Serial.println(boardName);
  display.println(boardName);

  if (!fsu.getPrefIPA(MyIP_KEY, MyIP))
  {
    Serial.println("My IP ERROR!");
    display.println("My IP ERROR!");
  }
  else
  {
    Serial.println("My IP OK! : ");
    display.println("My IP OK! : " + String(MyIP[0]) + "." + String(MyIP[1]) + "." + String(MyIP[2]) + "." + String(MyIP[3]));
  }
  if (!fsu.getPrefIPA(GATWAY_IP_KEY, GatewayIP))
  {
    Serial.println("My IP ERROR!");
    display.println("My IP ERROR!");
  }
  else
  {
    Serial.println("My IP OK! : ");
    display.println("My IP OK! : " + String(GatewayIP[0]) + "." + String(GatewayIP[1]) + "." + String(GatewayIP[2]) + "." + String(GatewayIP[3]));
  }

  if (!fsu.getPrefULong(SCAN_TIMING_KEY, scan_timing))
  {
    Serial.println("Scan Timing ERROR!");
    display.println("Scan Timing ERROR!");
  }
  else
  {
    Serial.println("Scan Timing OK! : " + String(scan_timing));
    display.println("Scan Timing OK! : " + String(scan_timing));
  }

  loadChildren();
}

// -----------------------------------------------------------------------
// == == Wi - Fi と時刻初期化 == ==
bool getNTP()
{
  bool ret = false;
  display.println("get NTP");
  if (WiFi.status() != WL_CONNECTED)
  {
    DisplayPrint("ERROR getNTP WiFi Disconnect");
    return ret;
  }
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp", "time.google.com"); // 2.7.0以降, esp32コンパチ
  delay(1000);                                                                    // NTPサーバーに接続するまで待機
  time_t t;
  struct tm *timeinfo;
  t = time(NULL);
  timeinfo = localtime(&t);
  int retry = 0;
  while (!getLocalTime(timeinfo, 1000))
  {
    if (retry > 30)
    {
      retry = -1;
      break;
    }
    display.print("*");
    delay(1000);
    retry++;
  }
  display.print("\n");
  if (retry < 0)
  {
    DisplayPrint("ERROR NTP/getLocalTime()");
    ret = false;
  }
  else
  {
    display.println("NTP OK!");
    ret = true;
  }
  return ret;
}
void setupWiFiAndTime()
{

  DisplayClear();
  display.println("Connecting WiFi...");
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFi.disconnect();
  }
  // 固定IP設定

  if ((MyIP[0] != 0) && (GatewayIP[0] != 0))
  {
    IPAddress local_IP(MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
    IPAddress gateway(GatewayIP[0], GatewayIP[1], GatewayIP[2], GatewayIP[3]);
    display.printf("Config IP: %d.%d.%d.%d\n", local_IP[0], local_IP[1], local_IP[2], local_IP[3]);
    display.printf("Config Gateway: %d.%d.%d.%d\n", gateway[0], gateway[1], gateway[2], gateway[3]);

    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8);   // オプション
    IPAddress secondaryDNS(8, 8, 4, 4); // オプション
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
      display.setCursor(10, 70);
      display.println("STA Failed to configure");
    }
    else
    {
      display.printf("Config OK!\nip:%s\n", WiFi.localIP().toString());
    }
  }
  WiFi.setHostname(boardName.c_str());
  WiFi.begin();
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  int retry = 0;
  bool done = true;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      break;
      ;
    }
    display.print("*");
    delay(500);
    if (retry % 10 == 9)
    {
      WiFi.disconnect();
      WiFi.reconnect();
    }
    else if (retry > 30)
    {
      break;
      ;
    }
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    display.println("\nWiFi Connected!");
  }
  else
  {
    display.println("\nWiFi Failed");
  }

  getNTP();
}
// -----------------------------------------------------------------------
// ==== 自機センサーの更新 ====
void setupAHT()
{
#ifdef SENSOR_ALIVE
  if (!aht.begin())
  {
    Serial.println("AHT20 が見つかりません。配線チェックして下さい。");
  }
  else
  {
    Serial.println("AHT20  接続確認");
  };
#endif
}
void setupBMP280()
{
#ifdef SENSOR_ALIVE
  unsigned status;
  status = bmp.begin(0x77, BMP280_CHIPID);
  if (!status)
  {
    Serial.println("BMP280が見つかりません。配線チェックして下さい。");
  }
  else
  {
    Serial.println("BMP280 接続確認");
    // Default settings from datasheet.
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  }
#endif
}
//---------------------------------------------------
void readOwnTemp()
{
#ifdef SENSOR_ALIVE
  sensors_event_t humidity, temp;
  if (!aht.getEvent(&humidity, &temp))
  {
    setupAHT();
    aht.getEvent(&humidity, &temp);
  }

  float pres = bmp.readPressure() / 100.0;
  if (pres < 500 || pres > 1500)
  {
    setupBMP280();
    pres = bmp.readPressure() / 100.0;
  }
  OwnTemp.temp = temp.temperature;
  OwnTemp.hum = humidity.relative_humidity;
  OwnTemp.pres = pres;
#endif
  // 現在時刻取得
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y/%m/%d %H:%M:%S", &timeinfo);

  OwnTemp.timeStr = timeStr;
}
//---------------------------------------------------
bool checkChild(int idx)
{
  bool ret = false;
  if (idx < 0 || idx >= MAX_CLIENTS)
    return ret;
  Serial.println("checkChild: " + String(idx));
  if (children[idx].ip.length() == 0)
  {
    Serial.println("no checkChild: " + String(idx));
    return ret;
  }
  WiFiClient client;
  int retry = 0;
  bool ok = true;
  Serial.println("connect Child: " + String(idx) + "/" + children[idx].ip);
  // display.println(children[idx].ip);
  while (!client.connect(children[idx].ip.c_str(), SERVER_PORT))
  {
    Serial.print("*");
    delay(350);
    retry++;
    if (retry > 2)
    {
      children[idx].IsConnected++;
      ok = false;
      break;
    }
  }
  if (!ok)
  {
    Serial.println("connect error! Child: " + String(idx));
    return ret;
  }
  if (client.connected())
  {
    children[idx].IsConnected = 0; // 接続成功時はカウンターをリセット
    JsonDocument sendData;
    sendData["cmd"] = "get_data";
    String json;
    serializeJson(sendData, json);
    client.print(json);
    Serial.println(json);

    String recvData;
    unsigned long start = millis();
    while (client.available() == 0 && millis() - start < 2000)
    {
      delay(10); // 最大2秒待機
    }
    while (client.available())
    {
      recvData += (char)client.read();
    }

    // 応答をJSONとして解析
    if (recvData.length() > 0)
    {
      Serial.println("Received data from child " + String(idx) + ": " + recvData);
      children[idx].IsConnected = true;
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, recvData);

      if (!doc["id"].isNull())
      {
        String s = doc["id"].as<String>();
        children[idx].id = s;
      }
      else
      {
        children[idx].id = children[idx].ip;
      }
      if (!doc["time"].isNull())
      {
        String s = doc["time"].as<String>();
        children[idx].timeStr = s;
      }
      else
      {
        children[idx].timeStr = "000/00/00 00:00:00";
      }
      if (!doc["temp"].isNull())
      {
        float s = doc["temp"].as<float>();
        children[idx].temp = s;
      }
      else
      {
        children[idx].temp = 0;
      }
      if (!doc["hum"].isNull())
      {
        float s = doc["hum"].as<float>();
        children[idx].hum = s;
      }
      else
      {
        children[idx].hum = 0;
      }
      if (!doc["pres"].isNull())
      {
        float s = doc["pres"].as<float>();
        children[idx].pres = s;
      }
      else
      {
        children[idx].pres = 0;
      }
      ret = true; // <-- Always set ret to true if we successfully got data
    }
  }
  else
  {
    children[idx].IsConnected++;
    Serial.println("Connect Err");
  }
  return ret;
}
//---------------------------------------------------
void checkChildren()
{
  if ((scanIndex < 0) || (scanIndex >= MAX_CLIENTS))
    scanIndex = 0;
  Serial.println("checkChildren: " + String(scanIndex));
  checkChild(scanIndex);

  scanIndex++;
  if (scanIndex >= MAX_CLIENTS)
  {
    scanIndex = 0;
  }
}
void checkChildrenAll()
{

  for (size_t i = 0; i < MAX_CLIENTS; i++)
  {
    Serial.println("checkChildren: " + String(i));
    display.println("checkChildren: " + String(i));
    checkChild(i);
  }
  scanIndex = 0; // Reset scanIndex after checking all children
}
//---------------------------------------------------

String toSuji(float v)
{
  if (v < 0)
    v *= -1;
  int v1 = (int)v;
  int v2 = (int)((v - v1) * 10);
  String ret = String(v1) + "." + String(v2);
  if (v < 10)
  {
    ret = " " + ret;
  }
  return ret;
}
// ==== 上部表示の更新 ====
void headbudPrint(int col)
{

  headbuf.fillScreen(TFT_BLACK);
  headbuf.setTextSize(1);
  headbuf.setTextColor(col);
  headbuf.setCursor(10, 2);
  headbuf.println("Temp-Hum Sensor by bry-ful");
  headbuf.setCursor(30, 20);
  headbuf.printf("%s-%.1f", OwnTemp.timeStr.c_str(), ((float)scan_timing / 1000));
  headbuf.setCursor(30, 38);
  headbuf.setTextSize(1);
  headbuf.printf("%.1f℃ %.1f%% %.1fpHa", OwnTemp.temp, OwnTemp.hum, OwnTemp.pres);
  headbuf.drawRoundRect(0, 0, SCR_WIDTH - 1, SCR_HEAD_HEIGHT - 1, 6, col);
  headbuf.setTextSize(1);
}
// ==== クライアント表示の更新 ====
void scrbudPrint(int index)
{
  if (index >= MAX_CLIENTS)
    return;
  String id = children[index].id;
  float t = children[index].temp;
  float h = children[index].hum;
  String tmstr = children[index].timeStr;
  int mode = 0;
  //
  //  0 no connect
  //  1 normal
  //  -1 Count err /Disconnect
  //
  int col = TFT_BLACK;
  if (id == "")
  {
    mode = 0;
    col = TFT_DARKGRAY;
  }
  else
  {
    col = TFT_WHITE;
    mode = 1;
    if (children[index].IsConnected >= 3)
    {
      mode = 2;
      col = TFT_RED;
    }
  }
  bool b = false;
  b = (((OwnTemp.hum - h < 10) || (h > 30)) && (mode == 1));
  scrbuf.fillScreen(TFT_BLACK);

  // ＩＤの描画
  scrbuf.setTextSize(1);
  scrbuf.setTextColor(col);
  scrbuf.setCursor(15, 1);
  scrbuf.setFont(&lgfxJapanGothic_16F);
  if (mode == -1)
  {
    scrbuf.print("-----");
  }
  else
  {
    scrbuf.print(id);
  }
  // 温度
  scrbuf.setTextSize(1);
  scrbuf.setCursor(15, 17);
  scrbuf.setFont(&lgfxJapanGothic_24F);
  if (mode == -1)
  {
    scrbuf.print("--.-");
  }
  else
  {
    scrbuf.print(toSuji(t));
  }
  if (t < 0)
  {
    scrbuf.drawLine(5, 12, 12, 12);
  }
  scrbuf.setFont(&lgfxJapanGothic_16F);
  scrbuf.setCursor(65, 25);
  scrbuf.print("℃");

  // 下線
  scrbuf.drawLine(10, SCR_LINE_HEIGHT - 1, SCR_WIDTH - 10, SCR_LINE_HEIGHT - 1);
  // 時間
  scrbuf.setTextSize(1);
  scrbuf.setCursor(200, 1);
  scrbuf.setFont(&lgfxJapanGothic_12F);
  // 2025/06/25 00:00:00
  // 0123456789ABCDEF0123
  scrbuf.println(tmstr.substring(0x0b, 0x10));

  // メーター
  int xp = 110;
  int yp = 25;
  int st = 90 + 45;
  int en = 360 + 45;
  int ea = st + (en - st) * h / 100;
  scrbuf.fillArc(xp, yp, 14, 10, st, en, TFT_DARKGRAY);
  scrbuf.fillArc(xp, yp, 14, 10, st, ea, col);
  int l = 18;
  double hr = st + (en - st) * (double)OwnTemp.hum / 100;

  double x = (cos(hr * PI / 180) * l);
  double y = (sin(hr * PI / 180) * l);
  scrbuf.drawArc(xp, yp, 18, 18, st, (int)hr, col);
  scrbuf.drawLine((int)(xp + x * 0.1), (int)(yp + y * 0.1), (int)(xp + x), (int)(yp + y), col);
  scrbuf.drawCircle(xp, yp, 6, col);
  // 湿度
  scrbuf.setCursor(130, 6);
  scrbuf.setFont(&lgfxJapanGothic_36F);
  if ((b) && (mode == 1))
  {
    scrbuf.setTextColor(TFT_YELLOW);
  }
  if (mode == -1)
  {
    scrbuf.print("--.-");
  }
  else
  {
    scrbuf.print(toSuji(h));
  }
  scrbuf.setFont(&lgfxJapanGothic_24F);
  scrbuf.setCursor(205, 16);
  scrbuf.print("%");
}
// ==== フッター表示の更新 ====
void footorPrint(int col)
{

  scrbuf.fillScreen(TFT_BLACK);

  int bitV = (millis() >> 3) & 0b11111;
  for (int i = 0; i < 5; i++)
  {
    if (bitV & 0x01 == 0x01)
    {
      scrbuf.fillRect(20 + i * 30, 2, 24, 6, col);
    }
    bitV = bitV >> 1;
    scrbuf.drawRect(20 + i * 30, 2, 24, 6, col);
  }
  scrbuf.setCursor(180, 0);
  scrbuf.setFont(&lgfxJapanGothic_8F);
  scrbuf.setTextColor(TFT_RED);
  scrbuf.setTextSize(1);
  scrbuf.println(WiFi.localIP().toString());
}
// ==== 画面表示の更新 ====
void PrintScrren()
{
  if (isScreenSaver)
  {
    return;
  }
  int col = TFT_WHITE;
  if (WiFi.status() != WL_CONNECTED)
  {
    col = TFT_RED;
  }
  headbudPrint(col);
  headbuf.pushSprite(0, 0);

  int idx = 0;
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    scrbudPrint(i);
    scrbuf.pushSprite(0, SCR_HEAD_HEIGHT + SCR_LINE_HEIGHT * i);
  }

  footorPrint(TFT_WHITE);
  scrbuf.pushSprite(0, SCR_HEAD_HEIGHT + SCR_LINE_HEIGHT * 6);
}
//---------------------------------------------------
void SerialSelect()
{
  bool isSend = false;
  String tx = fsu.GetSerialReadAll();
  tx.trim();
  if (tx == "")
    return;
  bool ok = false;
  JsonDocument jd = fsu.GetJsonData(tx, &ok);
  if (ok == false)
  {
    if (tx == "getid")
    {
      Serial.print(fsu.getBoardName(""));
    }
    else if (tx == "getstatus")
    {
      Serial.print(fsu.EPS_Status_json());
    }
    else
    {
      Serial.print("return:" + tx);
    }
    return;
  }
  JsonDocument res;
  if (fsu.JsonCMDCheck(jd, &res))
  {
    isSend = true;
    if (!jd["setBr"].isNull())
    {
      int br = jd["setBr"].as<int>();
      if (br >= 0)
      {
        display.setBrightness(br);
      }
      res["setBr"] = br;
    }
    else if (!jd["getBr"].isNull())
    {
      Serial.printf("brightness:%d", display.getBrightness());
      res["getBr"] = display.getBrightness();
    }
    else if (!jd["setScanTime"].isNull())
    {
      unsigned long v = jd["setScanTime"].as<unsigned long>();
      if (v >= 0)
      {
        scan_timing = v;
        int ff = fsu.setPrefULong(SCAN_TIMING_KEY, v);
        v = fsu.getPrefULong(SCAN_TIMING_KEY, v);
        Serial.println("ST" + String(v));
      }
      res["setScanTime"] = scan_timing;
    }
    else if (!jd["getScanTime"].isNull())
    {
      unsigned long v = fsu.getPrefULong(SCAN_TIMING_KEY, 0);
      if (v > 0)
      {
        scan_timing = v;
      }
      Serial.printf("scan_timing:%d/%d\n", v, scan_timing);
      res["getScanTime"] = scan_timing;
      // すぐに反映
    }
    else if (!jd["getIplist"].isNull())
    {
      String v = fsu.getPrefString(CHILDREN_KEY, "");
      Serial.printf("iplist:%s\n", v);
      res["getIplist"] = v;
    }
    else if (!jd["setIplist"].isNull())
    {

      String v = jd["setIplist"].as<String>();
      if (v != "")
      {
        setChildrenIPList(v);
        saveChildren();
      }
      res["setIplist"] = v;
    }
    else if (!jd["setMyIP"].isNull())
    {

      String v = jd["setMyIP"].as<String>();
      if (v != "")
      {
        if (fsu.setPrefString(MyIP_KEY, v))
        {
          res["setMyIP"] = v;
          isSend = true;
        }
        else
        {
          Serial.println("set My IP  ERROR");
        }
      }
    }
    else if (!jd["getMyIP"].isNull())
    {
      String v = fsu.getPrefString(MyIP_KEY, "");
      if (v != "")
      {
        res["getMyIP"] = v;
        isSend = true;
      }
      else
      {
        Serial.println("get My IP  ERROR");
      }
    }
  }
  if (isSend)
  {
    String ret;
    serializeJson(res, ret);
    Serial.print(ret);
  }
}
//---------------------------------------------------
void setup()
{
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW); // Turn off the LED

  pinMode(BTN_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  setupDisplay();
  display.println("Starting Sensor Control...");
  loadPref();
  setupWiFiAndTime();

  //---------------------------------------------------
#ifdef SENSOR_ALIVE
  setupAHT();
  setupBMP280();
#endif
  readOwnTemp();
  checkChildrenAll();
  PrintScrren();
  nextTime = millis();
  blinkTime = millis();
  screenSaveTime = millis();
}

void loop()
{
  SerialSelect();
  if (digitalRead(BTN_PIN) == LOW)
  {
    if (isScreenSaver)
    {
      isScreenSaver = false;
      display.setBrightness(80); // 画面を明るくする
      screenSaveTime = millis(); // スクリーンセーバーのタイマーをリセット
      nextTime = 0;              // タイマーをリセットしてすぐに画面を更新
    }
    else
    {
      isScreenSaver = true;
      display.setBrightness(0); // 画面を暗くする
    }
    delay(500); // ボタンのチャタリング防止
  }
  if (!isScreenSaver)
  {
    if (millis() - screenSaveTime > screenSaver_timing) //
    {
      // スクリーンセーバーの処理
      isScreenSaver = true;
      // screenSaveTime = millis();
      display.setBrightness(0); // 画面を暗くする
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    if (isScreenSaver)
    {
      isScreenSaver = false;
      display.setBrightness(80); // 画面を明るくする
      screenSaveTime = millis(); // スクリーンセーバーのタイマーをリセット
    }
    setupWiFiAndTime();
    nextTime = 0; // タイマーをリセットしてすぐに画面を更新
  }
  if (millis() - nextTime > scan_timing) //
  {
    readOwnTemp();
    checkChildren();
    // 正常か確認
    bool err = false;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (children[i].ip.length() == 0)
        continue;
      if (children[i].IsConnected >= 3)
      {
        // 接続エラーが続いているクライアント
        err = true;
        break;
      }
      if (children[i].hum >= 35.0f)
      {
        // 湿度が高いクライアント
        err = true;
        break;
      }
    }
    if (err)
    {
      display.setBrightness(100); // 画面を明るくする
      screenSaveTime = millis();  // スクリーンセーバーのタイマーをリセット
    }
    if (!isScreenSaver)
    {
      PrintScrren();
    }
    nextTime = millis();
  }
  /*
  if (millis() - blinkTime > 1000) // 500ミリ秒ごとにLEDを点滅
  {
    readOwnTemp();
    PrintScrren();
    blinkLED();
    blinkTime = millis();
  }
    */
}
