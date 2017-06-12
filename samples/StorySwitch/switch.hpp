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
 * @file switch.hpp
 * @brief
 *
 */


#pragma once

#include "mqtt/GreengrassMqttClient.hpp"
#include "NetworkConnection.hpp"
#include "discovery/DiscoveryResponse.hpp"

namespace awsiotsdk {
    namespace samples {
        class SwitchThing {
        protected:
            std::shared_ptr <NetworkConnection> p_network_connection_;
            std::shared_ptr <GreengrassMqttClient> p_iot_client_;

            ResponseCode RunPublish();
            static bool ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2);

        public:
            ResponseCode RunSample();
        };
    }
}


