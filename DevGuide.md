## Development Guide

* [Overview](#overview)
* [Getting Started](#getstarted)
* [Basic Usage Guidelines](#basicguide)
* [How to create a Network Connection](#createnetworkconnection)
* [How to use the MQTT Client](#usingmqttclient)
* [How to use Shadows](#usingshadows)
* [Shadow Limitations](#shadowlimitations)
* [Json Parsing Support](#jsonparsing)
* [Advanced Usage Guidelines](#advancedguide)
* [How to use Client Core](#usingclientcore)

<a name="overview"></a>
## Overview
This document provides information about how the Device SDK can be used in custom code

<a name="getstarted"></a>
## How to get started ?
As mentioned in the Readme, ensure you understand the AWS IoT platform and create the necessary certificates and policies. For more information on the AWS IoT platform please visit the [AWS IoT developer guide](http://docs.aws.amazon.com/iot/latest/developerguide/iot-security-identity.html).

<a name="basicguide"></a>
## Basic Usage Guidelines
The simplest way to use the SDK is to use it as is with the provided MQTT Client. This can be accomplished by following the below steps:
 * Build the SDK as a library, the provided samples show how this can be done using CMake
 * Include the SDK code as is in the client application. The SDK can be built along with the client code
 
To get basic MQTT support, only the [mqtt/Client.hpp](./include/mqtt/Client.hpp) file needs to be included in the client application. This contains a fully featured MQTT Client. The client expects to be provided with a Network Connection instance. Further details on this are [below](#createnetworkconnection)
Depending on the client application, other files such as [Utf8String.hpp](./include/util/Utf8String.hpp), [JsonParser.hpp](./include/util/JsonParser.hpp) and [ResponseCode.hpp](./include/ResponseCode.hpp) might also need to be included.

For Shadow support, the [shadow/Shadow.hpp](./include/shadow/Shadow.hpp) file needs to be included in the client application.

<a name="createnetworkconnection"></a>
### How to create a Network Connection
The SDK itself does NOT contain any Network Connection classes. It provides a [base class](./include/NetworkConnection.hpp) from which concrete implementations can be derived. The SDK expects that the client will be created and provided with one of these instances.
The only Network functions that the SDK will call directly are below:
 * Connect
 * Disconnect
 * Read
 * Write
 * IsConnected
 * IsPhysicalLayerConnected
 
The SDK will never create or destroy a Network Instance or perform any other operation aside from the ones mentioned above. The Network Connection base class provides a common interface for the above APIs. The SDK itself is designed to be thread safe but since there is no guarantee that the Network Instance provided is also thread safe, this class also provides thread safety for Network Operations.

An additional feature of the SDK is the ability for all actions to do partial Network Read and Writes. The Code for this is located in [src/Action.cpp](./src/Action.cpp).

As mentioned in the [README.md](./README.md) file, The SDK itself provides the below reference implementations for Network Connection:
 * MQTT over TLS using OpenSSL 1.0.2
 * MQTT over TLS using MbedTLS
 * MQTT over WebSocket using OpenSSL 1.0.2 as the underlying TLS layer

Details on how a custom network connection class can be created are available in the Network Connection [README.md](./network/README.md)

<a name="usingmqttclient"></a>
### How to use the MQTT Client
The provided Basic MQTT client supports the full range of MQTT operations. An instance for this client can be created by using either the provided factory method or directly using the constructor. The factory method creates and returns a unique_ptr to an instance of the MQTT Client.
There is no restrictions on how many clients an application can create as long as each client is provided with its own Network Connection instance.

The client creates three threads when it is instantiated:
 * Outbound Processing Queue Runner - Created to process queued outbound MQTT actions such as publish/subscribe etc.
 * Network Read Runner - Created to read incoming data from the Network connection and parse it. Also invokes ack and subscribe callbacks.
 * Keep Alive Runner - Runs the MQTT Keep Alive logic. Responsible for sending out Ping requests and for reconnecting when required.
 
Once the MQTT Client is instantiated, the Network Connection instance cannot be changed. The client will operate with the same Network Instance until it goes out of scope. The Network Connection instance should not be freed after being passed to the Client. The only values in this version which can be changed after the Client is instantiated are related to AutoReconnect. For further information, refer to the API documentation. 
Most API related settings are configured dynamically when the API is called. For example, each Sync API call requires an operation timeout to be provided. Similarly, each Sync API call both takes in a Notification handler as well as provides an Action ID as an out parameter to inform the Client application to differentiate between Acks.

To use the MQTT Client, follow the below steps:
 * Include the mqtt/Client.hpp header file in the application code
 * Create an instance of MQTT Client
 * Call the Connect API to start
 * Perform MQTT operations

To update the MQTT Client's Auto-reconnect configuration:
 * Update the default reconnect backoff timer values using the relevant APIs
 * The auto-reconnect flow can be completely disabled using SetAutoReconnectEnabled API
 * The minimum and maximum backoff timer values can be set using SetMinReconnectBackoffTimeout and SetMaxReconnectBackoffTimeout APIs
 * The default Min value is 1 second and Max value is 128 seconds
 * You can set callbacks for disconnect, reconnect and resubscribe. Please note that these callbacks have to be non-blocking. 

<a name="usingshadows"></a>
### How to use Shadows
The provided Shadow implementation can be used to perform Shadow operations over MQTT. It requires an active MQTT connection instance to be provided when the Shadow instance is created. It is possible to create multiple shadow instances. The shadow instance that is created does not automatically subscribe to any of the shadow action topics by default. It is required to subscribe to the topics manually by using the AddShadowSubscription API.

If an action is performed before the required subscriptions have been made, they will be created before performing the action. All subscriptions created are persistent for the lifetime of the Shadow instance. There's is not option in the shadow itself to unsubscribe from specific shadow action topics. The destructor of the shadow instance will unsubscribe to all currently subscribed topics when the instance goes out of scope.

To use the Shadow APIs, follow the below steps:
 * Create a MQTT Client by using the steps in the above section on [How to use MQTT Client](#usingmqttclient)
 * Create an instance of the Shadow Class using the MQTT client created in the previous step 
 * At this point, the shadow is ready for performing shadow actions:
	* Call the PerformGet API to get the current state of the shadow from the server. This will update the server shadow state document for this instance
 	* To Update the device shadow state, call UpdateDeviceShadow API. This updates the device side shadow document
 		* The JSON document used for this API call MUST have the same structure as the shadow JSON
 		* The suggested way is to either use the GetShadowDocument APIs to get the current shadow document or use the Get Empty Shadow document API to get an empty document. This document should be updated with new values and passed to the API
 		* The update API will merge the provided JSON to the current state of the device shadow
 	* Shadow state on the server can be updated by calling the PerformUpdate API. This generates a diff between the current device and server state shadow documents and calls the shadow update API using this diff
 	* The shadow json can be deleted by calling the PerformDelete API
 	
For further information about the various APIs please read the Shadow API documentation.

<a name="shadowlimitations"></a>
### Shadow Limitations
 * MQTT Client:
	Currently, there is only one MQTT Client implementation provided. It is not possible to use Client Core directly with the current version of Shadow.
 * Number of Shadows per MQTT Connection:
	Each shadow instance can create up to a maximum of 7 subscriptions. This number will only go higher as more features are added.Since the current maximum number of allowed subscriptions per MQTT connection is 50, currently, no more than 7 shadows should be added per connection. However, i
 * Automatic Sync:
 	In the current version of the SDK, there is no Action that automatically syncs device shadow state with the server. A manual update operation is required to perform a sync with the server.
 * Json Parsing limitations:
 	Please read the Json Parsing section below.
 
<a name="jsonparsing"></a>
### Json Parsing Support
The SDK uses [RapidJson](https://github.com/miloyip/rapidjson) to provide Json parsing support. It provides a very thin [layer](./include/util/JsonParser.hpp) on top of RapidJson to provide scope for future enhancements but the entire library is available for use and can be included in client applications as necessary. A tutorial for using RapidJson is available [here](http://rapidjson.org/md_doc_tutorial.html).

Please note that while RapidJson supports nested Json parsing and the SDK itself also supports it, operations such as merge and diff are very expensive. The server supports a nested Json depth of up to 5. But the SDK uses recursion to perform nested Json operations which might cause issues on devices with limited resources. The recommended approach is to keep the Json depth as low as possible. Code for the merge and diff operations can be seen in [here](./src/util/JsonParser.cpp). 

<a name="logging"></a>
### Logging
The ConsoleLogSystem class is used to provide logging capabilities. To enable logging in your application, using the PubSub sample as an example:
* [Create](./samples/PubSub/PubSub.cpp#L271) an instance of the ConsoleLogSystem 
* [Initialize](./samples/PubSub/PubSub.cpp#L273) the instance
* Define a [log tag](./samples/PubSub/PubSub.cpp#L40) for use in the application 
* Use [AWS_LOG_INFO](./src/mqtt/Connect.cpp#L74) for logging information
* Use [AWS_LOG_ERROR](./samples/PubSub/PubSub.cpp#142) for logging errors


<a name="advancedguide"></a>
## Advanced Usage Guidelines
The SDK can also be used to create custom clients for different use cases. The SDK is structured around a Core Client and Actions that are executed by this client. The provided MQTT Client referenced above, is simply one implementation that uses all the supported Actions. Custom Clients can be created using only some of the Actions which perform only a subset of MQTT functionality.
The following actions are provided in the current implementation of the SDK:
 * [ConnectActionAsync](./include/mqtt/Connect.hpp#L195) - Provides support for MQTT Connect operation. Also calls Network layer Connect. This action is also used for Reconnect operations.
 * [DisconnectActionAsync](./include/mqtt/Connect.hpp#L252) - Provides support for MQTT Disconnect operation. Also calls Network layer Disconnect.
 * [KeepaliveActionRunner](./include/mqtt/Connect.hpp#L308) - Provides support for the MQTT Keepalive workflow. Responsible for sending out ping requests and performing Reconnect if required. This action can be run in a thread or as a one time operation with the thread sync boolean variable set to false.
 * [NetworkReadActionRunner](./include/mqtt/NetworkRead.hpp) - Provides Network read functionality for the SDK. All Read operations are performed in this Action including waiting on Acks for sync operations. Similar to Keepalive, this action can also be run in a thread or as a one time operation with the thread sync boolean variable set to false.
 * [PublishActionAsync](./include/mqtt/Publish.hpp#L197) - Provides support for MQTT Publish operations.
 * [PubackActionAsync](./include/mqtt/Publish.hpp#L252) - Provides support for the MQTT Puback operation. Called by the SDK to reply with PUBACK to any received QOS1 messages.
 * [SubscribeActionAsync](./include/mqtt/Publish.hpp#L231) - Provides support for MQTT Subscribe operations. Maximum number of supported subscribe topics per message is 8. Only one handler can be provided for each subscription.
 * [UnsubscribeActionAsync](./include/mqtt/Publish.hpp#L286) - Provides support for MQTT Unsubscribe operations. Maximum number of supported unsubscribe topics per message is 8.
 
The Action instances are created at run time by Client Core whenever the Action is registered. The ClientCore class does not do this automatically. Different combinations of registered actions allow for different clients depending on use case.
For example, a custom Publish only Client can be created by only registering the Connect, Disconnect, KeepAlive, Network Read, Publish and Puback Actions.

Please note that the provided Shadow implementation uses the full MQTT Client instead of using [ClientCore](./include/ClientCore.hpp) directly. If Shadow features are required, there is no way to avoid using the reference client in this version of the SDK.

<a name="usingclientcore"></a>
### How to use Client Core
[ClientCore](./include/ClientCore.hpp) forms the basis of all Clients created using the SDK. It is responsible for executing Actions either one at a time or by creating threads for each action. To create a custom Client follow the steps listed below:
 * Create an instance of [ClientCoreState](./include/ClientCoreState.hpp)
 * Create the ClientCore instance, the constructor expects a shared pointer to the above State instance as argument
   * The constructor for the ClientCore instance will create a thread for processing Outbound Actions. This thread takes any Async actions that have been queued up and processes them one by one
   * All actions use a default value of sleep time if they are required to call sleep as a part of their execution. This is defined as a constant [here](./include/Action.hpp#L38). It may be necessary to tweak this value depending on the use case 
   * The Action processing rate is currently a constant defined [here](./src/ClientCoreState.cpp#L27)
   * The Maximum size of the queue can be modified in the ClientState instance using the SetMaxActionQueueSize API defined [here](./include/ClientCoreState.hpp#L135). The Default value is a constant defined [here](./include/ClientCoreState.hpp#L45)
 * Register actions to the ClientCore instance using the RegisterAction API defined [here](./include/ClientCore.hpp#L100)
   * This creates an instance of the Action that will be used for all subsequent calls to PerformAction with this ActionType. Only ONE instance of this Action will be created if it is not required to run in a separate thread
   * Custom Actions can be created by creating a derived class of the [Action](./include/Action.hpp#L142) class
   * Each Custom Action is required to define a Create Factory method that can be used by ClientCore to create an instance of the action
   * Each Custom Action requires an Action Type to be added to the [ActionType](./include/Action.hpp#L47) class
   * ClientCore will pass the ClientCoreState Instance provided to it when it was instantiated. Additional Action specific values can be added to the state by creating a derived class of ClientCoreState
 * After registration, any Actions that need to be run as their own threads can be started by calling the [CreateActionRunner](./include/ClientCore.hpp#L144) API
   * This will create a [ThreadTask](./include/util/threading/ThreadTask.hpp) instance that is used to run the Action
   * It then proceeds to create a new instance of the Action Type which was requested and assign it to the above ThreadTask
   * All threads created in the above manner are cleared out when either the PerformAction function returns or the ClientCore instance goes out of scope
 * To Perform a registered Action, the PerformAction and PerformActionAsync APIs can be used depending on desired behavior
   * Only one Sync/Blocking action can be performed at a time in the current version of the SDK
 * When the ClientCore instance goes out of scope, the destructor automatically stops all running threads and frees any memory associated with those threads
