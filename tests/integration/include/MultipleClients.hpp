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
 * @file MultipleClients.hpp
 * @brief
 *
 */


#pragma once

#include "mqtt/Client.hpp"

namespace awsiotsdk {
    namespace tests {
        namespace integration {
            class MultipleClients {
            protected:
                std::shared_ptr<NetworkConnection> p_network_connection_1_;
                std::shared_ptr<MqttClient> p_iot_client_1_;

                std::shared_ptr<NetworkConnection> p_network_connection_2_;
                std::shared_ptr<MqttClient> p_iot_client_2_;

                std::shared_ptr<NetworkConnection> p_network_connection_3_;
                std::shared_ptr<MqttClient> p_iot_client_3_;

                std::atomic_int cur_pending_messages_;
                std::atomic_int total_published_messages_;
                std::chrono::milliseconds mqtt_command_timeout_;

                ResponseCode RunPublish(int msg_count);
                ResponseCode SubscribeCallback(util::String topic_name,
                                               util::String payload,
                                               std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
                ResponseCode Subscribe();
                ResponseCode Unsubscribe();
                ResponseCode InitializeTLS(std::shared_ptr<NetworkConnection> &p_network_connection);

            public:
                ResponseCode RunTest();
            };
        }
    }
}


