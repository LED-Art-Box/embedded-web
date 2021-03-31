#include "mqtt.h"

#ifndef VERSION
#define VERSION "v0.0.0"
#endif

#ifndef MQTT_BROKER_HOST
#define MQTT_BROKER_HOST "broker.emqx.io"
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT "1883"
#endif

#ifndef MQTT_CONNECT_TOPIC
#define MQTT_CONNECT_TOPIC "lieblingswelt/draw/connect"
#endif

#ifndef MQTT_UPDATE_TOPIC
#define MQTT_UPDATE_TOPIC "lieblingswelt/draw/update"
#endif

#ifndef MQTT_DRAW_TOPIC
#define MQTT_DRAW_TOPIC "lieblingswelt/draw"
#endif

#ifndef FIRMWARE_FILE_NAME
#define FIRMWARE_FILE_NAME "firmware.bin"
#endif

uint32_t chip_id()
{
    uint32_t id = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return id;
}

void unique_id()
{
    uint32_t id = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    Serial.printf("%08X\n", id);
}

constexpr byte BYTE_ARRAY_OUT_OF_BOUNDS_RETURN = 0;

const byte &ByteArrayWrapper::operator[](int index) const
{
    if (index < 0 || index > this->length)
    {
        return BYTE_ARRAY_OUT_OF_BOUNDS_RETURN;
    }

    return this->data[index];
}

Subscription::Subscription(const char *topic, MqttCallback msg_callback) : topic_(topic), callback_(std::move(msg_callback)){};

bool Subscription::topic_matches(const char *other) const
{
    return this->topic_ == other;
}

void Subscription::trigger(const char *topic, const ByteArrayWrapper &payload)
{
    this->callback_(topic, payload);
}

const String &Subscription::topic() const
{
    return this->topic_;
}

int Subscription::qos() const
{
    return this->qos_;
}

MQTTClient::MQTTClient(PubSubClient &client) : MQTTClient(client, "ESP32Client-" + String(random(0xffff), HEX))
{
}

MQTTClient::MQTTClient(PubSubClient &client, const String &client_id) : client_(&client)
{
    this->client_id_ = client_id;
    uint16_t port = atoi(MQTT_BROKER_PORT);
    client_->setServer(MQTT_BROKER_HOST, port);
    client_->setBufferSize(MAX_MESSAGE_SIZE);

    this->setup_callback();
}

void MQTTClient::on_connect(std::function<void(void)> callback)
{
    this->on_connect_callback_ = std::move(callback);
}

void MQTTClient::setup_callback()
{
    using namespace std::placeholders;
    client_->setCallback(std::bind(&MQTTClient::callback, this, _1, _2, _3));
}

void MQTTClient::connect()
{
    if (!client_->connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client_->connect(client_id_.c_str()))
        {
            Serial.println("connected");
            this->setup_callback();
            this->resubscribe();
            this->on_connect_callback_();
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client_->state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void MQTTClient::resubscribe()
{
    for (auto sub : subscriptions_)
    {
        client_->subscribe(sub.topic().c_str(), sub.qos());
    }
}

void MQTTClient::loop()
{
    if (!client_->connected())
    {
        this->connect();
    }
    client_->loop();
}

void MQTTClient::subscribe(const char *topic, MqttCallback callback)
{
    subscriptions_.emplace_front(topic, std::move(callback));
    if (client_->connected())
    {
        client_->subscribe(subscriptions_.front().topic().c_str(), subscriptions_.front().qos());
    }
}

void MQTTClient::callback(const char *topic, const byte *msg, unsigned int length)
{
    for (auto sub : subscriptions_)
    {
        if (sub.topic_matches(topic))
        {
            sub.trigger(topic, ByteArrayWrapper{msg, length});
        }
    }
}