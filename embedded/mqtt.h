#ifndef LEDART_MQTT_H_
#define LEDART_MQTT_H_

#include <forward_list>
#include <functional>

#include <PubSubClient.h>

#define MAX_MESSAGE_SIZE 768

uint32_t chip_id();

struct ByteArrayWrapper
{
public:
  const byte *data;
  const size_t length;

  const byte &operator[](int index) const;
};

using MqttCallback = std::function<void(const char *topic, const ByteArrayWrapper &payload)>;

class Subscription
{
public:
  Subscription(const char *topic, MqttCallback callback);

  bool topic_matches(const char *topic) const;
  void trigger(const char *topic, const ByteArrayWrapper &payload);

  const String &topic() const;

  int qos() const;

private:
  String topic_;
  int qos_ = 0;
  MqttCallback callback_;
};

class MQTTClient
{
public:
  MQTTClient(PubSubClient &client);
  MQTTClient(PubSubClient &client, const String &client_id);

  void connect();
  void loop();

  void subscribe(const char *topic, MqttCallback callback);

  void on_connect(std::function<void(void)> callback);

private:
  PubSubClient *const client_;
  String client_id_;

  std::function<void(void)> on_connect_callback_ = []() {};

  std::forward_list<Subscription> subscriptions_;

  void callback(const char *topic, const byte *msg, unsigned int length);
  void resubscribe();
  void setup_callback();
};

#endif