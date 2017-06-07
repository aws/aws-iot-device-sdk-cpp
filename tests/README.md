## Tests

The SDK provides several unit and integration tests to verify SDK features and functionality. These tests can be run on any of the supported Operating Systems.

Please note that while the test descriptions apply across all supported platforms, the below execution instructions are for Linux. On Windows, the test targets show up as individual projects in Visual Studio after cmake generation and can be run by simply running the project. Please note that the network library needs to be selected while generating the solution at this time.

## Integration Tests

The SDK comes with the below Integration tests for various SDK features. All tests can be run for the provided reference Network wrappers. To build the tests for each provided wrapper, use the below cmake calls from the build directory:
 * OpenSSL (Default if no argument is provided)- `cmake <path_to_sdk>` OR `cmake <path_to_sdk> -DNETWORK_LIBRARY=OpenSSL`
 * MbedTLS - `cmake <path_to_sdk> -DNETWORK_LIBRARY=MbedTLS`
 * WebSocket - `cmake <path_to_sdk> -DNETWORK_LIBRARY=WebSocket`
 
Followed by:

`make aws-iot-integration-tests`
 
To run the tests, switch to the generated `bin` folder and use the below command:
 
`./aws-iot-integration-tests`

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

