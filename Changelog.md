# Change Log

## [1.4.0](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.4.0) (May 10, 2018)

Features:
  - [#95](https://github.com/aws/aws-iot-device-sdk-cpp/pull/95) - Jobs support with custom auth
  - ALPN support

Bugfixes/Improvements:
  - Fixed Issues:
    - [#60](https://github.com/aws/aws-iot-device-sdk-cpp/pull/60) - Ignore sigpipe to prevent program termination
    - [#93](https://github.com/aws/aws-iot-device-sdk-cpp/pull/93) - Set auto-reconnect to false when resubscribe fails
    - [#86](https://github.com/aws/aws-iot-device-sdk-cpp/pull/86) - Make shadow handler call response handler for error cases
    - [#87](https://github.com/aws/aws-iot-device-sdk-cpp/pull/87) - Failing WSS handshake for session token
    - Updated shadow documentation on shadow client token limitation
    - Fix wildcard regex for special topics with $ symbols
  - Included pull requests:
    - [#63](https://github.com/aws/aws-iot-device-sdk-cpp/pull/63) - Update LogMacros.hpp
    - [#67](https://github.com/aws/aws-iot-device-sdk-cpp/pull/67) - Fixed invalid read inside mqtt/Client
    - [#70](https://github.com/aws/aws-iot-device-sdk-cpp/pull/70) - Add some minor improvements
    - [#73](https://github.com/aws/aws-iot-device-sdk-cpp/pull/73) - Allow Position Independent Code for static library
    - [#75](https://github.com/aws/aws-iot-device-sdk-cpp/pull/75), [#76](https://github.com/aws/aws-iot-device-sdk-cpp/pull/76) and [#77](https://github.com/aws/aws-iot-device-sdk-cpp/pull/77) - Pull requests to fix warnings on windows
    - [#83](https://github.com/aws/aws-iot-device-sdk-cpp/pull/83) - Adding standard files
    - [#88](https://github.com/aws/aws-iot-device-sdk-cpp/pull/88) - URL encode session token when building the canonical error string
    - [#92](https://github.com/aws/aws-iot-device-sdk-cpp/pull/92) - Add the currently missing error checks when creating an MbedTLSconnection with ALPN support enabled


# [1.3.0](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.3.0) (Nov 22, 2017)

Features:
  - [#21](https://github.com/aws/aws-iot-device-sdk-cpp/issues/21) - add reconnect and resubscribe callback

Bugfixes/Improvements:
  - Fixed Issues:
    - [#5](https://github.com/aws/aws-iot-device-sdk-cpp/issues/5) - openssl wrapper gets stuck if client is started before network is ready
    - [#44](https://github.com/aws/aws-iot-device-sdk-cpp/issues/44) - SDK fails to build with OpenSSL 1.1.0
    - [#46](https://github.com/aws/aws-iot-device-sdk-cpp/issues/46) - MAX_RW_BUF_LEN not large enough for WebSocketConnection::WssHandshake
    - [#47](https://github.com/aws/aws-iot-device-sdk-cpp/issues/47) - Fix memory leak in OpenSSL when connect fails
    - [#52](https://github.com/aws/aws-iot-device-sdk-cpp/issues/52) - Fix for shadow crash when server document does not have all keys required
    - [#54](https://github.com/aws/aws-iot-device-sdk-cpp/issues/54) - Fix for comparison always being false due to limited range of data type
    - Removed redundant comments
    - Set correct packet ID for resubscribe packets in keepalive
    - updated default keepalive to 600 seconds
    - updated SDK metrics string
  - Included pull requests:
    - [#6](https://github.com/aws/aws-iot-device-sdk-cpp/pull/6) - Add res_init() call to update resolv.conf in glibc
    - [#19](https://github.com/aws/aws-iot-device-sdk-cpp/pull/19) - use version independent TLS method as TLSv1_2_method is deprecated
    - [#27](https://github.com/aws/aws-iot-device-sdk-cpp/pull/27) - update include
    - [#39](https://github.com/aws/aws-iot-device-sdk-cpp/pull/39) - Implement static Shadow method GetEmptyShadowDocument
    - [#40](https://github.com/aws/aws-iot-device-sdk-cpp/pull/40) - Do not add std::chrono::milliseconds to std::chrono::time_point
    - [#43](https://github.com/aws/aws-iot-device-sdk-cpp/pull/43) - Fill in context data for disconnect callback
    - [#45](https://github.com/aws/aws-iot-device-sdk-cpp/pull/45) - Allow SDK to work with OpenSSL 1.1.0
    - [#53](https://github.com/aws/aws-iot-device-sdk-cpp/pull/53) - Remove extra semicolons as they appear as warnings in clang -wpedantic


## [1.2.0](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.2.0) (September 26th, 2017)

Features:
  - Added a cross-compile toolchain template and instructions on how to cross compile the SDK
  
Bugfixes/Improvements:
  - Fixed Issues:
    - [#16](https://github.com/aws/aws-iot-device-sdk-cpp/issues/16) - alloc-dealloc-mismatch
    - Fixed OpenSSL memory leak and ensured the socket is closed correctly 
    - Fixed disconnect log loop in keepalive 
  - Included pull requests:
    - [#15](https://github.com/aws/aws-iot-device-sdk-cpp/pull/15) - Add sanitizers
    - [#37](https://github.com/aws/aws-iot-device-sdk-cpp/pull/37) - Fix compilation error with unused variables
  - Improvements:
    - Added line numbers and function names to all logs
    - Added unit tests for ConfigCommon and ResponseCode
    - Added SDK version string into username field

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
