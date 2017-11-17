## Tests

The SDK provides several unit and integration tests to verify SDK features and functionality. These tests can be run on any of the supported Operating Systems.

Please note that while the test descriptions apply across all supported platforms, the below execution instructions are for Linux. On Windows, the test targets show up as individual projects in Visual Studio after cmake generation and can be run by simply running the project. Please note that the network library needs to be selected while generating the solution at this time.

## Integration Tests

Prerequisites:
* Create an AWS IoT thing on the IoT console and download the certificates. Make sure the certificates are activated
* Note down the endpoint that you will use to interact with the thing (will be of the form <xxx>.iot.<region>.amazonaws.com).
* Copy the certs into the <SDK Root>/certs folder along with the Symantec root CA.

The SDK comes with the below Integration tests for various SDK features. All tests can be run for the provided reference Network wrappers. To build the tests for each provided wrapper, use the below cmake calls from the build directory:
 * OpenSSL (Default if no argument is provided)- `cmake <path_to_sdk>` OR `cmake <path_to_sdk> -DNETWORK_LIBRARY=OpenSSL`
 * MbedTLS - `cmake <path_to_sdk> -DNETWORK_LIBRARY=MbedTLS`
 * WebSocket - `cmake <path_to_sdk> -DNETWORK_LIBRARY=WebSocket`
 
Followed by:

`make aws-iot-integration-tests`
 
To run the tests, 
* switch to the generated `bin` folder
* modify the `config/IntegrationTestConfig.json` with the certificate names and the endpoint for your thing
* run  `./aws-iot-integration-tests`

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


### Multiple Subscription Auto Reconnect Test

This test verifies that the SDK can be used in applications with varying number of subscriptions and that the auto-reconnect will not fail irrespective of number of subscriptions. It creates a client that connects and subscribes to multiple topics, ranging from 0 to 8. It publishes a few messages to verify connectivity. Then it proceeds to simulate a disconnect and waits for reconnect to occur. The connection is verified by messages on the subscribe lifecycle event topic. Once the connection is successfully restored, the client publishes messages again on the test topic to verify resubscribe worked as expected.

## Using LLVM Sanitizers with unit/integration tests
* Install a recent Clang compiler suite. Some sanitizers work with recent versions of GCC, but generally Clang has better support. For Ubuntu, run `sudo apt-get install clang`. Most Linux systems have support for all sanitizers but OSX only suports address sanitizers. 
* From the main directory, run these commands to pick the Clang compiler and turn on a sanitizer. _Picking the compiler must be done before the very first run of Cmake in a fresh build dir_
 
```$xslt
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=debug -DSANITIZE_THREAD=On -DCMAKE_CXX_COMPILER=/usr/bin/clang++
make
```
* Supported sanitizers
  * -DSANITIZE_THREAD=On - Turns on [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) which detects data races
  * -DSANITIZE_ADDRESS=On - Turns on [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) which detects memory errors
  * -DSANITIZE_UNDEFINED=On - Turns on [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) 
  * -DSANITIZE_MEMORY=On - Turns on [MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html) which detects uninitialized reads.  
  _Note that msan generally gives many false reports unless all supporting code, including stdlib, is compiled with msan on. This often makes it impractical to use_
* Only turn on one sanitizer at a time
* Some helpful environmental variables that change sanitizer behavior:
  
```$xslt
export TSAN_OPTIONS=${TSAN_OPTIONS:-"second_deadlock_stack=1,halt_on_error=0"}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-"halt_on_error=0"}
export ASAN_OPTIONS=${ASAN_OPTIONS:-"halt_on_error=0"}
```
