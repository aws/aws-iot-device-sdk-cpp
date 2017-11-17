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
 * @file SdkConfig.hpp
 * @brief
 *
 */

#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "ResponseCode.hpp"

// Network settings
#define SDK_CONFIG_ENDPOINT_KEY "endpoint"
#define SDK_CONFIG_ENDPOINT_PORT_KEY "port"

// TLS Settings
#define SDK_CONFIG_ROOT_CA_RELATIVE_KEY "root_ca_relative_path"
#define SDK_CONFIG_DEVICE_CERT_RELATIVE_KEY "device_certificate_relative_path"
#define SDK_CONFIG_DEVICE_PRIVATE_KEY_RELATIVE_KEY "device_private_key_relative_path"
#define SDK_CONFIG_TLS_HANDSHAKE_TIMEOUT_SECS_KEY "tls_handshake_timeout_secs"
#define SDK_CONFIG_TLS_READ_TIMEOUT_SECS_KEY "tls_read_timeout_msecs"
#define SDK_CONFIG_TLS_WRITE_TIMEOUT_SECS_KEY "tls_write_timeout_msecs"

// Websocket settings
#define SDK_CONFIG_AWS_ACCESS_KEY_ID_KEY "aws_access_key_id"
#define SDK_CONFIG_SECRET_ACCESS_KEY "aws_secret_access_key"
#define SDK_CONFIG_AWS_SESSION_TOKEN_KEY "aws_session_token"

// MQTT Settings
#define SDK_CONFIG_CLIENT_ID_KEY "client_id"
#define SDK_CONFIG_THING_NAME_KEY "thing_name"
#define SDK_CONFIG_IS_CLEAN_SESSION_KEY "is_clean_session"
#define SDK_CONFIG_MQTT_COMMAND_TIMEOUT_MSECS_KEY "mqtt_command_timeout_msecs"
#define SDK_CONFIG_KEEPALIVE_INTERVAL_SECS_KEY "keepalive_interval_secs"
#define SDK_CONFIG_MIN_RECONNECT_INTERVAL_SECS_KEY "minimum_reconnect_interval_secs"
#define SDK_CONFIG_MAX_RECONNECT_INTERVAL_SECS_KEY "maximum_reconnect_interval_secs"
#define SDK_CONFIG_MAX_ACKS_TO_WAIT_FOR_KEY "maximum_acks_to_wait_for"

// Core settings
#define SDK_CONFIG_MAX_TX_ACTION_QUEUE_LENGTH_KEY "maximum_outgoing_action_queue_length"
#define SDK_CONFIG_DRAINING_INTERVAL_MSECS_KEY "draining_interval_msecs"

namespace awsiotsdk {
    namespace util {
        class SdkTestConfig {
        protected:
            static rapidjson::Document sdkConfigJson;
        public:
            static ResponseCode InitializeFromJsonFile(const util::String &configFilePath);

            static ResponseCode InitializeFromJsonString(const util::String &configJsonString);

            static ResponseCode GetBoolValue(const char *key, bool &value);

            static ResponseCode GetIntValue(const char *key, int &value);

            static ResponseCode GetUint16Value(const char *key, uint16_t &value);

            static ResponseCode GetUint32Value(const char *key, uint32_t &value);

            static ResponseCode GetSizeTValue(const char *key, size_t &value);

            static ResponseCode GetCStringValue(const char *key, char *value, uint16_t max_string_len);

            static ResponseCode GetStringValue(const char *key, util::String &value);

            static rapidjson::ParseErrorCode GetParseErrorCode();

            static size_t GetParseErrorOffset();

            static void PrintSdkConfig() {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                sdkConfigJson.Accept(writer);

                const char *document = buffer.GetString();
                std::cout << std::endl << "Json Document" << std::endl;
                std::cout << document << std::endl;
            }
        };
    }
}
