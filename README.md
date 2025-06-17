# ASB MQTT Zephyr Firmware

## General Description

This is a ZephyrRTOS application for environment sensors connected through WiFi to the network.

This application performs the following functions:

- Connects to a preconfigured WiFi network and acquired IP address using DHCP.
- Reads environment sensor data at predefined intervals and processes it.
- Publishes aggregated sensor data to a configured MQTT broker.

## Required Hardware

This application was tested with [`XIAO ESP32C3`](https://docs.zephyrproject.org/4.1.0/boards/seeed/xiao_esp32c3/doc/index.html)
and [`XIAO ESP32C6`](https://docs.zephyrproject.org/4.1.0/boards/seeed/xiao_esp32c6/doc/index.html) boards
together with [`BME680`](https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors/bme680) environment sensor,
however it should work with any board that supports WiFi and environment sensor that provides ambient temperature, pressure and humidity channels.

### Sample Configuration (XIAO ESP32C3 + BME680)

1. Connecting sensor to the board

| XIAO ESP32C3 Pin | BME680 Pin | Purpose           |
| ---------------- | ---------- | ----------------- |
| 3V3              | VCC        | Power for sensor  |
| GND              | GND        | Common Ground     |
| D5 (SCL)         | SCL        | I2C Clock Line    |
| D4 (SDA)         | SDA        | I2C Data Line     |
| GND              | SDO        | Sets I2C Address  |
| Not Connected    | CS         | Chip Select (SPI) |

2. Setting up devicetree overlay

To setup a devicetree overlay for your board, create `<your_board>.overlay` file in the `boards` directory.
The `env-sensor` alias should be pointing to the sensor node.

```dts
// boards/xiao_esp32c3.overlay
/ {
    aliases {
        // Point env-sensor to target sensor
        env-sensor = &env_sensor;
    };
};

&i2c0 {
    status = "okay";
    // Add BME680 sensor on I2C
    env_sensor: bme680@76 {
        compatible = "bosch,bme680";
        reg = <0x76>;
        label = "BME680";
    };
};

// Enable WiFi
&wifi {
    status = "okay";
};
```

3. Set application specific options

In order to customize application specific options, you can either change `prj.conf` to edit configuration for all the boards, or create / edit `<your_board>.conf` file in the `boards` directory.

| Config Name             | Description                    | Type   | Default Value |
| ----------------------- | ------------------------------ | ------ | ------------- |
| APP_WIFI_SSID           | WiFi SSID                      | string | env_sensors   |
| APP_WIFI_PASS           | WiFi Passphrase                | string | test1234      |
| APP_MQTT_IP             | MQTT Broker IP address         | string | 10.20.30.1    |
| APP_MQTT_PORT           | MQTT Broker port               | int    | 1883          |
| APP_SENSOR_ID           | Sensor ID                      | int    | 0             |
| APP_READ_INTERVAL       | Sensor Read Interval (seconds) | int    | 10            |
| APP_SAMPLES_PER_READING | Number of samples to aggregate | int    | 3             |

```conf
# boards/xiao_esp32c3.conf
# Set sensor interval to 1 minute
CONFIG_APP_READ_INTERVAL=60
# Change WiFi SSID
CONFIG_APP_WIFI_SSID=my_super_ssid
# Change sensor ID
CONFIG_APP_SENSOR_ID=123
```

## Building and Flashing

In order to build the application, you will have to configure ZephyrRTOS build environment on your computer.
It's recommended that you do that using [Zephyr's official `Getting Started Guide`](https://docs.zephyrproject.org/4.1.0/develop/getting_started/index.html) for your platform of choice.

Below build instructions are describing the build process for ESP32 based boards (specifically XIAO ESP32C3).
If your board isn't based on that, it might be necessary to modify `west.yml` file, in order to fetch modules necessary for your board. For more information, refer to the [official Zephyr documentation](https://docs.zephyrproject.org/4.1.0/boards/index.html) for your board.

### Building

1. Clone this repository:

```sh
git clone https://github.com/rosowskimik/asb-zephyr-firmware.git
cd asb-zephyr-firmware
```

2. Initialize Zephyr workspace and fetch all the necessary components:

```sh
west init -l .
west update
west blobs fetch hal_espressif
```

3. Build the application

```sh
west build -b xiao_esp32c3
```

### Flashing

```sh
west flash
```

## Usage Instructions

The application expects to be able to connect to configured WiFi network with DHCP configured on it, as well as be able to access the specified MQTT broker in order to be able to publish environment data.

Below are instructions for setting up a Linux computer as wireless access point and MQTT broker.

### Requirements

- Linux computer with wireless interface
- `hostapd` for access point configuration
- `dnsmasq` as DHCP server
- `mosquitto` as MQTT broker

1. Setup network on wireless interface

Configure the wireless interface to be in the same network as your environment sensor.

```sh
sudo ip addr add 10.20.30.1 dev <wireless_interface>
sudo ip link set dev <wireless_interface> up
```

2. Setup Access Point

Configure your wireless interface to act as an access point to the network.
You can use provided `hostapd.conf` file from the `sample` directory.
You might have to modify the `interface`, `ssid` and `wpa_passphrase` values to match the ones specified by the application.

```sh
sudo hostapd sample/hostapd.conf
```

3. Setup DHCP server

Start DHCP server to provide dyniamic IP addresses in the same network as your wireless interface.

```sh
sudo dnsmasq \
    --interface=<wireless_interface> \
    --dhcp-range=10.20.30.10,10.20.30.254,infinite \
    --log-dhcp \
    --log-debug \
    --dhcp-ignore-clid \
    --dhcp-leasefile="/run/dhcp-zephyr.lease" \
    --dhcp-option=3,"10.20.30.1" \
    -d
```

4. Setup MQTT broker

Configure your computer to act as MQTT broker.
You can use provided `mosquitto.conf` file from the `sample` directory.
You might have to modify the `bind_interface` and `listener` values to match the ones specified by the application.

```sh
sudo mosquitto -c sample/mosquitto.conf
```

## License

This project is licensed under the Apache 2.0 License. See the `LICENSE` file in the root of the repository for more details.
