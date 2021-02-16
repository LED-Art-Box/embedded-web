#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define PIN 4
#ifndef VERSION
  #define VERSION "v0.0.0"
#endif

const char *MQTT_SERVER = "broker.emqx.io";
const uint16_t MQTT_PORT = 1883;

const char *rootCACertificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
    "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
    "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
    "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
    "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
    "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
    "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
    "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
    "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
    "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
    "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
    "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
    "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
    "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
    "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
    "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
    "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
    "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
    "+OkuE6N36B9K\n"
    "-----END CERTIFICATE-----\n";

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(16, 16, PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
                                                   NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
                                               NEO_GRB + NEO_KHZ800);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
String clientId;

const char DRAW_TOPIC[] = "lieblingswelt/draw";
const char CONNECT_TOPIC[] = "lieblingswelt/draw/connect";
const char UPDATE_TOPIC[] = "lieblingswelt/draw/update";

uint8_t data[16][16][3] = {0};

void startWifi()
{
  Serial.println("Connecting Wifi");

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setEnableConfigPortal(false);
  wifiManager.setTimeout(0);
  uint8_t i = 0;
  while (!wifiManager.autoConnect("draw", "drawdraw") && i++ < 3)
  {
    Serial.println("Retry autoConnect");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
  }
  if (!WiFi.isConnected())
  {
    wifiManager.setEnableConfigPortal(true);
    wifiManager.autoConnect("draw", "drawdraw");
  }

  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  Serial.printf("Version: %s\n", VERSION);

  matrix.begin();
  matrix.setBrightness(30);
  matrix.setRotation(1);
  matrix.fill(0);
  matrix.show();

  startWifi();

  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void sync()
{
  Serial.print("Sync: ");
  uint8_t payload[5];
  for (uint8_t x = 0; x < matrix.width(); x++)
  {
    for (uint8_t y = 0; y < matrix.height(); y++)
    {
      if (data[x][y][0] == 0 && data[x][y][1] == 0 && data[x][y][2] == 0)
      {
        Serial.print(".");
        continue;
      }
      payload[0] = x;
      payload[1] = y;
      payload[2] = data[x][y][0];
      payload[3] = data[x][y][1];
      payload[4] = data[x][y][2];
      mqttClient.publish(DRAW_TOPIC, payload, sizeof(payload));
      Serial.print(",");
    }
  }
  Serial.println();
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  if (strcmp(topic, DRAW_TOPIC) == 0)
  {
    if (length != 5)
    {
      Serial.print("Wrong msg size: ");
      Serial.println(length);
      return;
    }

    uint8_t x = message[0];
    uint8_t y = message[1];
    uint16_t color = matrix.Color(message[2], message[3], message[4]);

    if (x >= matrix.width() || y >= matrix.height())
    {
      Serial.printf("Invalid coordinates %d,%d\n", x, y);
      return;
    }

    data[x][y][0] = message[2];
    data[x][y][1] = message[3];
    data[x][y][2] = message[4];

    matrix.drawPixel(x, y, color);
    matrix.show();
  }
  else if (strcmp(topic, CONNECT_TOPIC) == 0)
  {
    sync();
  }
  else if (strcmp(topic, UPDATE_TOPIC) == 0)
  {
    update();
  }
  else
  {
    Serial.print("Wrong topic?! ");
    Serial.println(topic);
  }
}

void reconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("connected");
      mqttClient.subscribe(DRAW_TOPIC);
      mqttClient.subscribe(CONNECT_TOPIC);
      mqttClient.subscribe(UPDATE_TOPIC);
      sync();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

String followRequest(String url, int count, int times) {
  if (count >= times) { //stop following redirects after n times
    return url;
  }

  const char *headerKeys[] = {"Location"};
  const size_t numberOfHeaders = 1;

  HTTPClient http;

  http.begin(url);
  http.collectHeaders(headerKeys, numberOfHeaders);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
      http.end();
      return url;
  } else if (httpResponseCode == 302) {
    String nextUrl = http.header(headerKeys[0]);
    http.end();
    return followRequest(nextUrl, count++, times);
  } else {
    return url;
  }
}


void update()
{
  Serial.printf("Updating from %s to latest firmware\n", VERSION);

  String resourceUrl = followRequest("https://github.com/LED-Art-Box/embedded-web/releases/latest/download/firmware.bin", 0, 4);

  WiFiClientSecure wiFiClientSecure;
  wiFiClientSecure.setCACert(rootCACertificate);
  t_httpUpdate_return ret = httpUpdate.update(wiFiClientSecure, resourceUrl);

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}

void loop()
{
  if (!mqttClient.connected())
  {
    reconnect();
  }
  mqttClient.loop();
}