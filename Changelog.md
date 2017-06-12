# Change Log
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
