/*
 * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file GreengrassMqttClient.hpp
 * @brief Contains the MQTT Client class for AWS Greengrass devices
 *
 * Defines MQTT client wrapper using a Client Core instance. This is provided
 * for ease of use. Instead of separately having to define Core Client and adding
 * Actions to the client, applications can use this class directly. Similar to
 * the Client class but also contains functions for Discovery.
 */

#pragma once

#include "util/Utf8String.hpp"
#include "util/JsonParser.hpp"

#include "ClientCore.hpp"

#include "mqtt/Connect.hpp"
#include "mqtt/Publish.hpp"
#include "mqtt/Subscribe.hpp"
#include "mqtt/ClientState.hpp"

#include "discovery/DiscoveryResponse.hpp"
#include "mqtt/Client.hpp"

namespace awsiotsdk {

    /**
     * @brief MQTT Client Class
     *
     * Defining a class for the MQTT Client.
     * This class is a wrapper on the Core Client and creates a Client Core instance with MQTT Actions
     * It also provides APIs to perform MQTT operations directly on the Core Client instance
     *
     */
    AWS_API_EXPORT class GreengrassMqttClient : public MqttClient {
    protected:
        /**
         * @brief Constructor
         *
         * @param p_network_connection - Network connection to use with this MQTT Client instance
         * @param mqtt_command_timeout - Command timeout in milliseconds for internal blocking operations (Reconnect and Resubscribe)
         * @param callback_ptr - pointer of the disconnect callback handler
         * @param app_handler_data - context data for the disconnect handler
         */
        GreengrassMqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                             std::chrono::milliseconds mqtt_command_timeout,
                             ClientCoreState::ApplicationDisconnectCallbackPtr p_callback_ptr,
                             std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data);

        /**
         * @brief Constructor
         *
         * @param p_network_connection - Network connection to use with this MQTT Client instance
         * @param mqtt_command_timeout - Command timeout in milliseconds for internal blocking operations (Reconnect and Resubscribe)
         */
        GreengrassMqttClient(std::shared_ptr<NetworkConnection> p_network_connection,
                             std::chrono::milliseconds mqtt_command_timeout);

    public:

        // Disabling default and copy constructors. Defining a virtual destructor
        // Client instances should not be copied to avoid possible Connection issues with two clients
        // using same connection data
        GreengrassMqttClient() = delete;                                           // Delete Default constructor
        GreengrassMqttClient(const GreengrassMqttClient &) = delete;               // Delete Copy constructor
        GreengrassMqttClient(GreengrassMqttClient &&) = default;                   // Default Move constructor
        GreengrassMqttClient &operator=(const GreengrassMqttClient &) & = delete;  // Delete Copy assignment operator
        GreengrassMqttClient &operator=(GreengrassMqttClient &&) & = default;      // Default Move assignment operator
        virtual ~GreengrassMqttClient();

        /**
         * @brief Create factory method. Returns a unique instance of GreengrassMqttClient
         *
         * @param p_network_connection - Network connection to use with this MQTT Client instance
         * @param mqtt_command_timeout - Command timeout in milliseconds for internal blocking operations (Reconnect and Resubscribe)
         *
         * @return std::unique_ptr<GreengrassMqttClient> pointing to a unique MQTT client instance
         */
        static std::unique_ptr<GreengrassMqttClient> Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                            std::chrono::milliseconds mqtt_command_timeout);

        /**
         * @brief Create factory method, with additional parameters for disconnect callback.
         *
         * @param networkConnection  - Network connection to use with this MQTT Client instance
         * @param mqtt_command_timeout - Command timeout in milliseconds for internal blocking operations (Reconnect and Resubscribe)
         * @param callback_ptr - pointer of the disconnect callback handler
         * @param app_handler_data - context data for the disconnect handler
         * @return std::unique_ptr<MqttClient> pointing to a unique MQTT client instance
         */
        static std::unique_ptr<GreengrassMqttClient> Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                            std::chrono::milliseconds mqtt_command_timeout,
                                                            ClientCoreState::ApplicationDisconnectCallbackPtr p_callback_ptr,
                                                            std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data);

        /**
         * @brief Performs a Sync Discovery operation
         *
         * Performs a blocking discovery operation to receive the connectivity information for the GGCs in the group
         * this device belongs to. Returns DISCOVERY_RESPONSE_SUCCESS if successful. The action timeout is the time for
         * which the client waits for a response AFTER the request is sent.
         *
         * @param action_response_timeout
         * @param p_thing_name
         * @param discovery_response_payload
         * @return
         */
        virtual ResponseCode Discover(std::chrono::milliseconds action_reponse_timeout,
                                      std::unique_ptr<Utf8String> p_thing_name,
                                      DiscoveryResponse &discovery_response);
    };
}
