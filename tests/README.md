## Tests

The SDK provides several unit and integration tests to verify SDK features and functionality. These tests can be run on any of the supported Operating Systems.

Please note that while the test descriptions apply across all supported platforms, the below execution instructions are for Linux. On Windows, the test targets show up as individual projects in Visual Studio after cmake generation and can be run by simply running the project. Please note that the network library needs to be selected while generating the solution at this time.

## Integration Tests

prereqs:
* set up an aws iot endpoint by turning on aws iot on some aws account.  it will look like "xxx.iot.us-west-2.amazonaws.com"
* edit `build/bin/config/IntegrationTestConfig.json` after running cmake (add endpoint like `xxx.iot.us-west-2.amazonaws.com` and note cert names)
* drop certs into `build/bin/certs/` (or they can be pre-placed into `/certs/` in the main directory and they'll be copied over when cmake is first run).  name certs according to their `IntegrationTestConfig.json` names

The SDK comes with the below Integration tests for various SDK features. All tests can be run for the provided reference Network wrappers. To build the tests for each provided wrapper, use the below cmake calls from the build directory:
 * OpenSSL (Default if no argument is provided)- `cmake <path_to_sdk>` OR `cmake <path_to_sdk> -DNETWORK_LIBRARY=OpenSSL`
 * MbedTLS - `cmake <path_to_sdk> -DNETWORK_LIBRARY=MbedTLS`
 * WebSocket - `cmake <path_to_sdk> -DNETWORK_LIBRARY=WebSocket`
 
Followed by:

`make aws-iot-integration-tests`
 
To run the tests, switch to the generated `bin` folder and use the below command:

`./aws-iot-integration-tests`

## Using LLVM Sanitizers with unit/integration tests
* sanitizers are enabled using this github project: https://github.com/arsenm/sanitizers-cmake
* it's installed as a git submodule.  run `git submodule update --init` to download it
* install a recent clang compiler suite.  Some sanitizers work with recent versions of GCC, but generally clang has better support
* on ubuntu, `apt-get install clang`
* from the main directory, run these commands to pick the clang compiler and turn on a sanitizer. _picking the compiler must be done before the very first run of cmake in a fresh build dir_

```
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=debug -DSANITIZE_THREAD=On
make
```
* supported sanitizers: (see https://github.com/arsenm/sanitizers-cmake)
 * -DSANITIZE_THREAD=On
 * -DSANITIZE_ADDRESS=On
 * -DSANITIZE_UNDEFINED=On
 * -DSANITIZE_MEMORY=On _note that msan generally gives many false reports unless all supporting code, including stdlib, is compiled with msan on.  this often makes it impractical to use_
* only turn on one sanitizer at a time
* some helpful environment vars that change sanitizer behavior:
```
export TSAN_OPTIONS=${TSAN_OPTIONS:-"second_deadlock_stack=1,halt_on_error=0"}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-"halt_on_error=0"}
export ASAN_OPTIONS=${ASAN_OPTIONS:-"halt_on_error=0"}
```

### Basic MQTT Publish Subscribe

This tests basic MQTT features. It creates one MQTT Client with all MQTT features and then proceeds to subscribe to a test topic. Once it has subscribe successfully it publishes messages on that topic and verifies that all messages were received successfully.
There are two distinct publish phases:
 * Publish close to the max size payload supported by the AWS IoT service (128KB) and verify successful publish.
 * Publish enough messages to completely fill up the MQTT Client's action queue and wait for space to become available before filling it up again. Repeats this process upto the maximum number of messages and then verifies all messages were received.
 
NOTE - This test can fail if auto-reconnect happens while the test is in progress. Its a reliable indicator that functionality is working as expected. It is not a reliable indicator of the stability of the connection.


### Auto-reconnect test

This test verifies the Auto-reconnect functionality. It creates an MQTT Client similar to the above basic test and publishes a few messages to verify connectivity. Then it proceeds to simulate a disconnect and waits for reconnect to occur. The connection is verified by messages on the subscribe lifecycle event topic. Once the connection is successfully restored, the client publishes messages again on the test topic to verify resubscribe worked as expected.


### Multiple Clients test

This test verifies that the SDK can be used in applications that create several MQTT Clients at the same time. It creates three MQTT Clients, once that only subscribes to a test topic and two others that only publish to the test topic. Then it proceeds to publish messages using both the Publishers and verifies all messages were successfully received.
 
NOTE - This test can fail if auto-reconnect happens while the test is in progress. Its a reliable indicator that functionality is working as expected. It is not a reliable indicator of the stability of the connection.


