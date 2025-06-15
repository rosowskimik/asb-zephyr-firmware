#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>

#include "network.h"

LOG_MODULE_REGISTER(network);

K_SEM_DEFINE(network_connected, 0, 1);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Connection request failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
	}
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_DBG("Disconnection request (%d)", status->status);
	} else {
		LOG_INF("Disconnected");
	}
}

static void handle_ipv4_result(struct net_if *iface)
{
	int i = 0;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {

		char buf[NET_IPV4_ADDR_LEN];
		if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) {
			continue;
		}

		LOG_INF("IPv4 address: %s",
			net_addr_ntop(AF_INET,
				      &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr, buf,
				      sizeof(buf)));
		LOG_INF("Subnet: %s",
			net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[i].netmask, buf,
				      sizeof(buf)));
		LOG_INF("Router: %s",
			net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf)));
		LOG_INF("Lease time: %u seconds", iface->config.dhcpv4.lease_time);
	}

	k_sem_give(&network_connected);
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {

	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;

	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;

	case NET_EVENT_IPV4_ADDR_ADD:
		handle_ipv4_result(iface);
		break;

	default:
		break;
	}
}

void setup_network(void)
{
	net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT |
					     NET_EVENT_WIFI_DISCONNECT_RESULT);

	net_mgmt_init_event_callback(&ipv4_cb, wifi_mgmt_event_handler, NET_EVENT_IPV4_ADDR_ADD);

	net_mgmt_add_event_callback(&wifi_cb);
	net_mgmt_add_event_callback(&ipv4_cb);
}

void wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();

	struct wifi_connect_req_params wifi_params = {0};

	wifi_params.ssid = CONFIG_APP_WIFI_SSID;
	wifi_params.psk = CONFIG_APP_WIFI_PASS;
	wifi_params.ssid_length = strlen(CONFIG_APP_WIFI_SSID);
	wifi_params.psk_length = strlen(CONFIG_APP_WIFI_PASS);
	wifi_params.channel = WIFI_CHANNEL_ANY;
	wifi_params.security = WIFI_SECURITY_TYPE_PSK;
	wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ;
	wifi_params.mfp = WIFI_MFP_OPTIONAL;

	LOG_INF("Connecting to SSID: %s", wifi_params.ssid);

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params,
		     sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("WiFi Connection Request Failed");
	}

	net_dhcpv4_start(iface);

	k_sem_take(&network_connected, K_FOREVER);

	LOG_INF("WiFi Connected");
}
