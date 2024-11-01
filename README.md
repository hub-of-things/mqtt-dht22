# (ESP8266 | ESP32 ) + DHT22 SENSOR + MQTT

This is a test proyect that captures DHT22 sensor readings and then send it to a MQTT server

## Features
- Configurable html console using  [IotWebConf](https://github.com/prampec/IotWebConf)
- JSON Payload to MQTT
- Supports ESP32 and ESP8266
- **TODO:** Hysteresis for temperature and humidity control.

## Dependencies

- [IotWebConf](https://github.com/prampec/IotWebConf) version 3.2.1
- [PubSubClient](https://github.com/knolleary/pubsubclient) version 2.8.0
- [ArduinoJson] (https://arduinojson.org/?utm_source=meta&utm_medium=library.properties) version 6.20.0