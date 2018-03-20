/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * @file JobsSample.hpp
 * @brief
 *
 */


#pragma once

#include "mqtt/Client.hpp"
#include "NetworkConnection.hpp"

namespace awsiotsdk {
    namespace samples {
        class JobsSample {
        protected:
            std::shared_ptr<NetworkConnection> p_network_connection_;
            std::shared_ptr<MqttClient> p_iot_client_;
            std::shared_ptr<Jobs> p_jobs_;
            std::atomic<bool> done_;

            ResponseCode DisconnectCallback(util::String topic_name,
                                            std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data);
            ResponseCode ReconnectCallback(util::String client_id,
                                           std::shared_ptr<ReconnectCallbackContextData> p_app_handler_data,
                                           ResponseCode reconnect_result);
            ResponseCode ResubscribeCallback(util::String client_id,
                                             std::shared_ptr<ResubscribeCallbackContextData> p_app_handler_data,
                                             ResponseCode resubscribe_result);

            ResponseCode GetPendingCallback(util::String topic_name,
                                            util::String payload,
                                            std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
            ResponseCode NextJobCallback(util::String topic_name,
                                         util::String payload,
                                         std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
            ResponseCode UpdateAcceptedCallback(util::String topic_name,
                                                util::String payload,
                                                std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
            ResponseCode UpdateRejectedCallback(util::String topic_name,
                                                util::String payload,
                                                std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);

            ResponseCode Subscribe();
            ResponseCode InitializeTLS();

        public:
            ResponseCode RunSample();
        };
    }
}


