# Google Cloud Mbed OS SDK port library

This library provides the port of the google cloud platform IoT core embedded 'C' SDK for Mbed OS. It can be used to connect devices running Mbed OS to the Google IoT Core over MQTT.

To use this library, a Mbed OS application needs to
* get the default instance of NetworkInterface and connect to the Internet.
* ensure the clock is set to the actual date and time as credentials require that.

Then it will be able to use [APIs](https://googlecloudplatform.github.io/iot-device-sdk-embedded-c/bsp/html/index.html) from the Google cloud IoT Device SDK.

An example demonstrating the use of this library has been provided as part of the official Mbed OS examples [here](https://github.com/ARMmbed/mbed-os-example-google-cloud.git).

## Summary

1. This library depends on Google cloud platform IoT device embedded-C SDK available [here](https://github.com/GoogleCloudPlatform/iot-device-sdk-embedded-c).
1. This SDK port follows the steps listed in SDK docs found [here](https://github.com/GoogleCloudPlatform/iot-device-sdk-embedded-c/blob/master/doc/porting_guide.md).
1. The "Config header" and the "Platform Types" requirements can be found in the library under ['mbed/include/'](./mbed/include)
1. The "Platform layer" port can be found under ['mbed/src/'](./mbed/src) . As a minimum, a time function, random number generator, TLS, heap memory management, network implementation are required. More details on this are provided [here](https://github.com/GoogleCloudPlatform/iot-device-sdk-embedded-c/blob/master/doc/porting_guide.md#bsp-modules).

## Related Links

* [Mbed OS Stats API](https://os.mbed.com/docs/latest/apis/mbed-statistics.html).
* [Mbed OS Configuration](https://os.mbed.com/docs/latest/reference/configuration.html).
* [Mbed OS Serial Communication](https://os.mbed.com/docs/latest/tutorials/serial-communication.html).
* [Mbed OS bare metal](https://os.mbed.com/docs/mbed-os/latest/reference/mbed-os-bare-metal.html).
* [Mbed boards](https://os.mbed.com/platforms/).
* [Google Cloud IoT Core](https://cloud.google.com/iot-core)

## License and contributions

The software is provided under the Apache-2.0 license. Contributions to this project are accepted under the same license.
