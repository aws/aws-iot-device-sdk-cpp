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
 * @file SampleCommon.hpp
 * @brief
 *
 */


#pragma once

#include <chrono>

#include "util/memory/stl/String.hpp"
#include "util/JsonParser.hpp"

namespace awsiotsdk {
    class ConfigCommon {
    protected:
        static util::JsonDocument sdk_config_json_;

        static void LogParseError(const ResponseCode& response_code, const util::JsonDocument& config, util::String key);
    public:
        static uint16_t endpoint_mqtt_port_;
        static uint16_t endpoint_https_port_;
        static uint16_t endpoint_greengrass_discovery_port_;

        static util::String endpoint_;
        static util::String root_ca_path_;
        static util::String client_cert_path_;
        static util::String client_key_path_;
        static util::String base_client_id_;
        static util::String thing_name_;
        static util::String aws_region_;
        static util::String aws_access_key_id_;
        static util::String aws_secret_access_key_;
        static util::String aws_session_token_;

        static std::chrono::milliseconds mqtt_command_timeout_;
        static std::chrono::milliseconds tls_handshake_timeout_;
        static std::chrono::milliseconds tls_read_timeout_;
        static std::chrono::milliseconds tls_write_timeout_;
        static std::chrono::milliseconds discover_action_timeout_;
        static std::chrono::seconds keep_alive_timeout_secs_;

        static bool is_clean_session_;
        static std::chrono::seconds minimum_reconnect_interval_;
        static std::chrono::seconds maximum_reconnect_interval_;
        static size_t max_pending_acks_;
        static size_t maximum_outgoing_action_queue_length_;
        static uint32_t action_processing_rate_hz_;

        static ResponseCode InitializeCommon(const util::String &config_file_path);
        static util::String GetCurrentPath();
    };
}


