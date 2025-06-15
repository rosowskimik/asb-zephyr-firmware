#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "network.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	LOG_INF("Environment Sensor Starting Up");

	setup_network();

	wifi_connect();

	for (;;) {
		k_sleep(K_FOREVER);
	}
}
