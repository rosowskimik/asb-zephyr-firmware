#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <stdint.h>
#include <string.h>

#include "mqtt.h"
#include "sensor.h"

LOG_MODULE_REGISTER(mqtt);

static uint8_t rx_buffer[MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_BUFFER_SIZE];

static struct mqtt_client client;
static struct sockaddr_storage broker;

static void get_mqtt_payload(const struct sensor_aggregated_data *const data, uint8_t *buf,
			     size_t size)
{
	snprintk(buf, size,
		 // clang-format off
		 "{"
			 "\"temp\":"
				 "{\"min\":%d.%d,\"max\":%d.%d,\"med\":%d.%d},"
			 "\"pressure\":"
				 "{\"min\":%d.%d,\"max\":%d.%d,\"med\":%d.%d},"
			 "\"humidity\":"
				 "{\"min\":%d.%d,\"max\":%d.%d,\"med\":%d.%d}"
		 "}",
		 // clang-format on
		 SENSOR_DATA_FMT(data->temp.min), SENSOR_DATA_FMT(data->temp.max),
		 SENSOR_DATA_FMT(data->temp.median), SENSOR_DATA_FMT(data->press.min),
		 SENSOR_DATA_FMT(data->press.max), SENSOR_DATA_FMT(data->press.median),
		 SENSOR_DATA_FMT(data->humid.min), SENSOR_DATA_FMT(data->humid.max),
		 SENSOR_DATA_FMT(data->humid.median));

	LOG_INF("Payload: %s", buf);
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		LOG_INF("MQTT client connected");

		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected");
		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error %d", evt->result);
			break;
		}

		LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {.message_id =
								    evt->param.pubrec.message_id};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			LOG_ERR("Failed to send MQTT PUBREL: %d", err);
		}

		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error %d", evt->result);
			break;
		}

		LOG_INF("PUBCOMP packet id: %u", evt->param.pubcomp.message_id);

		break;

	case MQTT_EVT_PINGRESP:
		LOG_INF("PINGRESP packet");
		break;

	default:
		break;
	}
}

void setup_mqtt(void)
{
	mqtt_client_init(&client);

	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(MQTT_SERVER_PORT);
	inet_pton(AF_INET, MQTT_SERVER_ADDR, &broker4->sin_addr);

	client.broker = &broker;
	client.evt_cb = mqtt_evt_handler;
	client.client_id.utf8 = MQTT_CLIENT_ID;
	client.client_id.size = strlen(MQTT_CLIENT_ID);
	client.password = NULL;
	client.user_name = NULL;
	client.protocol_version = MQTT_VERSION_3_1_1;

	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

	client.transport.type = MQTT_TRANSPORT_NON_SECURE;
}

int publish_mqtt(const struct sensor_aggregated_data *const data)
{
	static uint8_t payload[512];
	struct mqtt_publish_param param;
	int ret;

	ret = mqtt_connect(&client);
	if (ret) {
		LOG_ERR("Failed to connect to MQTT (%d)", ret);
		return ret;
	}

	k_sleep(K_MSEC(300));
	mqtt_input(&client);

	param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	param.message.topic.topic.utf8 = MQTT_TOPIC;
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);

	get_mqtt_payload(data, payload, sizeof(payload));

	param.message.payload.data = payload;
	param.message.payload.len = strlen(param.message.payload.data);
	param.message_id = 0;
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	ret = mqtt_publish(&client, &param);
	if (ret) {
		LOG_ERR("Failed to publish MQTT message (%d)", ret);
		return ret;
	}

	ret = mqtt_disconnect(&client);
	if (ret) {
		LOG_ERR("Failed to disconnect from MQTT (%d)", ret);
	}

	return ret;
}
