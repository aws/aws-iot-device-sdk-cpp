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
 * @file cli.hpp
 * @brief
 *
 */

#pragma once

#ifdef WIN32
#define MAX_PATH_LENGTH_ 260
#include <direct.h>
#define getcwd _getcwd // avoid MSFT "deprecation" warning
#else
#include <limits>
#define MAX_PATH_LENGTH_ PATH_MAX
#endif

#include "NetworkConnection.hpp"

#include "ResponseCode.hpp"
#include "ClientCore.hpp"
#include "mqtt/Client.hpp"
#include "util/memory/stl/Vector.hpp"
#include "util/Utf8String.hpp"

using namespace awsiotsdk;

namespace cppsdkcli {
    class CLI {
    protected:
        int port_;
        int qos_;
        bool is_publish_;
        bool is_subscribe_;
        char topic_[MAX_PATH_LENGTH_];
        util::String endpoint_;
        std::shared_ptr<awsiotsdk::MqttClient> p_iot_client_;
        std::shared_ptr<awsiotsdk::NetworkConnection> p_network_connection_;

        ResponseCode InitializeTLS();

        ResponseCode InitializeCliConfig();

        ResponseCode Connect();

        ResponseCode RunPublish();

        ResponseCode RunPublish(int msg_count);

        ResponseCode Subscribe(std::unique_ptr<Utf8String> p_topic_name, awsiotsdk::mqtt::QoS qos);

        ResponseCode RunSubscribe();

        util::JsonDocument cli_config_;

        static ResponseCode SubscribeCallback(util::String topic_name,
                                              util::String payload,
                                              std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);

    public:
        CLI() {
            port_ = 8883;
            qos_ = 0;
            is_publish_ = false;
            is_subscribe_ = false;
            topic_[0] = '\0';
            endpoint_[0] = '\0';
            p_iot_client_ = nullptr;
            p_network_connection_ = nullptr;
        }
        awsiotsdk::ResponseCode InitializeCLI(int argc, char **argv);

        awsiotsdk::ResponseCode RunCLI();
    };
}
