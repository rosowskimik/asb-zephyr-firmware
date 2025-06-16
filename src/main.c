#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensor.h"
#include "network.h"
#include "mqtt.h"

LOG_MODULE_REGISTER(main);

static struct sensor_aggregated_data data = {0};

int main(void)
{
	LOG_INF("Environment Sensor Starting Up");

	if (setup_sensor()) {
		return 0;
	}

	if (connect_wifi()) {
		return 0;
	}

	setup_mqtt();

	for (;;) {
		for (int i = 0; i < CONFIG_APP_SAMPLES_PER_READING; ++i) {
			read_sensor(i);

			if (i == CONFIG_APP_SAMPLES_PER_READING - 1) {
				get_aggregated_data(&data);

				LOG_INF("Aggregated data");
				LOG_INF("  Temperature: " ENTRY_PRI, ENTRY_FMT(data.temp));
				LOG_INF("  Pressure: " ENTRY_PRI, ENTRY_FMT(data.press));
				LOG_INF("  Humidity: " ENTRY_PRI, ENTRY_FMT(data.humid));

				publish_mqtt(&data);
			}

			k_sleep(K_SECONDS(CONFIG_APP_READ_INTERVAL));
		}
	}
}
