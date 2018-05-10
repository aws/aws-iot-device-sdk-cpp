# Known Issues/Limitations
## [1.4.0](https://github.com/aws/aws-iot-device-sdk-cpp/releases/tag/v1.4.0) (May 10, 2018)

### Common

  - Client Core - Reduce shared pointer usage, figure out ways to use fewer copy operations
  - MQTT - if subcriptions are disallowed by the policy, enabling auto resubcribe can lead to a connect-disconnect loop, issue [#94](https://github.com/aws/aws-iot-device-sdk-cpp/issues/94)
  - Network Read - Add recovery options for out of sync buffers
  - Network Layer, Common - Currently, vector resize is called before the vector is passed for reading in data. Determine if a more flexible solution is possible with fewer allocations
  - Network Layer, OpenSSL - error handling for library API call
  - Network Layer, OpenSSL - setting for disconnect timeouts
  - Network Layer, WebSocket - Refactor WSLay and merge it with the WebSocket wrapper, improve testing
  - Network Layer, WebSocket - TLS Read and Write are highly inefficient in current implementation, improvements required
  - Network Layer, Common - Add support for IPv6, remove deprecated system API calls
  - Shadow - Refactor delta requests into separate function
  - Shadow - Add action for automatically performing Shadow updates
  - Samples, Shadow - Add Console Echo Sample
  - Threading - lock-order-inversion (potential deadlock), issue [#14](https://github.com/aws/aws-iot-device-sdk-cpp/issues/14)
  - CLI, Configuration - Add support for command line cert paths
  - CMake, path warnings - Cleaner way to download repositories from github and build them
  - CMake, build settings - Provide options for setting up certain values that are hardcoded using macros at the moment
  - Testing - More edge case testing required for Reconnect logic
  - Testing - Improvements needed on Integration testing for network reference layers specifically for different compilers and platforms
  - UTF-8 string - improve validation checks
  
### Windows
  
  - Build issues with VS 2013
  - Project structure in Solution Explorer needs to be updated
  - CMake warnings on solution generation
  - Working directory needs to be updated every time CMake solution is rebuilt to ${OutDir} for configuration to be picked up as expected
