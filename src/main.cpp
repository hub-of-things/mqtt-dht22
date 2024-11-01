#include <PubSubClient.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include <ArduinoJson.h>
#include "DHT.h"
#include "time.h"
#include <Wire.h>
#include <BH1750.h>

// PIN DEFINITION

#define STATUS_PIN LED_BUILTIN
#ifdef ESP8266
#define DHTPIN D4
// -- When FACTORY_RESET_BUTTON_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define FACTORY_RESET_BUTTON_PIN D7
#elif ESP32
#define DHTPIN 4
// -- When FACTORY_RESET_BUTTON_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define FACTORY_RESET_BUTTON_PIN 35
#endif

//const char* ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = -3*60*60; //todo put in config
//const int   daylightOffset_sec = 0 * 60 * 60 ; //todo put in config

//--------------
// IoT STUFF
//--------------
#ifdef ESP8266
String ChipId = String(ESP.getChipId(), HEX);
#elif ESP32
String ChipId = String((uint32_t)ESP.getEfuseMac(), HEX);
#endif
const char htmlRoot[] PROGMEM = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>"
                                "<title>IotWebConf 01 Minimal</title></head><body>"
                                "Go to <a href='config'>configure page</a> to change settings."
                                "</body></html>\n";

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
String thingName = "NKM-" + ChipId;

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "nkmTHNG32";
#define STRING_LEN 128
#define NUMBER_LEN 10
// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "nkm2"
// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN
DNSServer dnsServer;
WebServer server(80);
WiFiClient net;
PubSubClient mqttClient(net);

// CONFIG VALUES
char mqttServerValue[STRING_LEN];       // MQTT SERVER
char mqttUserNameValue[STRING_LEN];     // MQTT USER
char mqttUserPasswordValue[STRING_LEN]; // MQTT PASSWORD
char mqttPrefixValue[STRING_LEN];       // BASE TOPIC TO PUBLISH <<BASE>>/status AND TO SOBSCRIBE TO COMMANDS <<BASE>>/+/set
char mqttPortValue[NUMBER_LEN];         // MQTT PORT

char fanSetPointValue[NUMBER_LEN];  // MINIMUN TIME BETWEEEN STATUS UPDATES
char fanThresholdValue[NUMBER_LEN]; // MAXIMUN TIME BETWEEEN STATUS PUBLISH

String mqttStatusTopic;
String mqttCmdTopic;
#define DEFAULT_MQTT_PORT 1883
int mqttPort = DEFAULT_MQTT_PORT;

// char MQTT_AVAILABILITY_TOPIC
IotWebConf iotWebConf(thingName.c_str(), &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::ParameterGroup
IotWebConfParameterGroup mqttGroup = IotWebConfParameterGroup("mqtt", "MQTT configuration");
IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("MQTT server", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfTextParameter mqttUserNameParam = IotWebConfTextParameter("MQTT user", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfPasswordParameter mqttUserPasswordParam = IotWebConfPasswordParameter("MQTT password", "mqttPass", mqttUserPasswordValue, STRING_LEN);
IotWebConfTextParameter mqttPrefixPrefix = IotWebConfTextParameter("MQTT topic prefix (no / ending)", "mqttPrefix", mqttPrefixValue, STRING_LEN);
IotWebConfNumberParameter mqttPortParam = IotWebConfNumberParameter("MQTT port", "mqttPort", mqttPortValue, NUMBER_LEN, "1883", "1..65535", "min='1' max='65535' step='1'");

IotWebConfParameterGroup fanGroup = IotWebConfParameterGroup("timing", "Status Publich Threshold");
IotWebConfNumberParameter fanSetPoint = IotWebConfNumberParameter("Min Time between (sec)", "fanSetPoint", fanSetPointValue, NUMBER_LEN, nullptr, "e.g. 1", "min='1' max='50' step='1'");
IotWebConfNumberParameter fanThreshold = IotWebConfNumberParameter("Max Time between (sec)", "fanThreshold", fanThresholdValue, NUMBER_LEN, nullptr, "e.g. 10", "min='10' max='50' step='1'");


// STATUS VARIABLES
struct tm timeinfo;
float lastTemp = -999;
float lastLux=-999;
float humidity=0;
float temperature=0;
float lux;

// THROTTLING VARIABLES
unsigned long now = millis();
unsigned long lastSensorLoop=0;
unsigned long lastStatus = 0;
unsigned long minStatus = 10000;
unsigned long maxStatus = 30000;

unsigned long lastMqttConnectionAttempt = 0;

// FLAGS
bool needReset = false;       // reset flag
bool needMqttConnect = false; // need to connect to mqtt

//--------------
// TEMP SENSOR SETUP
//--------------
#define TEMP_STATUS_THRESHOLD 0.1 // TEMPERATURE SENSITIVITY TO SEND UPDATES
#define DHTTYPE DHT22             // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// -- Method declarations.
void handleRoot();
void mqttMessageReceived(char *topic, byte *payload, unsigned int length);

//--------------
// LIGTH SENSOR SETUP
//--------------
#define LUX_STATUS_THRESHOLD 50 // TEMPERATURE SENSITIVITY TO SEND UPDATES
BH1750 lightMeter;





// iotWebConf -Handle web requests to "/" path.
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  server.send(200, "text/html", htmlRoot);
}

// iotWebConf - CONFIG SAVED HANDLER
void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

// iotWebConf - WIFI CONNECTED HANDLER
void wifiConnected()
{
  needMqttConnect = true;
}

// SET MQTT CONNECT
boolean connectMqttOptions()
{
  boolean sucess = false;

  if (mqttUserPasswordValue[0] != '\0' && mqttUserNameValue[0] != '\0')
  {
    sucess = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue, mqttUserPasswordValue);
  }
  else if (mqttUserNameValue[0] != '\0')
  {
    sucess = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue, "");
  }
  else
  {
    sucess = mqttClient.connect(iotWebConf.getThingName());
  }

  return sucess;
}

// MQTT SEND STATUS
void publishStatus()
{

  bool threashold = false;
  // minimun period between status not met
  if (now - lastStatus < minStatus)
    return;

  if (abs(lastTemp - temperature) > TEMP_STATUS_THRESHOLD)
    threashold = true;
  else if (abs(lastLux - lux) > LUX_STATUS_THRESHOLD)
    threashold = true;

  // maximun period beween status and min value change not met
  if (!threashold && (now - lastStatus) < maxStatus)
    return;

  const size_t bufferSize = JSON_OBJECT_SIZE(8);
  DynamicJsonDocument jsonBuffer(bufferSize);
  JsonObject root = jsonBuffer.to<JsonObject>();
  root["temperature"] = temperature;
  root["humidity"] = humidity;
  root["lux"] = lux;

  /*char buf[50];
  sprintf(buf,  "%A, %B %d %Y %H:%M:%S",&timeinfo);
  root["time"] = buf;
  */
  char message[70];
  size_t n = serializeJson(jsonBuffer, message, sizeof(message));

  if (mqttClient.publish(mqttStatusTopic.c_str(), (const uint8_t *)message, n, true))
  {
    lastStatus = now;
    lastTemp = temperature;
    lastLux = lux;
    Serial.println(message);
  }
  else
  {
    Serial.println("MQTT ERROR PUBLISHING");
  }
}

boolean connectMqtt()
{
  if (5000 > now - lastMqttConnectionAttempt)
  {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Connecting to MQTT server...");
  if (!connectMqttOptions())
  {
    lastMqttConnectionAttempt = now;
    return false;
  }

  Serial.println(mqttCmdTopic);
  // comando de power
  mqttClient.subscribe(mqttCmdTopic.c_str());
  publishStatus();
  return true;
}

// MQTT COMMAND HANDLER
void mqttMessageReceived(char *topic, byte *payload, unsigned int length)
{
  if (length > NUMBER_LEN)
  {
    Serial.println("MQTT command payload too long");
    return;
  }

  payload[length] = '\0'; // null al final para que no siga elyendo y usarlo cono char*
  char *chr_payload = (char *)payload;

  int pinIndex = -1;
  float val;
  if (strstr(topic, "/refresh/") != NULL)
  {
    // clena variables to force refresh
    lastStatus=0;
    lastTemp=-999;
    lastLux=-999;
  }

  if (strstr(topic, "/reset/") != NULL)
  {
    needReset=true;
  }
  if (strstr(topic, "/firmware/") != NULL)
  {
    //TODO: CHECK FIRMWARE
  }

  publishStatus();

}



void timeLoop()
{
  /*
  if (iotWebConf.getState() != iotwebconf::OnLine)
    return;
  // the second parameter is the timeout
  if(!getLocalTime(&timeinfo,5000)){
    Serial.println("Failed to obtain time");
    return;
  }
  */

}
#ifdef ESP8266
bool getLocalTime(struct tm * info, uint32_t ms)
{
    uint32_t start = millis();
    time_t now;
    while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            return true;
        }
         iotWebConf.delay(10);
    }
    return false;
}
#endif

void tempLoop()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  lux = lightMeter.readLightLevel();
  if (isnan(lux)) {
    Serial.println(F("Failed to read from BH1750  sensor!"));
    return;
  }
}

void internalLopp()
{
  if (now - lastSensorLoop < minStatus)
    return;

  timeLoop();
  tempLoop();

  lastSensorLoop = now;
}

void setup()
{

  Serial.begin(115200);
  while (!Serial)
    ;
  dht.begin();
  Wire.begin();
  lightMeter.begin();

  // iotWebConf INIT
  mqttGroup.addItem(&mqttServerParam);
  mqttGroup.addItem(&mqttPortParam);
  mqttGroup.addItem(&mqttUserNameParam);
  mqttGroup.addItem(&mqttUserPasswordParam);
  mqttGroup.addItem(&mqttPrefixPrefix);
  fanGroup.addItem(&fanSetPoint);
  fanGroup.addItem(&fanThreshold);

  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setApTimeoutMs(5000);

  pinMode(FACTORY_RESET_BUTTON_PIN, INPUT);
  iotWebConf.setConfigPin(FACTORY_RESET_BUTTON_PIN);

  iotWebConf.addParameterGroup(&fanGroup);
  iotWebConf.addParameterGroup(&mqttGroup);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);

  // -- Initializing the configuration.
  bool validConfig = iotWebConf.init();
  if (!validConfig)
  {
    mqttServerValue[0] = '\0';
    mqttUserNameValue[0] = '\0';
    mqttUserPasswordValue[0] = '\0';
    mqttPrefixValue[0] = '\0';
    mqttPortValue[0] = '\0';
    fanSetPointValue[0] = '\0';
    fanThresholdValue[0] = '\0';
  }

  String tmp = String(mqttPortValue);
  if (tmp.toInt())
  {
    mqttPort = tmp.toInt();
  }
  else
  {
    mqttPort = DEFAULT_MQTT_PORT;
  }

  tmp = String(fanSetPointValue);
  if (tmp.toFloat())
  {
    minStatus = tmp.toFloat() * 1000;
  }

  tmp = String(fanThresholdValue);
  if (tmp.toFloat())
  {
    maxStatus = tmp.toFloat() * 1000;
  }

  mqttStatusTopic = String(mqttPrefixValue) + "/status";
  mqttCmdTopic = String(mqttPrefixValue) + "/+/set";

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []
            { iotWebConf.handleConfig(); });
  server.onNotFound([]()
                    { iotWebConf.handleNotFound(); });
  // MQTT INIT
  mqttClient.setServer(mqttServerValue, mqttPort);
  mqttClient.setCallback(mqttMessageReceived);

  // NTPM INIT
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop()
{

  now = millis();

  internalLopp();

  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
  mqttClient.loop();

  // put your main code here, to run repeatedly:

  if (needMqttConnect)
  {

    if (connectMqtt())
    {
            needMqttConnect = false;
    }
  }
  else if ((iotWebConf.getState() == iotwebconf::OnLine) && (!mqttClient.connected()))
  {
    Serial.println("MQTT reconnect");
    connectMqtt();
  }
  else if (mqttClient.connected()) // si esta conectdo con mqtt
    publishStatus();

  if (needReset)
  {
    Serial.println("Rebooting after 1 second.");
    iotWebConf.delay(1000);
    ESP.restart();
  }
}