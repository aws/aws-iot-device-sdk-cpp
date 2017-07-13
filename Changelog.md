# Change Log

## [1.1.1](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.1.1) (July 13th, 2017)

Features:
  - N/A

Bugfixes/Improvements:
  - Fixed issues:
    - [#10](https://github.com/aws/aws-iot-device-sdk-cpp/issues/10) - Fix just-in-time-registration of device certs when using OpenSSL
    - [#12](https://github.com/aws/aws-iot-device-sdk-cpp/issues/12) - Stop receiving duplicate messages when using QoS 1 
    - [#17](https://github.com/aws/aws-iot-device-sdk-cpp/issues/17) - Disconnect callback should be called even when auto-reconnect is disabled
    - [#18](https://github.com/aws/aws-iot-device-sdk-cpp/issues/18) - Clear subscriptions on disconnect 
    - [#20](https://github.com/aws/aws-iot-device-sdk-cpp/issues/20) - Resubscribe to previously subscribed topic should not cause crash 
    - [#23](https://github.com/aws/aws-iot-device-sdk-cpp/issues/23) - Fix memory leaks
  - Included pull requests:
    - [#24](https://github.com/aws/aws-iot-device-sdk-cpp/pull/24) - Fix cyclic refences
    - [#26](https://github.com/aws/aws-iot-device-sdk-cpp/pull/26) - Fix subscription API documentaion
  
## [1.1.0](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.1.0) (May 8th, 2017)
 
Features:
  - Support for Greengrass discovery
    - GreengrassClient that inherits from Client
    - Greengrass Response parser
    - Updates to support shadow operations with Greengrass Cores
    - additional samples for discovery
  - Updated unit tests and integration tests
  - Added support for wildcard subscriptions
 
 
Bugfixes/Improvements:
  - Fixed issues [#4](https://github.com/aws/aws-iot-device-sdk-cpp/issues/4) and [#7](https://github.com/aws/aws-iot-device-sdk-cpp/issues/7)
  - Updated makefiles with [#3](https://github.com/aws/aws-iot-device-sdk-cpp/pull/3) and [#8](https://github.com/aws/aws-iot-device-sdk-cpp/pull/8)
  - Added install target
  - Fixed bug where single character subscriptions causes client to crash
  - Split up ports being used for WebSockets, MQTT and Greengrass discovery into separate fields in the config file
  - Cleanup of formatting
  - Helper class to convert response codes to strings to enable easier debugging
  - OpenSSL wrapper updates
  - Updated Platform.md
 
## [1.0.0](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.0.0) (October 27, 2016)
 
Features:
  - Initial release
  - MQTT Publish and Subscribe
  - Basic Async Shadow support
  - TLS mutual auth on linux with OpenSSL
  - TLS mutual auth on linux with MbedTLS
  - MQTT over Websockets with OpenSSL as TLS layer
  - Unit Tests
  - Integration Tests
  - Tested platforms - Linux, Windows (VS 2015) and Mac OS

Bugfixes/Improvements:
  - N/A
