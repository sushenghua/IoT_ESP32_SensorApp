# SensorApp

## Introduction

SensorApp is an IoT device application which is used to send messages back and forth (over MQTT protocol) to corresponding mobile App.
The mobile side App may read sensor data from device or send command to it. The real device show is as the following image:

<img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo/dev1.jpg" width="500"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo/dev2.jpg" width="500">

Some short demo videos are listed at https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo, more information about the device can be found [here](https://htmlpreview.github.io/?https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo/webpage/index.html#second). And the mobile apps are [iOS App](https://itunes.apple.com/cn/app/q-monitor/id1318735622) and [Android App](http://appsgenuine.com/download/qmonitor.apk)

## Program features

As the SensorApp device contains an ESP32 chip (integrated in the ESP32-WROOM-32D module) as its MCU, this SensorApp IoT application is mainly
build on framework [eps-idf](https://github.com/espressif/esp-idf.git) by Espressif Systems. To make the underlayer C interface easier to use,
the application provides some components that wrap the eps-idf C interface into corresponding C++ classes. Besides the official esp-idf framework,
this application also uses some other great libraries like [Mongoose Networking](https://github.com/cesanta/mongoose),
[ILI9341 LCD Driver](https://github.com/adafruit/Adafruit_ILI9341), [QRCode](https://github.com/nayuki/QR-Code-generator) and et al. The following
lists the features in this application:

- [SPI](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/Spi): Wraps esp-idf C spi

- [I2C](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/I2c): Wraps eps-idf C i2c

- [UART](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/Uart): Wraps eps-idf C uart

- [Wifi](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/Wifi): Wrap eps-idf C wifi

- [MQTT Client](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/MessageProtocol/MqttClient.h): Implemented with the mongoose lib

- [HTTP Server](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/MessageProtocol/HttpServer.h): Implemented with the mongoose lib

- [Command Engine](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/CmdEngine): Interpretes remote command bytes or strings and executes it accordingly

- [App Updater](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/AppUpdater): Wraps the esp-idf OTA mechanism, data transmitted over MQTT channel

- [Display Controller](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/DisplayController): Management of LCD device control and screen content display

- [Power Manager](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/PowerManager): Device battery power management with TI BQ24295 chip

- [Pripheral Sensor Interface](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/Sensor): CO2 (over Uart), PM (over Uart), SHT3xSensor (over I2c), TSL2561 (over I2c), MPU6050 (over I2c)

- [Application System](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/Application): Combines all stuff together and launches different tasks accordingly


## Hardware

#### Board, Chips, Sensors, Programmer

The **SensorApp main board** chooses ESP32 chip (in [ESP32-WROOM-32D](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf) module)
as its MCU which ingrates Wifi and Bluetooth/BLE providing greate and rich functionalities. Based on that, this board integrates battery management chip
[TI BQ24295](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/bq24295.pdf),
DC-DC converter [TI TPS62130](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/tps62130.pdf),
Montion tracking [InvenSense MPU6050](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/MPU-6050_DataSheet_V3.4.pdf),
Load switch [Micrel MIC9409X](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/MIC9409.pdf),
ESD protection [TI TPD4E1U06](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/tpd4e1u06.pdf)

The **extended FPC** contains
Humidity and Temperature Sensor [SENSIRION SHT3x](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/Sensirion_Humidity_Datasheet_SHT3x_DIS-539854.pdf),
Light to digital converter [TAOS TSL2561](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/TSL2561.pdf)

The separated **off-board sensors** used in this application are
Air pollution particles sensor [Plantower PM](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/PMS5003S_V2.2.pdf)
and Carbon dioxide gas sensor [Plantower CO2](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/ChipDatasheet/CO2_Sensor_V1.3.pdf)

The **programmer board** is merely the CP2102 related parts from Espressif's official [ESP32-DevKitC](https://www.espressif.com/sites/default/files/documentation/esp32-devkitc-v2_reference_design_0.zip) development board

The following images show the board, chips, sensors and the programmer

<img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/BoardSensor/Board1.jpg" width="400"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/BoardSensor/Board2.jpg" width="400"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/BoardSensor/Board3.jpg" width="400"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/BoardSensor/Sensor.jpg" width="400"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/BoardSensor/Programmer1.jpg" width="400"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/BoardSensor/Programmer2.jpg" width="400">

### Schematic and BOM

The schematic of board can be found in the following list:
- [main board](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/CircuitSchematic/MainBoard.pdf)
- [extended board](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/CircuitSchematic/ExpandBoard.pdf)
- [sensor FPC](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/CircuitSchematic/SensorFPC.pdf)
- [usb programmer](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/CircuitSchematic/EspUsb.pdf)

For the BOM list:
- [BOM](https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/CircuitSchematic/bom.html)


### Build and Run

To build this project, latest [esp-idf](https://github.com/espressif/esp-idf.git) is required to setup properly, just by following the [step-by-step instructions](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation-step-by-step). An [ESP32-DevKitC](https://www.espressif.com/en/products/devkits/esp32-devkitc) board or similar compitable development boards can be used to deploy the firmware of this
project(switch to branch [**'esp32_devkit_board'**](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/esp32_devkit_board)). As these boards don't have the peripheral chips as this SensorApp board has, some related tasks in file [System.cpp](https://github.com/sushenghua/IoT_ESP32_SensorApp/tree/master/components/Application/System.cpp) had been commented out. Also, a MQTT broker is required
to depoly on a server, [Mosquitto](https://mosquitto.org) is used in this project. To make the OTA update enabled, a MQTT subscriber service is needed to
response the Device App (SensorApp) or the mobile App.

build and flash the firmware
```bash
git clone git@github.com:sushenghua/IoT_ESP32_SensorApp.git
cd IoT_ESP32_SensorApp
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py flash monitor
```

The following images show the different screens of a running device

<img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo/DeviceScreen1.jpg" width="250"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo/DeviceScreen2.jpg" width="250"><img src="https://github.com/sushenghua/IoT_ESP32_SensorDoc/blob/master/Demo/DeviceScreen3.jpg" width="250">


### MQTT broker

In this project, [Mosquitto](https://mosquitto.org) is used as the MQTT broker, just follow [Steve's Internet Guide](http://www.steves-internet-guide.com/mossquitto-conf-file/) to setup the configuration. This site also provides instructions about how to apply [user password in mosquitto](http://www.steves-internet-guide.com/mqtt-username-password-example/) and [Certificates in mosquitto](http://www.steves-internet-guide.com/creating-and-using-client-certificates-with-mqtt-and-mosquitto/). A handy script to generate CA files can be found at [here](https://github.com/sushenghua/IoT_Firmware_MQTT_Updater/blob/master/ca/genCA.sh). For more learning resource about MQTT, please visit [HiveMQ](https://www.hivemq.com/blog/how-to-get-started-with-mqtt)


### OTA service over MQTT

Another python project [IoT_Firmware_MQTT_Updater](https://github.com/sushenghua/IoT_Firmware_MQTT_Updater) implemented the OTA over MQTT. It's simply a MQTT subscriber/publisher based service, as it will setup an unique communication channel between the device and this service node to transmit data upon the OTA request.
