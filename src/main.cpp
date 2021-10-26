#ifdef ESP32

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
// #include "FS.h"
// #include "SPIFFS.h"

/*
This example uses FreeRTOS softwaretimers as there is no built-in Ticker library
*/
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#define ESP_getChipId() ((uint32_t)ESP.getEfuseMac())

#define ESPhttpUpdate httpUpdate

#elif ESP8266

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// #include <FS.h>
#include <Ticker.h>

#define ESP_getChipId() (ESP.getChipId())

#endif

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "led.h"

// hw set
String deviceSN = "0x" + String(ESP_getChipId(), HEX); // Must be unique on the MQTT network

// mqtt set
String pubTopic = setTopic(deviceSN, "msg");
String subTopic = setTopic(deviceSN, "event");

Led myLed; //Reverse
WiFiClient client;
AsyncMqttClient mqttClient;

void update_started()
{
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished()
{
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total)
{
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err)
{
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void otaUpdate()
{
// #ifdef ESP8266 
  // LED_BUILTIN for ESP32 is defined in config.h
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
// #endif

  // Add optional callback notifiers
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, CONFIG_OTA_HOST, CONFIG_OTA_PORT, CONFIG_OTA_PATH);

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

// bool loadConfig() {
//   File configFile = SPIFFS.open("/config.json", "r");
//   if (!configFile) {
//     Serial.println("Failed to open config file");
//     return false;
//   }

//   size_t size = configFile.size();
//   if (size > 1024) {
//     Serial.println("Config file size is too large");
//     return false;
//   }

//   // Allocate a buffer to store contents of the file.
//   std::unique_ptr<char[]> buf(new char[size]);

//   // We don't use String here because ArduinoJson library requires the input
//   // buffer to be mutable. If you don't use ArduinoJson, you may as well
//   // use configFile.readString instead.
//   configFile.readBytes(buf.get(), size);

//   StaticJsonDocument<200> doc;
//   auto error = deserializeJson(doc, buf.get());
//   if (error) {
//     Serial.println("Failed to parse config file");
//     return false;
//   }

//   const char* ledStatus = doc["data"]["led"];

//   // Real world application would store these values in some variables for
//   // later use.

//   Serial.print("led: ");
//   Serial.println(ledStatus);
//   return true;
// }

// bool saveConfig() {
//   StaticJsonDocument<200> doc;
//   doc["data"]["led"] = "off";

//   File configFile = SPIFFS.open("/config.json", "w");
//   if (!configFile) {
//     Serial.println("Failed to open config file for writing");
//     return false;
//   }

//   serializeJson(doc, configFile);
//   return true;
// }

void sendStatus(unsigned int msg_id, String cmd)
{
  DynamicJsonDocument msg(1024);

  String type = "status";

  // create the job json data
  msg["type"] = type;
  msg["msg_id"] = msg_id;
  msg["event_cmd"] = cmd;

  if (myLed.getStatus())
  {
    msg["data"]["led"] = "on";
  }
  else
  {
    msg["data"]["led"] = "off";
  }

  msg["wifi"]["ssid"] = WiFi.SSID();
  msg["wifi"]["gateway"] = WiFi.gatewayIP().toString();
  msg["wifi"]["ip"] = WiFi.localIP().toString();
  msg["wifi"]["rssi"] = WiFi.RSSI();
  msg["wifi"]["mac"] = WiFi.BSSIDstr();
  msg["wifi"]["channel"] = WiFi.channel();
  msg["wifi"]["dns"] = WiFi.dnsIP().toString();

  msg["sn"] = deviceSN;
  msg["mac"] = WiFi.macAddress();
  msg["model"] = CONFIG_DEVICE_MODEL;
  msg["hw_ver"] = CONFIG_DEVICE_HW;
  msg["fw_ver"] = CONFIG_DEVICE_FW;
  //msg["data"]["full_ver"] = ESP.getFullVersion();

  char buffer[1024];
  serializeJson(msg, buffer);
  mqttClient.publish(pubTopic.c_str(), 0, false, buffer);
}

void sendFeedback(unsigned int msg_id, String cmd)
{
  DynamicJsonDocument msg(1024);

  String type = "feedback";
  // create the job json data
  msg["type"] = type;
  msg["msg_id"] = msg_id;
  msg["event_cmd"] = cmd;

  msg["sn"] = deviceSN;
  msg["model"] = CONFIG_DEVICE_MODEL;
  msg["hw_ver"] = CONFIG_DEVICE_HW;
  msg["fw_ver"] = CONFIG_DEVICE_FW;

  char buffer[256];
  serializeJson(msg, buffer);
  mqttClient.publish(pubTopic.c_str(), 0, false, buffer);
}

#ifdef ESP32
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void WiFiEvent(WiFiEvent_t event)
{
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    connectToMqtt();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    xTimerStart(wifiReconnectTimer, 0);
    break;
  }
}
#elif ESP8266
Ticker mqttReconnectTimer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}
#endif

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(subTopic.c_str(), 0);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub);
  Serial.println(subTopic.c_str());
  // mqttClient.publish(pubTopic.c_str(),);
  // Serial.println("Publishing at QoS 0");
  // uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
  // Serial.print("Publishing at QoS 1, packetId: ");
  // Serial.println(packetIdPub1);
  // uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
  // Serial.print("Publishing at QoS 2, packetId: ");
  // Serial.println(packetIdPub2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {

#ifdef ESP32
    xTimerStart(mqttReconnectTimer, 0);
#elif ESP8266
    mqttReconnectTimer.once(2, connectToMqtt);
#endif
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, len);
  JsonObject event = doc.as<JsonObject>();

  String cmd = event["cmd"];
  String data = event["data"];
  unsigned int msg_id = event["msg_id"];

  if (cmd == "ota")
  {
    sendFeedback(msg_id, cmd);
    otaUpdate();
  }
  else if (cmd == "status")
    sendStatus(msg_id, cmd);
  else if (cmd == "led")
  {
    if (data == "on")
      myLed.on();
    else if (data == "off")
      myLed.off();
    sendFeedback(msg_id, cmd);
  }
}

void onMqttPublish(uint16_t packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  randomSeed(analogRead(0));

#ifdef ESP32
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.onEvent(WiFiEvent);
#elif ESP8266
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
#endif

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT).setCredentials(CONFIG_MQTT_USER, CONFIG_MQTT_PASS).setClientId(deviceSN.c_str()).setKeepAlive(CONFIG_MQTT_KEEP_ALIVE);

  connectToWifi();

  Serial.println(CONFIG_OTA_PATH);

  // if (!SPIFFS.begin()) {
  //   Serial.println("Failed to mount file system");
  //   return;
  // }

  // if (!saveConfig()) {
  //   Serial.println("Failed to save config");
  // } else {
  //   Serial.println("Config saved");
  // }

  // if (!loadConfig()) {
  //   Serial.println("Failed to load config");
  // } else {
  //   Serial.println("Config loaded");
  // }
}

void loop()
{
}
