/*
  *鸣谢
  https://youtu.be/bePgZIXHSkM
  (c)2017 Pawel A. Hernik
  感谢视频原作者分享的源代码以及提供的思路。
  
  *作者
  AlexCheng Bilibili频道：咸鱼Cosmos 欢迎订阅！
  
  *日期
  2018/11/1
*/

//  硬件接线说明
//  ESP-01：

//  GPIO 2 - DataIn
//  GPIO 1 - LOAD/CS
//  GPIO 0 - CLK

//  ------------------------
//  NodeMCU 1.0 ：

//  D8 - DataIn
//  D7 - LOAD/CS
//  D6 - CLK

	

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define NUM_MAX 4
#define ROTATE 90

// for ESP-01 module
//#define DIN_PIN 2 // D4
//#define CS_PIN  3 // D9/RX
//#define CLK_PIN 0 // D3

// for NodeMCU 1.0
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 12  // D6

#include "max7219.h"
#include "fonts.h"

// =======================================================================
// 配置信息
// =======================================================================
const char* ssid     = "Cosmos";                 // WiFi名
const char* password = "abc123456";            //WiFi密码
String channelId = "96458132";                   //bilibili UID
long utcOffset = 8;                              //时区，默认 +8 为东八区（中国北京）
// =======================================================================

void setup()
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);
  WiFi.enableAP(false);     //关闭AP热点
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift(" WiFi ...~", 15, font, ' ');
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
  Serial.println("Getting data ...");
  printStringWithShift("Bi ... ~", 15, font, ' ');
}
// =======================================================================

long viewCount, viewCount24h = -1, viewsGain24h;
long subscriberCount, subscriberCount1h = -1, subscriberCount24h = -1, subsGain1h = 0, subsGain24h = 0;
long videoCount;
int cnt = 0;
unsigned long time1h, time24h;
long localEpoc = 0;
long localMillisAtUpdate = 0;
int h, m, s;
String date;

void loop()
{
  if (cnt <= 0) {
    if ((getYTData() == 0) && (getVideoData() == 0) && (getplayData() == 0)) {
      cnt = 1;  // data is refreshed every 50 loops
      if (subscriberCount1h < 0) {
        time1h = time24h = millis();
        subscriberCount1h = subscriberCount24h = subscriberCount;
        viewCount24h = viewCount;
      }
      if (millis() - time1h > 1000 * 60 * 60) {
        time1h = millis();
        subscriberCount1h = subscriberCount;
      }
      if (millis() - time24h > 1000 * 60 * 60 * 24) {
        time24h = millis();
        subscriberCount24h = subscriberCount;
        viewCount24h = viewCount;
      }
      subsGain1h = subscriberCount - subscriberCount1h;
      subsGain24h = subscriberCount - subscriberCount24h;
      viewsGain24h = viewCount - viewCount24h;
    }
  }
  cnt--;
  updateTime();
  int del = 3000;
  int scrollDel = 20;
  char txt[10];

  //+++++++++++++++++++++粉丝数+++++++++++++++++++++//
  printStringWithShift("  Fans: ", scrollDel, font, ' '); // eng
  printValueWithShift(subscriberCount, scrollDel, 0);
  delay(del);
  //+++++++++++++++++++++1小时增加粉丝数（无则不显示）+++++++++++++++++++++//
  if (subsGain1h > 0) {
    printStringWithShift("  Fans gain 1h: ", scrollDel, font, ' '); // eng
    printValueWithShift(subsGain1h, scrollDel, 1);
    delay(del);
  }
  //+++++++++++++++++++++24小时粉丝数（无则不显示）+++++++++++++++++++++//
  if (subsGain24h > 0) {
    printStringWithShift("  Fans gain 24h: ", scrollDel, font, ' '); // eng
    printValueWithShift(subsGain24h, scrollDel, 1);
    delay(del);
  }
  //+++++++++++++++++++++直播数（无则不显示）+++++++++++++++++++++//	
  if (viewCount != viewCount24h) {
    printStringWithShift("  Live: ", scrollDel, font, ' '); // eng
    printValueWithShift(viewCount, scrollDel, 0);
    delay(del);
  }
  //+++++++++++++++++++++播放数+++++++++++++++++++++//
  printStringWithShift("  Play: ", scrollDel, font, ' '); // eng
  printValueWithShift(videoCount, scrollDel, 0);
  delay(del);
  //+++++++++++++++++++++时间（根据前面设定的时区决定）+++++++++++++++++++++//
  sprintf(txt, "    %02d:%02d  ", h, m);
  printStringWithShift(txt, scrollDel, font, ' '); // real time
  delay(del);

}
// =======================================================================

int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if (c == 196 || c == 197 || c == 195) {
    dualChar = c;
    return 0;
  }
  if (dualChar) {
    switch (_c) {
      case 133: c = 1 + '~'; break; // 'ą'
      case 135: c = 2 + '~'; break; // 'ć'
      case 153: c = 3 + '~'; break; // 'ę'
      case 130: c = 4 + '~'; break; // 'ł'
      case 132: c = dualChar == 197 ? 5 + '~' : 10 + '~'; break; // 'ń' and 'Ą'
      case 179: c = 6 + '~'; break; // 'ó'
      case 155: c = 7 + '~'; break; // 'ś'
      case 186: c = 8 + '~'; break; // 'ź'
      case 188: c = 9 + '~'; break; // 'ż'
      //case 132: c = 10+'~'; break; // 'Ą'
      case 134: c = 11 + '~'; break; // 'Ć'
      case 152: c = 12 + '~'; break; // 'Ę'
      case 129: c = 13 + '~'; break; // 'Ł'
      case 131: c = 14 + '~'; break; // 'Ń'
      case 147: c = 15 + '~'; break; // 'Ó'
      case 154: c = 16 + '~'; break; // 'Ś'
      case 185: c = 17 + '~'; break; // 'Ź'
      case 187: c = 18 + '~'; break; // 'Ż'
      default:  break;
    }
    dualChar = 0;
    return c;
  }
  switch (_c) {
    case 185: c = 1 + '~'; break;
    case 230: c = 2 + '~'; break;
    case 234: c = 3 + '~'; break;
    case 179: c = 4 + '~'; break;
    case 241: c = 5 + '~'; break;
    case 243: c = 6 + '~'; break;
    case 156: c = 7 + '~'; break;
    case 159: c = 8 + '~'; break;
    case 191: c = 9 + '~'; break;
    case 165: c = 10 + '~'; break;
    case 198: c = 11 + '~'; break;
    case 202: c = 12 + '~'; break;
    case 163: c = 13 + '~'; break;
    case 209: c = 14 + '~'; break;
    case 211: c = 15 + '~'; break;
    case 140: c = 16 + '~'; break;
    case 143: c = 17 + '~'; break;
    case 175: c = 18 + '~'; break;
    default:  break;
  }
  return c;
}

// =======================================================================

int charWidth(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  return pgm_read_byte(data + 1 + ch * len);
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  scr[NUM_MAX * 8] = 0;
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i + 1] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  return w;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay, const uint8_t *data, int offs)
{
  c = convertPolish(c);
  if (c < offs || c > MAX_CHAR) return;
  c -= offs;
  int w = showChar(c, data);
  for (int i = 0; i < w + 1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char *s, int shiftDelay, const uint8_t *data, int offs)
{
  while (*s) printCharWithShift(*s++, shiftDelay, data, offs);
}

// =======================================================================
// printValueWithShift():
// converts int to string
// centers string on the display
// chooses proper font for string/number length
// can display sign - or +
void printValueWithShift(long val, int shiftDelay, int sign)
{
  const uint8_t *digits = digits5x7;       // good for max 5 digits
  if (val > 1999999) digits = digits3x7;   // good for max 8 digits
  else if (val > 99999) digits = digits4x7; // good for max 6-7 digits
  String str = String(val);
  if (sign) {
    if (val < 0) str = ";" + str; else str = "<" + str;
  }
  const char *s = str.c_str();
  int wd = 0;
  while (*s) wd += 1 + charWidth(*s++ - '0', digits);
  wd--;
  int wdL = (NUM_MAX * 8 - wd) / 2;
  int wdR = NUM_MAX * 8 - wdL - wd;
  //Serial.println(wd); Serial.println(wdL); Serial.println(wdR);
  s = str.c_str();
  while (wdL > 0) {
    printCharWithShift(':', shiftDelay, digits, '0');
    wdL--;
  }
  while (*s) printCharWithShift(*s++, shiftDelay, digits, '0');
  while (wdR > 0) {
    printCharWithShift(':', shiftDelay, digits, '0');
    wdR--;
  }
}

// =====================订阅数==================================================

const char *ytHost = "api.bilibili.com";

int getYTData()
{
  WiFiClientSecure client;
  Serial.print("connecting to "); Serial.println(ytHost);
  if (!client.connect(ytHost, 443)) {
    Serial.println("connection failed");
    return -1;
  }
  String cmd = String("GET /x/relation/stat?vmid=") + channelId + " HTTP/1.1\r\n" +
               "Host: " + ytHost + "\r\nUser-Agent: ESP8266/1.1\r\nConnection: close\r\n\r\n";
  client.print(cmd);

  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    Serial.println("y."); delay(500);
  }
  String line, buf = "";
  int startJson = 0, dateFound = 0;
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    if (line[0] == '{') startJson = 1;
    if (startJson) {
      for (int i = 0; i < line.length(); i++)
        if (line[i] == '[' || line[i] == ']') line[i] = ' ';
      buf += line + "\n";
    }
    if (!dateFound && line.startsWith("Date: ")) {
      dateFound = 1;
      date = line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(buf);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    printStringWithShift("json error!", 30, font, ' ');
    delay(10);
    return -1;
  }
  subscriberCount = root["data"]["follower"];
  return 0;
}

// =====================直播参数==================================================

const char *videoHost = "api.live.bilibili.com";

int getVideoData()
{
  WiFiClientSecure client;
  Serial.print("connecting to "); Serial.println(videoHost);
  if (!client.connect(videoHost, 443)) {
    Serial.println("connection failed");
    return -1;
  }
  String cmd = String("GET /room/v1/Room/getRoomInfoOld?mid=") + channelId + " HTTP/1.1\r\n" +
               "Host: " + videoHost + "\r\nUser-Agent: ESP8266/1.1\r\nConnection: close\r\n\r\n";
  client.print(cmd);

  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    Serial.println("y."); delay(500);
  }
  String line, buf = "";
  int startJson = 0, dateFound = 0;
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    if (line[0] == '{') startJson = 1;
    if (startJson) {
      for (int i = 0; i < line.length(); i++)
        if (line[i] == '[' || line[i] == ']') line[i] = ' ';
      buf += line + "\n";
    }
    if (!dateFound && line.startsWith("Date: ")) {
      dateFound = 1;
      date = line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(buf);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    printStringWithShift("json error!", 30, font, ' ');
    delay(10);
    return -1;
  }
  viewCount = root["data"]["online"];
  return 0;
}
// =====================播放数参数==================================================

const char *playHost = "api.bilibili.com";

int getplayData()
{
  WiFiClientSecure client;
  Serial.print("connecting to "); Serial.println(playHost);
  if (!client.connect(playHost, 443)) {
    Serial.println("connection failed");
    return -1;
  }
  String cmd = String("GET /x/space/upstat?mid=") + channelId + " HTTP/1.1\r\n" +
               "Host: " + playHost + "\r\nUser-Agent: ESP8266/1.1\r\nConnection: close\r\n\r\n";
  client.print(cmd);

  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    Serial.println("y."); delay(500);
  }
  String line, buf = "";
  int startJson = 0, dateFound = 0;
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    if (line[0] == '{') startJson = 1;
    if (startJson) {
      for (int i = 0; i < line.length(); i++)
        if (line[i] == '[' || line[i] == ']') line[i] = ' ';
      buf += line + "\n";
    }
    if (!dateFound && line.startsWith("Date: ")) {
      dateFound = 1;
      date = line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(buf);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    printStringWithShift("json error!", 30, font, ' ');
    delay(10);
    return -1;
  }
  videoCount = root["data"]["archive"]["view"];
  return 0;
}
// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L) % 86400L;
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================
