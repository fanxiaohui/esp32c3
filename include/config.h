// WIFI
#define CONFIG_WIFI_SSID "singzer"
#define CONFIG_WIFI_PASS "1008610086"

// Default
#define CONFIG_PROJECT_CODE "iot"
#define CONFIG_DEVICE_MODEL "icepie-test"
#define CONFIG_DEVICE_FW 0.81

#ifdef ESP32
#define CONFIG_DEVICE_HW "ESP32-REV02"
#elif ESP8266
#define CONFIG_DEVICE_HW "ESP8266-REV02"
#endif

// MQTT
#define CONFIG_MQTT_HOST "192.168.1.120"
#define CONFIG_MQTT_PORT 1883
#define CONFIG_MQTT_USER "device"
#define CONFIG_MQTT_PASS "password"
#define CONFIG_MQTT_KEEP_ALIVE 60

// OTA
#define CONFIG_OTA_HOST "iot.icepie.net"
#define CONFIG_OTA_PORT 6689


#ifdef ESP32C3
#define CONFIG_OTA_PATH "/" CONFIG_DEVICE_MODEL "/esp32c3/firmware.bin"
#elif ESP32S2
#define CONFIG_OTA_PATH "/" CONFIG_DEVICE_MODEL "/esp32s2/firmware.bin"
#elif ESP32
#define CONFIG_OTA_PATH "/" CONFIG_DEVICE_MODEL "/esp32/firmware.bin"
#elif ESP8266
#define CONFIG_OTA_PATH "/" CONFIG_DEVICE_MODEL "/esp8266/firmware.bin"
#endif

/*
 *  Set a topic use a unique value and topic type, just like "project/deviceid/msgtype"
 *  Return a String value
 */
String setTopic(String unique, String type)
{
    String topic = CONFIG_PROJECT_CODE;
    topic += "/";
    topic += unique;
    topic += "/";
    topic += type;

    return topic;
}
