#ifndef LEDART_CONSTANTS_H_
#define LEDART_CONSTANTS_H_

#include <Arduino.h>

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

// github urls for releases
extern String LATEST_FIRMWARE_URL;
extern const char LATEST_RELEASE_INFO_URL[];

// github cert
extern const char *rootCACertificate;

#endif
