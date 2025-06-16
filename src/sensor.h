#ifndef _SENSOR_H
#define _SENSOR_H

#include <zephyr/toolchain.h>
#include <zephyr/drivers/sensor.h>

#define READ_PRECISION 3

// clang-format off
#define SENSOR_DATA_PRI "%d.%0" STRINGIFY(READ_PRECISION) "d"
#define SENSOR_DATA_FMT(data) \
	(data).val1, ((data).val2 / (10 * (6 - READ_PRECISION)))

#define ENTRY_PRI \
	"min: " SENSOR_DATA_PRI \
	" max: " SENSOR_DATA_PRI \
	" median: " SENSOR_DATA_PRI

#define ENTRY_FMT(entry) \
	SENSOR_DATA_FMT((entry).min), \
	SENSOR_DATA_FMT((entry).max), \
	SENSOR_DATA_FMT((entry).median)
// clang-format on

struct entry {
	struct sensor_value min;
	struct sensor_value max;
	struct sensor_value median;
};

struct sensor_aggregated_data {
	struct entry temp;
	struct entry press;
	struct entry humid;
};

int setup_sensor(void);

int read_sensor(int reading);

void get_aggregated_data(struct sensor_aggregated_data *data);

#endif // _SENSOR_H
