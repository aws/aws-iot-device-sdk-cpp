## ***** NOTICE *****

This repository is moving to a new branching system. The master branch will now contain bug fixes/features that have been minimally tested to ensure nothing major is broken. The release branch will contain new releases for the SDK that have been tested thoroughly on all supported platforms. Please ensure that you are tracking the release branch for all production work.

This change will allow us to push out bug fixes quickly and avoid having situations where issues stay open for a very long time.

## AWS IoT C++ Device SDK

* [Overview](#overview)
* [Features](#features)
* [Design Goals](#design)
* [Collection of Metrics](#metrics)
* [Getting Started](#getstarted)
* [Installation](#installation)
* [Porting to different platforms](#porting)
* [Quick Links](#quicklinks)
* [Sample APIs](#sampleapis)
* [License](#license)
* [Support](#support)

<a name="overview"></a>
## Overview
This document provides information about the AWS IoT device SDK for C++.

<a name="features"></a>
## Features
The Device SDK simplifies access to the Pub/Sub functionality of the AWS IoT broker via MQTT and provides APIs to interact with Thing Shadows. The SDK has been tested to work with the AWS IoT platform to ensure best interoperability of a device with the AWS IoT platform.

### MQTT Connection
The Device SDK provides functionality to create and maintain a MQTT Connection. It expects to be provided with a Network Connection class that connects and authenticates to AWS IoT using either direct TLS or WebSocket over TLS. This connection is used for any further publish operations. It also allows for subscribing to MQTT topics which will call a configurable callback function when these messages are received on these topics.

### Thing Shadow
This SDK implements the specific protocol for Thing Shadows to retrieve, update and delete Thing Shadows adhering to the protocol that is implemented to ensure correct versioning and support for client tokens. It abstracts the necessary MQTT topic subscriptions by automatically subscribing to and unsubscribing from the reserved topics as needed for each API call. Inbound state change requests are automatically signalled via a configurable callback.

### Jobs
This SDK also implements the Jobs protocol to interact with the AWS IoT Jobs service. The IoT Job service manages deployment of IoT fleet wide tasks such as device software/firmware deployments and updates, rotation of security certificates, device reboots, and custom device specific management tasks. For additional information please see the [Jobs developer guide](https://docs.aws.amazon.com/iot/latest/developerguide/iot-jobs.html).

<a name="design"></a>
## Design Goals of this SDK
The C++ SDK was specifically designed for devices that are not resource constrained and required advanced features such as Message queueing, multi-threading support and the latest language features

Primary aspects are:
 * Designed around the C++11 standard
 * Platform neutral, as long as the included CMake can find a C++11 compatible compiler and threading library
 * Network layer abstracted from the SDK. Can use any TLS library and initialization method
 * Support for multiple platforms and compilers. Tested on Linux, Windows (with VS2015) and Mac OS
 * Flexibility in picking and choosing functionality, can create Clients which only perform a subset of MQTT operations
 * Support for Rapidjson allowing use of complex shadow document structures
 
 <a name="metrics"></a>
 ## Collection of Metrics
 Beginning with Release v1.2.0 of the SDK, AWS collects usage metrics indicating which language and version of the SDK is being used. This allows us to prioritize our resources towards addressing issues faster in SDKs that see the most and is an important data point. However, we do understand that not all customers would want to report this data by default. In that case, the sending of usage metrics can be easily disabled by the user by using the overloaded Connect action which takes in a boolean for enabling or disabling the SDK metrics:
```
p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_, ConfigCommon::is_clean_session_,
                                        mqtt::Version::MQTT_3_1_1, ConfigCommon::keep_alive_timeout_secs_,
                                        std::move(client_id), nullptr, nullptr, nullptr, false); // false for disabling metrics
```

<a name="getstarted"></a>
## How to get started ?
Ensure you understand the AWS IoT platform and create the necessary certificates and policies. For more information on the AWS IoT platform please visit the [AWS IoT developer guide](http://docs.aws.amazon.com/iot/latest/developerguide/iot-security-identity.html).

<a name="installation"></a>
## Installation
This section explains the individual steps to retrieve the necessary files and be able to build your first application using the AWS IoT C++ SDK.
The SDK uses CMake to generate the necessary Makefile. CMake version 3.2 and above is required.

Prerequisites:
 
 * Make sure to have latest CMake installed. Minimum required version is 3.2
 * Compiler should support C++11 features. We have tested this SDK with gcc 5+, clang 3.8 and on Visual Studio 2015.
 * Openssl has version 1.0.2 and libssl-dev has version 1.0.2. OpenSSL v1.1.0 reference wrapper implementation is not included in this version of the SDK.
 * You can find basic information on how to set up the above on some popular platforms in [Platform.md](https://github.com/aws/aws-iot-device-sdk-cpp/blob/master/Platform.md)

Build Targets:

 * The SDK itself builds as a library by default. All the samples/tests link to the library. The library target is `aws-iot-sdk-cpp`
 * Unit tests - `aws-iot-unit-tests`
 * Integration tests - `aws-iot-integration-tests`
 * Sample - `pub-sub-sample`
 * Sample - `shadow-delta-sample`

 This following sample targets are generated only if OpenSSL is being used:
 * Sample - `discovery-sample`. 
 * Sample - `robot-arm-sample`. 
 * Sample - `switch-sample`
 
Steps:

 * Clone the SDK from the github repository
 * Change to the repository folder. Create a folder called `build` to hold the build files and change to this folder. In-source builds are NOT allowed
 * Run `cmake ../.` to build the SDK with the CLI.
 * The command will download required third party libraries automatically and generate a Makefile
 * Type `make <target name>` to build the desired target. It will create a folder called `bin` that will have the build output
   
<a name="porting"></a>
## Porting to different platforms
The SDK has been written to adhere to C++11 standard without any additional compiler specific features enabled. It should compile on any platform that has a modern C++11 enabled compiler without issue.
The platform should be able to provide a C++11 compatible threading implementation (eg. pthread on linux).
TLS libraries can be added by simply implementing a derived class of NetworkConnection and providing an instance to the Client.
We provide the following reference implementations for the Network layer:

 * OpenSSL - MQTT over TLS using OpenSSL v1.0.2. Tested on Windows (VS 2015) and Linux
 	* The provided implementation requires OpenSSL to be pre-installed on the device
 	* Use the mqtt port setting from the config file while setting up the network instance
 * MbedTLS - MQTT over TLS using MbedTLS. Tested on Linux
 	* The provided implementation will download MbedTLS v2.3.0 from the github repo and build and link to the libraries. Please be warned that the default configuration of MbedTLS limits packet sizes to 16K
 	* Use the mqtt port setting from the config file while setting up the network instance
 * WebSocket - MQTT over WebSocket. Tested on both Windows (VS 2015) and Linux. Uses OpenSSL 1.0.2 as the underlying TLS layer
 	* The provided implementation requires OpenSSL to be pre-installed on the device
 	* Please be aware that while the provided reference implementation allows initialization of credentials from any source, the recommended way to do so is to use the aws cli to generate credential files and read the generated files
 	* Use the https port setting from the config file while setting up the network instance

### Cross-compiling the SDK for other platforms
The included ToolchainFile.cmake file can be used to cross-compile the SDK for other platforms.
Procedure for testing cross compiling (if using OpenSSL):

1. build/download toolchain for specific platform
2. modify the ToolchainFile.cmake with location and target of toolchain.
 ```
 # specify toolchain directory
 SET(TOOLCHAIN_DIR /home/toolchain/dir/here/bin)
   
 # specify cross compilation target
 SET(TARGET_CROSS target-here)`
 ```
3. Cross-compile OpenSSL using the same toolchain 
4. modify `network/CMakeLists.txt.in` and change OpenSSL library location to cross-compiled OpenSSL

5. 
```
cd build
cmake ../. -DCMAKE_TOOLCHAIN_FILE=../ToolchainFile.cmake
make
```
6. Scp the application binary, certs and config for the application into the platform you're testing 
7. Run `./<application>`

For MbedTLS, you don't need to cross-compile MbedTLS as it gets compiled when you run `make` with the same compiler as pointed to by the toolchain file.

Also included is a simple example 'toolchain' which is used for setting the default compiler as clang++ instead of g++ as an example to show how the toolchain file can be modified.


<a name="quicklinks"></a>
## Quick Links

 * [SDK Documentation](http://aws-iot-device-sdk-cpp-docs.s3-website-us-east-1.amazonaws.com/v1.4.0/index.html) - API documentation for the SDK
 * [Platform Guide](./Platform.md) - This file lists the steps needed to set up the pre-requisites on some popular platforms
 * [Developers Guide](./DevGuide.md) - Provides a guide on how the SDK can be included in custom code
 * [Greengrass Discovery Support Guide](./GreengrassDiscovery.md) - Provides information on support for AWS Greengrass Discovery Service
 * [Network Layer Implementation Guide](./network/README.md) - Detailed description about the Network Layer and how to implement a custom wrapper class
 * [Sample Guide](./samples/README.md) - Details about the included samples
 * [Test Information](./tests/README.md) - Details about the included unit and integration tests
 * [MQTT 3.1.1 Spec](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/csprd02/mqtt-v3.1.1-csprd02.html) - Link to the MQTT v3.1.1 spec that this SDK implements

<a name="sampleapis"></a>
## Sample APIs
### Sync

Creating a basic MQTT Client requires a NetworkConnection instance and MQTT Command timeout in milliseconds for any internal blocking operations.

```
std::shared_ptr<NetworkConnection> p_network_connection = <Create Instance>;
std::shared_ptr<MqttClient> p_client = MqttClient::Create(p_network_connection, std::chrono::milliseconds(30000));
```

Connecting to the AWS IoT MQTT platform

```
rc = p_client->Connect(std::chrono::milliseconds(30000), false, mqtt::Version::MQTT_3_1_1, std::chrono::seconds(60), Utf8String::Create("<client_id>"), nullptr, nullptr, nullptr);
```

Subscribe to a topic

```
util::String p_topic_name_str = <topic>;
std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler = std::bind(&<handler>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(std::move(p_topic_name), mqtt::QoS::QOS0, p_sub_handler, nullptr);
util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
topic_vector.push_back(p_subscription);
rc = p_client->Subscribe(topic_vector, std::chrono::milliseconds(30000));
```

Publish to a topic
```
util::String p_topic_name_str = <topic>;
std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
rc = p_client->Publish(std::move(p_topic_name), false, false, mqtt::QoS::QOS1, payload, std::chrono::milliseconds(30000));
```

Unsubscribe from a topic

```
util::String p_topic_name_str = <topic>;
std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
util::Vector<std::unique_ptr<Utf8String>> topic_vector;
topic_vector.push_back(std::move(p_topic_name));
rc = p_client->Subscribe(topic_vector, std::chrono::milliseconds(30000));
```

### Async
Connect is a sync only API in this version of the SDK.
Subscribe to a topic

```
uint16_t packet_id_out;
util::String p_topic_name_str = <topic>;
std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler = std::bind(&<handler>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
std::shared_ptr<mqtt::Subscription> p_subscription = mqtt::Subscription::Create(std::move(p_topic_name), mqtt::QoS::QOS0, p_sub_handler, nullptr);
util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
topic_vector.push_back(p_subscription);
rc = p_client->SubscribeAsync(topic_vector, nullptr, packet_id_out);
```

Publish to a topic
```
uint16_t packet_id_out;
util::String p_topic_name_str = <topic>;
std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
rc = p_client->PublishAsync(std::move(p_topic_name), false, false, mqtt::QoS::QOS1, payload, packet_id_out);
```

Unsubscribe from a topic

```
uint16_t packet_id_out;
util::String p_topic_name_str = <topic>;
std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(p_topic_name_str);
util::Vector<std::unique_ptr<Utf8String>> topic_vector;
topic_vector.push_back(std::move(p_topic_name));
rc = p_client->Subscribe(topic_vector, packet_id_out);
```

### Logging
To enable logging, create an instance of the ConsoleLogSystem in the main() of your application as shown below:

```
std::shared_ptr<awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system =
    std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);
```

Create a log tag for your application to distinguish it from the SDK logs:
```
#define LOG_TAG_APPLICATION "[Application]"
```

You can now add logging to any part of your application using AWS_LOG_ERROR or AWS_LOG_INFO as shown below:

```
AWS_LOG_ERROR(LOG_TAG_APPLICATION, "Failed to perform action. %s",
              ResponseHelper::ToString(rc).c_str());
```

<a name="license"></a>
## License

This SDK is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0), see LICENSE and NOTICE.txt for more information.

<a name="support"></a>
## Support

If you have any technical questions about AWS IoT C++ SDK, use the [AWS IoT forum](https://forums.aws.amazon.com/forum.jspa?forumID=210).
For any other questions on AWS IoT, contact [AWS Support](https://aws.amazon.com/contact-us/).
