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
 * @file Discovery.hpp
 * @brief
 *
 */

#pragma once

#include <chrono>

#include "util/JsonParser.hpp"

#include "mqtt/Packet.hpp"
#include "mqtt/ClientState.hpp"
#include "discovery/DiscoveryResponse.hpp"

namespace awsiotsdk {
    namespace discovery {
        /**
         * @brief Discover Request Packet Type
         *
         * Defines a type for Discover Request Packet message
         */
        class DiscoverRequestData : public ActionData {
        protected:
            std::unique_ptr<Utf8String> p_thing_name_;          ///< Utf8 string defining the Thing name
            util::String discovery_request_data_;               ///< Packet data
            std::chrono::milliseconds max_response_wait_time_;  ///< Maximum time the device should wait for response

        public:
            DiscoveryResponse discovery_response_;              ///< Response received in Discover request

            // Rule of 5 stuff
            // Disable copying and moving because class contains DiscoveryResponse
            // Delete Default constructor
            DiscoverRequestData() = delete;                                         // Default constructor
            DiscoverRequestData(const DiscoverRequestData &) = delete;              // Copy constructor
            DiscoverRequestData(DiscoverRequestData &&) = delete;                   // Move constructor
            DiscoverRequestData &operator=(const DiscoverRequestData &) & = delete; // Copy assignment operator
            DiscoverRequestData &operator=(DiscoverRequestData &&) & = delete;      // Move assignment operator
            virtual ~DiscoverRequestData() = default;                               // Default destructor

            /**
             * @brief Constructor
             *
             * @warning This constructor can throw exceptions, it is recommended to use Factory create method
             * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
             *
             * @param p_thing_name_ - Thing name to use to perform discovery
             * @param max_response_wait_time - discovery reply timeout in milliseconds
             */
            DiscoverRequestData(std::unique_ptr<Utf8String> p_thing_name,
                                std::chrono::milliseconds max_response_wait_time);

            /**
             * @brief Create Factory method
             *
             * @param p_thing_name_ - Thing name to use to perform discovery
             * @param max_response_wait_time - max time for which it will wait for a discovery reply, in milliseconds
             * @return nullptr on error, shared_ptr pointing to a created DiscoverRequestData instance if successful
             */
            static std::shared_ptr<DiscoverRequestData> Create(std::unique_ptr<Utf8String> p_thing_name,
                                                               std::chrono::milliseconds max_response_wait_time);

            /**
             * @brief Serialize this packet into a String
             * @return String containing serialized packet
             */
            util::String ToString();

            /**
             * @brief return the max time for which it will wait for a discovery reply
             * @return wait time in milliseconds
             */
            std::chrono::milliseconds GetMaxResponseWaitTime() { return max_response_wait_time_; }

            /**
             * @brief returns the action ID (currently unused in Discovery)
             * @return uint16_t
             */
            virtual uint16_t GetActionId() { return 0; }

            /**
             * @brief sets the action ID (currently unused in Discovery)
             * @param action_id
             */
            virtual void SetActionId(uint16_t action_id) { IOT_UNUSED(action_id); }
        };

        /**
         * @brief Define a class for DiscoverAction
         *
         * This class defines a Synchronous action for performing an AWS Greengrass Discovery operation
         */
        class DiscoverAction : public Action {
        protected:
            std::shared_ptr<mqtt::ClientState> p_client_state_;        ///< Shared Client State instance

            /**
             * @brief Parses the discovery response to get the header and response data.
             *
             * Parses the discovery response to obtain the header and response payload. Returns a SUCCESS when it is able to parse
             * it correctly. Otherwise returns error codes if the discovery request fails.
             *
             * @param sent_packet
             * @param read_payload
             * @param max_response_wait_time
             * @return ResponseCode
             */
            ResponseCode ReadResponseFromNetwork(std::shared_ptr<NetworkConnection> p_network_connection,
                                                 util::String &sent_packet,
                                                 util::String &read_payload,
                                                 std::chrono::milliseconds max_response_wait_time);

            /**
             * @brief Make the discovery request
             *
             * Writes the discovery request to the network. Returns a SUCCESS when it is able to write to the network
             * correctly. Returns error code otherwise.
             *
             * @param p_network_connection
             * @param p_discover_packet
             * @return ResponseCode
             */
            ResponseCode MakeDiscoveryRequest(std::shared_ptr<NetworkConnection> p_network_connection,
                                              const util::String packet_data);

            /**
             * @brief Initialize the discovery response json
             *
             * Convert the received discovery response and insert it into the discover packet. Returns a SUCCESS if it
             * able to covert it into a Json successfully. Returns error code otherwise.
             *
             * @param received_response
             * @param discover_packet
             * @return ResponseCode
             */
            ResponseCode InitializeDiscoveryResponseJson(const util::String received_response,
                                                         std::shared_ptr<DiscoverRequestData> discover_packet);

        public:
            // Disabling default, move and copy constructors to match Action parent
            // Default virtual destructor
            DiscoverAction() = delete;
            // Default Copy constructor
            DiscoverAction(const DiscoverAction &) = delete;
            // Default Move constructor
            DiscoverAction(DiscoverAction &&) = delete;
            // Default Copy assignment operator
            DiscoverAction &operator=(const DiscoverAction &) & = delete;
            // Default Move assignment operator
            DiscoverAction &operator=(DiscoverAction &&) & = delete;
            // Default destructor
            virtual ~DiscoverAction() = default;

            /**
             * @brief Constructor
             *
             * @warning This constructor can throw exceptions, it is recommended to use Factory create method
             * Constructor is kept public to not restrict usage possibilities (eg. make_shared)
             *
             * @param p_client_state - Shared Client State instance
             */
            DiscoverAction(std::shared_ptr<mqtt::ClientState> p_client_state);

            /**
             * @brief Factory Create method
             *
             * @param p_client_state - Shared Client State instance
             * @return nullptr on error, unique_ptr pointing to a created DiscoverAction instance if successful
             */
            static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);

            /**
             * @brief Performs the Discovery Action
             *
             * Performs the Discovery operation to get the connectivity information of GGCs in the group that this device belongs to by making
             * an HTTP GET request to the endpoint. Returns a DISCOVER_ACTION_SUCCESS response when connectivity information
             * is found. Otherwise returns error codes based on whether it's an HTTP error or if connectivity information is not present.
             *
             * @param p_network_connection
             * @param p_action_data
             * @return ResponseCode
             */
            ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                       std::shared_ptr<ActionData> p_action_data);
        };
    }
}
