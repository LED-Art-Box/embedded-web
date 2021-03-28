#include <WiFiManager.h>
#include <PubSubClient.h>

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "matrix.h"
#include "constants.h"

#define PIN 4
#define MATRIX_WIDTH 16
#define MATRIX_HEIGHT 16

#define MAX_MESSAGE_SIZE 768

Matrix matrix;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
String clientId;

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

void updateIfNeeded()
{
  Serial.printf("Version: %s\n", VERSION);
  String url = LATEST_RELEASE_INFO_URL;

  HTTPClient http;
  http.begin(url);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200)
  {
    StaticJsonDocument<2000> doc;
    deserializeJson(doc, http.getStream());

    String latest = doc["tag_name"].as<String>();

    if (latest != VERSION)
    {
      Serial.printf("Needs Update from %s to %s\n", VERSION, latest.c_str());
      update();
    }
  }

  http.end();
}

void sync()
{
  Serial.print("Sync: ");
  uint8_t payload[5];
  matrix.foreach([&](uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (r != 0 || g != 0 || b != 0) {
      payload[0] = x;
      payload[1] = y;
      payload[2] = r;
      payload[3] = g;
      payload[4] = b;

      mqttClient.publish(MQTT_DRAW_TOPIC, payload, sizeof(payload));
      Serial.print(",");              
    } else {
      Serial.print(".");
    }
  });
  Serial.println();
}

void drawSinglePixel(byte *message)
{
  uint8_t x = message[0];
  uint8_t y = message[1];

  matrix.draw(x, y, message[2], message[3], message[4]);
}


void drawFullImage(const byte *message, size_t length) {

  const uint16_t *image = (const uint16_t*)message;
  matrix.draw565Image(image, length / 2);
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  if (strcmp(topic, MQTT_DRAW_TOPIC) == 0)
  {
    if (length == 5) {
      drawSinglePixel(message);
    } else if (length == 512) {
      drawFullImage(message, length);
    } else {
      Serial.print("Wrong msg size: ");
      Serial.println(length);
      return;
    }
  }
  else if (strcmp(topic, MQTT_CONNECT_TOPIC) == 0)
  {
    sync();
  }
  else if (strcmp(topic, MQTT_UPDATE_TOPIC) == 0)
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
      mqttClient.subscribe(MQTT_DRAW_TOPIC);
      mqttClient.subscribe(MQTT_CONNECT_TOPIC);
      mqttClient.subscribe(MQTT_UPDATE_TOPIC);
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

String followRedirect(String url, int count, int times)
{
  if (count >= times)
  { //stop following redirects after n times
    return url;
  }

  const char *headerKeys[] = {"Location"};
  const size_t numberOfHeaders = 1;

  HTTPClient http;

  http.begin(url);
  http.collectHeaders(headerKeys, numberOfHeaders);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200)
  {
    http.end();
    return url;
  }
  else if (httpResponseCode == 302)
  {
    String nextUrl = http.header(headerKeys[0]);
    http.end();
    return followRedirect(nextUrl, count++, times);
  }
  else
  {
    return url;
  }
}

void update()
{
  Serial.println("Starting update");
  String resourceUrl = followRedirect(LATEST_FIRMWARE_URL, 0, 4);

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

void setup()
{
  Serial.begin(115200);

  matrix.init();

  startWifi();
  updateIfNeeded();

  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  uint16_t port = atoi(MQTT_BROKER_PORT);
  mqttClient.setServer(MQTT_BROKER_HOST, port);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(MAX_MESSAGE_SIZE);
}

void loop()
{
  if (!mqttClient.connected())
  {
    reconnect();
  }
  mqttClient.loop();
}
