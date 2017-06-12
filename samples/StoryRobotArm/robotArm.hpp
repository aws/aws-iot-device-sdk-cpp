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
 * @file robotArm.hpp
 * @brief
 *
 */


#pragma once

#include "mqtt/GreengrassMqttClient.hpp"
#include "NetworkConnection.hpp"
#include "discovery/DiscoveryResponse.hpp"
#include "shadow/Shadow.hpp"

namespace awsiotsdk {
    namespace samples {
        class RobotArmThing {
        protected:
            std::shared_ptr <NetworkConnection> p_network_connection_;
            std::shared_ptr <GreengrassMqttClient> p_iot_client_;

            ResponseCode RunPublish();
            static bool ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2);
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


