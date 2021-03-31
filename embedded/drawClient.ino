#include <WiFiManager.h>
#include <PubSubClient.h>

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "matrix.h"
#include "mqtt.h"
#include "constants.h"

#define MAX_MESSAGE_SIZE 768

Matrix matrix;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MQTTClient mqtt(mqttClient);
String clientId;
WiFiManager wifiManager;

void startWifi()
{
  Serial.println("Connecting Wifi");

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
  matrix.foreach ([&](uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (r != 0 || g != 0 || b != 0)
    {
      uint8_t payload[5] = {x, y, r, g, b};

      mqttClient.publish(MQTT_DRAW_TOPIC, payload, sizeof(payload));
      Serial.print(",");
    }
    else
    {
      Serial.print(".");
    }
  });
  Serial.println();
}

struct DrawMessage
{
  union
  {
    struct
    {
      uint8_t x;
      uint8_t y;
      uint8_t r;
      uint8_t g;
      uint8_t b;
    };
    uint8_t payload[5];
  };
};

void drawSinglePixel(const byte *payload)
{
  auto *msg = (const DrawMessage *)payload;
  matrix.draw(msg->x, msg->y, msg->r, msg->g, msg->b);
}

void drawFullImage(const byte *message, size_t length)
{
  const uint16_t *image = (const uint16_t *)message;
  matrix.draw565Image(image, length / 2);
}

void subscribe_topics()
{
  mqtt.subscribe(MQTT_DRAW_TOPIC, [](const char *, const ByteArrayWrapper &payload) {
    if (payload.length == 5)
    {
      drawSinglePixel(payload.data);
    }
    else if (payload.length == 512)
    {
      drawFullImage(payload.data, payload.length);
      Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    }
    else
    {
      Serial.print("Wrong msg size: ");
      Serial.println(payload.length);
      return;
    }
  });

  mqtt.subscribe(MQTT_CONNECT_TOPIC, [](const char *, const ByteArrayWrapper &) {
    sync();
  });

  mqtt.subscribe(MQTT_UPDATE_TOPIC, [](const char *, const ByteArrayWrapper &) {
    update();
  });
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

  Serial.printf("%08X\n", chip_id());

  mqtt.on_connect([]() {
    sync();
  });
  subscribe_topics();

  mqtt.connect();
}

void loop()
{
  mqtt.loop();
}
