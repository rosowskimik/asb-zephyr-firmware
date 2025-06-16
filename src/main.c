#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "network.h"
#include "sensor.h"

LOG_MODULE_REGISTER(main);

static struct sensor_aggregated_data data = {0};

int main(void)
{
	LOG_INF("Environment Sensor Starting Up");

	if (setup_sensor()) {
		return 0;
	}

	/* setup_network(); */
	/**/
	/* wifi_connect(); */

	for (;;) {
		for (int i = 0; i < CONFIG_APP_SAMPLES_PER_READING; ++i) {
			read_sensor(i);

			k_sleep(K_SECONDS(5));
		}

		get_aggregated_data(&data);

		LOG_INF("Aggregated data");
		LOG_INF("  Temperature: " ENTRY_PRI, ENTRY_FMT(data.temp));
		LOG_INF("  Pressure: " ENTRY_PRI, ENTRY_FMT(data.press));
		LOG_INF("  Humidity: " ENTRY_PRI, ENTRY_FMT(data.humid));
	}
}
