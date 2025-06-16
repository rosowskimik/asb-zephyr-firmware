#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include "sensor.h"

LOG_MODULE_REGISTER(sensor);

const static struct device *const dev = DEVICE_DT_GET(DT_ALIAS(env_sensor));

struct reading {
	struct sensor_value temp;
	struct sensor_value press;
	struct sensor_value humid;
};

static struct reading readings[CONFIG_APP_SAMPLES_PER_READING] = {0};

static int compare_sensor_values(const void *x, const void *y)
{
	const struct sensor_value *a = *(const struct sensor_value **)x;
	const struct sensor_value *b = *(const struct sensor_value **)y;

	int ret = (a->val1 > b->val1) - (a->val1 < b->val1);

	return ret != 0 ? ret : (a->val2 > b->val2) - (a->val2 < b->val2);
}

int setup_sensor(void)
{
	if (dev == NULL) {
		LOG_ERR("No sensor device found");
		return -ENODEV;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("Device %s not ready", dev->name);
		return -ENODEV;
	}

	LOG_INF("Sensor found");

	return 0;
}

int read_sensor(int i)
{
	if (i > CONFIG_APP_SAMPLES_PER_READING) {
		LOG_ERR("Reading out of range");
		return -ERANGE;
	}

	struct reading *data = &readings[i];

	int rc = sensor_sample_fetch(dev);
	if (rc != 0) {
		LOG_ERR("Failed to fetch sensor data");
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &data->temp);
	if (rc != 0) {
		LOG_ERR("Failed to read temperature data");
	}
	rc = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &data->press);
	if (rc != 0) {
		LOG_ERR("Failed to read pressure data");
	}
	rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &data->humid);
	if (rc != 0) {
		LOG_ERR("Failed to read humidity data");
	}

	LOG_DBG("Temperature: " SENSOR_DATA_PRI, SENSOR_DATA_FMT(data->temp));
	LOG_DBG("Pressure: " SENSOR_DATA_PRI, SENSOR_DATA_FMT(data->press));
	LOG_DBG("Humidity: " SENSOR_DATA_PRI, SENSOR_DATA_FMT(data->humid));

	return 0;
}

void get_aggregated_data(struct sensor_aggregated_data *data)
{
	struct sensor_value *vals[CONFIG_APP_SAMPLES_PER_READING];

	for (int i = 0; i < CONFIG_APP_SAMPLES_PER_READING; ++i) {
		vals[i] = &readings[i].temp;
	}

	qsort(vals, CONFIG_APP_SAMPLES_PER_READING, sizeof(struct sensor_value *),
	      compare_sensor_values);

	data->temp = (struct entry){.min = *vals[0],
				    .max = *vals[CONFIG_APP_SAMPLES_PER_READING - 1],
				    .median = *vals[CONFIG_APP_SAMPLES_PER_READING / 2]};

	for (int i = 0; i < CONFIG_APP_SAMPLES_PER_READING; ++i) {
		vals[i] = &readings[i].press;
	}

	qsort(vals, CONFIG_APP_SAMPLES_PER_READING, sizeof(struct sensor_value *),
	      compare_sensor_values);

	data->press = (struct entry){.min = *vals[0],
				     .max = *vals[CONFIG_APP_SAMPLES_PER_READING - 1],
				     .median = *vals[CONFIG_APP_SAMPLES_PER_READING / 2]};

	for (int i = 0; i < CONFIG_APP_SAMPLES_PER_READING; ++i) {
		vals[i] = &readings[i].humid;
	}

	qsort(vals, CONFIG_APP_SAMPLES_PER_READING, sizeof(struct sensor_value *),
	      compare_sensor_values);

	data->humid = (struct entry){.min = *vals[0],
				     .max = *vals[CONFIG_APP_SAMPLES_PER_READING - 1],
				     .median = *vals[CONFIG_APP_SAMPLES_PER_READING / 2]};
}
