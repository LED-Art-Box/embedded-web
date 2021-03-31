#include <WiFiManager.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "matrix.h"
#include "mqtt.h"
#include "updater.h"
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
    OTAUpdater().updateIfNeeded();
  });
}

void setup()
{
  Serial.begin(115200);

  matrix.init();

  startWifi();
  OTAUpdater().updateIfNeeded();

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
