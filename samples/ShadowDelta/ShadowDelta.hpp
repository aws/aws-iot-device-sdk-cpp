/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * @file ShadowDelta.hpp
 * @brief
 *
 */


#pragma once

#include "mqtt/Client.hpp"
#include "NetworkConnection.hpp"
#include "shadow/Shadow.hpp"

namespace awsiotsdk {
    namespace samples {
        class ShadowDelta {
        protected:
            std::shared_ptr<NetworkConnection> p_network_connection_;
            std::shared_ptr<mqtt::ConnectPacket> p_connect_packet_;
            std::atomic_int cur_pending_messages_;
            std::atomic_int total_published_messages_;
            std::shared_ptr<MqttClient> p_iot_client_;
            std::atomic_bool publish_mqtt_messages_;

            ResponseCode InitializeTLS();
            ResponseCode ActionResponseHandler(util::String thing_name, ShadowRequestType request_type,
                                               ShadowResponseType response_type, util::JsonDocument &payload);

            std::mutex sync_action_response_lock_;               ///< Mutex for Sync Action Response flow
            std::condition_variable sync_action_response_wait_;  ///< Condition variable used to wake up calling thread on Sync Action response
            ResponseCode sync_action_response_;                  ///< Variable to store received Sync Action response
        public:
            ResponseCode RunSample();
        };
    }
}


