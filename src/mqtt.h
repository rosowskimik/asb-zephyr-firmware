#ifndef _MQTT_H
#define _MQTT_H

#include <zephyr/toolchain.h>

#include "sensor.h"

#define MQTT_CLIENT_ID   "zephyr_" STRINGIFY(CONFIG_APP_SENSOR_ID)
#define MQTT_BUFFER_SIZE 512
#define MQTT_SERVER_PORT CONFIG_APP_MQTT_PORT
#define MQTT_SERVER_ADDR CONFIG_APP_MQTT_IP

#define MQTT_TOPIC "env/sensor/" STRINGIFY(CONFIG_APP_SENSOR_ID) "/data"

void setup_mqtt(void);

int publish_mqtt(const struct sensor_aggregated_data *const data);

#endif // _MQTT_H
