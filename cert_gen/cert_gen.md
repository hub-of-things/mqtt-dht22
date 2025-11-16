# CERTIFICADOS NE LE PROGRAMA

Si bien en el programa funciona no carga mas de un certiicado, sigiere hacer lel arhcibo embebido

https://github.com/espressif/arduino-esp32/issues/10949


Se puede poner directamente en el porgrama y actualizar el firmware con todo

https://github.com/thingsboard/thingsboard-client-sdk/issues/134

# GENERANDO LSO CERTIFICADOS

1. Obtener el certificado del servidor
```bash
openssl s_client -connect your_mqtt_server_address:mqtt_tls_port -showcerts </dev/null 2>/dev/null | sed -n '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/p' > ./data/cacert_all.pem

```

Despues se ejecuta el que genera el cabundle de expresiff `x509_crt_bundle` que es un binario con todos los pem y der dentro de `.\data` y que es referenciado por `/platformio.ini` 

```bash
pip install -r requirements.txt 
python gen_crt_bundle.py -i ../data
```

El script `certs-from-mozilla.py` es recomendando por Hivemq y baja todos los certificados de la CA pero termina pesando como 8 megas.

## How to use CA Cert Bundle in Arduino ESP32?

The process for using the ESP32 `setCACertBundle()` was described in Arduino Library README.md of [WiFiClientSecure](https://github.com/espressif/arduino-esp32/tree/release/v2.x/libraries/WiFiClientSecure#using-a-bundle-of-root-certificate-authority-certificates) Library for Arduino ESP32 framework release 2.x. For some reason it has been reduced to a simple vague paragraph in release 3.x [README.md](https://github.com/espressif/arduino-esp32/tree/master/libraries/NetworkClientSecure) when the `WiFiCleintSecure` library is re-factored to `NetworkClientSecure` library. Espressif lately like to do that and sometime break the backward compatibility or remove something useful from its Github without explanation.

### Create a CA Cert Bundle

In order to use a cert bundle, first you would need to generate a cert bundle. You could generate it yourself, or get CA certificates extracted from Mozilla from [here](https://curl.se/docs/caextract.html). The download Mozilla CA certificate store `cacert.pem` is in PEM format and is around 200KB uncompressed.

Esp-idf has a [python utility](https://github.com/espressif/esp-idf/blob/master/components/mbedtls/esp_crt_bundle/gen_crt_bundle.py) for generating a binary version of CA bundle which is only around 64kB. Download the `gen_crt_bundle.py` (you might need to install python package `cryptography` with `pip3 install cryptography` before you can run the python script).

Run the python script with the following command line to generate the binary file:

```python
python3 gen_crt_bundle.py -i cacert.pem
```

This should generated a file called `x509_crt_bundle`, make a directory in your *project directory* and move the created bundle into the directory.

```shell
mv x509_crt_bundle ../data
```

### Setup PlatformIO

> The code that I'm going to shown work for PlatformIO, but does not work for Arduino IDE (I don't know why and in fact I don't really use Arduino IDE these days so I never spent the time to find out why).

Assuming you had PlatfromIO installed and have created an Arduino project. Add build flag in `platformio.ini`:

```shell
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
board_build.embed_files = data/cert/x509_crt_bundle
monitor_speed = 115200
```

### Call setCACertBundle() before making an secure connection

The code for using the `setCACertBundle()` is actually quite simple and similar to the usage of `WiFiClientSecure` example, with only two differences:

1. add `extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");` to your sketch;
2. call `client.setCACertBundle(rootca_crt_bundle_start);` before making a client connection.

Here is the complete example sketch that I used for testing, it access the httpbin.org test site (you could try other sites) and httpbin server would return a response in json format.

```cpp
#include <WiFiClientSecure.h>

const char* ssid     = "wifi ssid";
const char* password = "wifi password";

const char*  server = "httpbin.org";

extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Connected");

  // client.setCACert(ISRG_Root_X1_CA);
  client.setCACertBundle(rootca_crt_bundle_start);
  // client.setCertificate(test_client_cert); // for client verification
  // client.setPrivateKey(test_client_key);  // for client verification

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("Connected to server!");
    // Make a HTTP request:
    client.printf("GET https://%s/get HTTP/1.1\n", server);
    client.printf("Host: %s\n", server);
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    // read the response body
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
  }
}

void loop() {
  // do nothing
}
```

The screen capture shown the overall directory structure of platformIO setup as well as the response received from the test server (httpbin.org).
