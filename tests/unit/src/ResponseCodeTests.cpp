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
 * @file JsonParserTests.cpp
 * @brief
 *
 */

#include <gtest/gtest.h>

#include "TestHelper.hpp"

#include "ResponseCode.hpp"
#include <iostream>
#include "util/logging/LogSystemInterface.hpp"

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class ResponseCodeTester : public ::testing::Test {
            public:
                util::String ResponseCodeToString(util::String response_string, ResponseCode rc);
                ResponseCodeTester() {}
            };

            util::String ResponseCodeTester::ResponseCodeToString(util::String response_string, ResponseCode rc) {
                util::OStringStream responseText;
                responseText << response_string << " : SDK Code "
                             << static_cast<int>(rc) << ".";
                return responseText.str();
            }

            TEST_F(ResponseCodeTester, RunTests) {
                /* Test invalid file path */
                util::String response_string;
                util::String expected_string;

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_NO_INFORMATION_PRESENT_STRING,
                                                       ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_SUCCESS);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_SUCCESS_STRING,
                                                       ResponseCode::DISCOVER_ACTION_SUCCESS);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_RECEIVED_DELTA);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_RECEIVED_DELTA_STRING,
                                                       ResponseCode::SHADOW_RECEIVED_DELTA);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_REQUEST_ACCEPTED);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_REQUEST_ACCEPTED_STRING,
                                                       ResponseCode::SHADOW_REQUEST_ACCEPTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_PHYSICAL_LAYER_CONNECTED);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_PHYSICAL_LAYER_CONNECTED_STRING,
                                                       ResponseCode::NETWORK_PHYSICAL_LAYER_CONNECTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_MANUALLY_DISCONNECTED);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_MANUALLY_DISCONNECTED_STRING,
                                                       ResponseCode::NETWORK_MANUALLY_DISCONNECTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_ATTEMPTING_RECONNECT);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_ATTEMPTING_RECONNECT_STRING,
                                                       ResponseCode::NETWORK_ATTEMPTING_RECONNECT);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_RECONNECTED);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_RECONNECTED_STRING,
                                                       ResponseCode::NETWORK_RECONNECTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_NOTHING_TO_READ);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_NOTHING_TO_READ_STRING,
                                                       ResponseCode::MQTT_NOTHING_TO_READ);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_CONNECTION_ACCEPTED_STRING,
                                                       ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SUCCESS);
                expected_string = ResponseCodeToString(ResponseHelper::SUCCESS_STRING, ResponseCode::SUCCESS);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::FAILURE);
                expected_string = ResponseCodeToString(ResponseHelper::FAILURE_STRING, ResponseCode::FAILURE);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NULL_VALUE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NULL_VALUE_ERROR_STRING,
                                                       ResponseCode::NULL_VALUE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::FILE_OPEN_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::FILE_OPEN_ERROR_STRING,
                                                       ResponseCode::FILE_OPEN_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::FILE_NAME_INVALID);
                expected_string = ResponseCodeToString(ResponseHelper::FILE_NAME_INVALID_STRING,
                                                       ResponseCode::FILE_NAME_INVALID);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MUTEX_INIT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MUTEX_INIT_ERROR_STRING,
                                                       ResponseCode::MUTEX_INIT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MUTEX_LOCK_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MUTEX_LOCK_ERROR_STRING,
                                                       ResponseCode::MUTEX_LOCK_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MUTEX_UNLOCK_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MUTEX_UNLOCK_ERROR_STRING,
                                                       ResponseCode::MUTEX_UNLOCK_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MUTEX_DESTROY_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MUTEX_DESTROY_ERROR_STRING,
                                                       ResponseCode::MUTEX_DESTROY_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::THREAD_EXITING);
                expected_string = ResponseCodeToString(ResponseHelper::THREAD_EXITING_STRING,
                                                       ResponseCode::THREAD_EXITING);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_TCP_CONNECT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_TCP_CONNECT_ERROR_STRING,
                                                       ResponseCode::NETWORK_TCP_CONNECT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_TCP_SETUP_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_TCP_SETUP_ERROR_STRING,
                                                       ResponseCode::NETWORK_TCP_SETUP_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_TCP_UNKNOWN_HOST);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_TCP_UNKNOWN_HOST_STRING,
                                                       ResponseCode::NETWORK_TCP_UNKNOWN_HOST);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_TCP_NO_ENDPOINT_SPECIFIED);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_TCP_NO_ENDPOINT_SPECIFIED_STRING,
                                                       ResponseCode::NETWORK_TCP_NO_ENDPOINT_SPECIFIED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_INIT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_INIT_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_INIT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_ROOT_CRT_PARSE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_ROOT_CRT_PARSE_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_ROOT_CRT_PARSE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_KEY_PARSE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_KEY_PARSE_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_KEY_PARSE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_TLS_HANDSHAKE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_TLS_HANDSHAKE_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_TLS_HANDSHAKE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_CONNECT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_CONNECT_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_CONNECT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_CONNECT_TIMEOUT_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_CONNECTION_CLOSED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_CONNECTION_CLOSED_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_CONNECTION_CLOSED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_WRITE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_WRITE_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_WRITE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_WRITE_TIMEOUT_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_READ_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_READ_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_READ_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_READ_TIMEOUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_READ_TIMEOUT_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_READ_TIMEOUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_NOTHING_TO_READ);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_NOTHING_TO_READ_STRING,
                                                       ResponseCode::NETWORK_SSL_NOTHING_TO_READ);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_UNKNOWN_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_UNKNOWN_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_UNKNOWN_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_SSL_SERVER_VERIFICATION_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_SSL_SERVER_VERIFICATION_ERROR_STRING,
                                                       ResponseCode::NETWORK_SSL_SERVER_VERIFICATION_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_DISCONNECTED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_DISCONNECTED_ERROR_STRING,
                                                       ResponseCode::NETWORK_DISCONNECTED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_RECONNECT_TIMED_OUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_RECONNECT_TIMED_OUT_ERROR_STRING,
                                                       ResponseCode::NETWORK_RECONNECT_TIMED_OUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_ALREADY_CONNECTED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_ALREADY_CONNECTED_ERROR_STRING,
                                                       ResponseCode::NETWORK_ALREADY_CONNECTED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_PHYSICAL_LAYER_DISCONNECTED);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_PHYSICAL_LAYER_DISCONNECTED_STRING,
                                                       ResponseCode::NETWORK_PHYSICAL_LAYER_DISCONNECTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::NETWORK_NOTHING_TO_WRITE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::NETWORK_NOTHING_TO_WRITE_ERROR_STRING,
                                                       ResponseCode::NETWORK_NOTHING_TO_WRITE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::ACTION_NOT_REGISTERED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::ACTION_NOT_REGISTERED_ERROR_STRING,
                                                       ResponseCode::ACTION_NOT_REGISTERED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::ACTION_QUEUE_FULL);
                expected_string = ResponseCodeToString(ResponseHelper::ACTION_QUEUE_FULL_STRING,
                                                       ResponseCode::ACTION_QUEUE_FULL);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::ACTION_CREATE_FAILED);
                expected_string = ResponseCodeToString(ResponseHelper::ACTION_CREATE_FAILED_STRING,
                                                       ResponseCode::ACTION_CREATE_FAILED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNECTION_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNECTION_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNECTION_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNECT_TIMEOUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNECT_TIMEOUT_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNECT_TIMEOUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_REQUEST_TIMEOUT_ERROR_STRING,
                                                       ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_UNEXPECTED_CLIENT_STATE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_UNEXPECTED_CLIENT_STATE_ERROR_STRING,
                                                       ResponseCode::MQTT_UNEXPECTED_CLIENT_STATE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CLIENT_NOT_IDLE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CLIENT_NOT_IDLE_ERROR_STRING,
                                                       ResponseCode::MQTT_CLIENT_NOT_IDLE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR_STRING,
                                                       ResponseCode::MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR_STRING,
                                                       ResponseCode::MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_DECODE_REMAINING_LENGTH_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_DECODE_REMAINING_LENGTH_ERROR_STRING,
                                                       ResponseCode::MQTT_DECODE_REMAINING_LENGTH_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_UNKNOWN_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_UNKNOWN_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNACK_UNKNOWN_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_BAD_USERDATA_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_BAD_USERDATA_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNACK_BAD_USERDATA_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_CONNACK_NOT_AUTHORIZED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_CONNACK_NOT_AUTHORIZED_ERROR_STRING,
                                                       ResponseCode::MQTT_CONNACK_NOT_AUTHORIZED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_NO_SUBSCRIPTION_FOUND);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_NO_SUBSCRIPTION_FOUND_STRING,
                                                       ResponseCode::MQTT_NO_SUBSCRIPTION_FOUND);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_SUBSCRIPTION_NOT_ACTIVE);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_SUBSCRIPTION_NOT_ACTIVE_STRING,
                                                       ResponseCode::MQTT_SUBSCRIPTION_NOT_ACTIVE);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR_STRING,
                                                       ResponseCode::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST_STRING,
                                                       ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_INVALID_DATA_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_INVALID_DATA_ERROR_STRING,
                                                       ResponseCode::MQTT_INVALID_DATA_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_SUBSCRIBE_PARTIALLY_FAILED);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_SUBSCRIBE_PARTIALLY_FAILED_STRING,
                                                       ResponseCode::MQTT_SUBSCRIBE_PARTIALLY_FAILED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::MQTT_SUBSCRIBE_FAILED);
                expected_string = ResponseCodeToString(ResponseHelper::MQTT_SUBSCRIBE_FAILED_STRING,
                                                       ResponseCode::MQTT_SUBSCRIBE_FAILED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::JSON_PARSE_KEY_NOT_FOUND_ERROR_STRING,
                                                       ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR_STRING,
                                                       ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::JSON_PARSING_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::JSON_PARSING_ERROR_STRING,
                                                       ResponseCode::JSON_PARSING_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::JSON_MERGE_FAILED);
                expected_string = ResponseCodeToString(ResponseHelper::JSON_MERGE_FAILED_STRING,
                                                       ResponseCode::JSON_MERGE_FAILED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::JSON_DIFF_FAILED);
                expected_string = ResponseCodeToString(ResponseHelper::JSON_DIFF_FAILED_STRING,
                                                       ResponseCode::JSON_DIFF_FAILED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_WAIT_FOR_PUBLISH);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_WAIT_FOR_PUBLISH_STRING,
                                                       ResponseCode::SHADOW_WAIT_FOR_PUBLISH);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_JSON_BUFFER_TRUNCATED);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_JSON_BUFFER_TRUNCATED_STRING,
                                                       ResponseCode::SHADOW_JSON_BUFFER_TRUNCATED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_JSON_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_JSON_ERROR_STRING,
                                                       ResponseCode::SHADOW_JSON_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_JSON_EMPTY_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_JSON_EMPTY_ERROR_STRING,
                                                       ResponseCode::SHADOW_JSON_EMPTY_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_REQUEST_MAP_EMPTY);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_REQUEST_MAP_EMPTY_STRING,
                                                       ResponseCode::SHADOW_REQUEST_MAP_EMPTY);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_MQTT_DISCONNECTED_ERROR_STRING,
                                                       ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_UNEXPECTED_RESPONSE_TYPE_STRING,
                                                       ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TOPIC);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_UNEXPECTED_RESPONSE_TOPIC_STRING,
                                                       ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TOPIC);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_REQUEST_REJECTED);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_REQUEST_REJECTED_STRING,
                                                       ResponseCode::SHADOW_REQUEST_REJECTED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_MQTT_CLIENT_NOT_SET_ERROR_STRING,
                                                       ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_NOTHING_TO_UPDATE);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_NOTHING_TO_UPDATE_STRING,
                                                       ResponseCode::SHADOW_NOTHING_TO_UPDATE);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD_STRING,
                                                       ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::SHADOW_RECEIVED_OLD_VERSION_UPDATE);
                expected_string = ResponseCodeToString(ResponseHelper::SHADOW_RECEIVED_OLD_VERSION_UPDATE_STRING,
                                                       ResponseCode::SHADOW_RECEIVED_OLD_VERSION_UPDATE);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_SIGN_URL_NO_MEM);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_SIGN_URL_NO_MEM_STRING,
                                                       ResponseCode::WEBSOCKET_SIGN_URL_NO_MEM);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_GEN_CLIENT_KEY_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_GEN_CLIENT_KEY_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_GEN_CLIENT_KEY_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_HANDSHAKE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_HANDSHAKE_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_HANDSHAKE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_HANDSHAKE_WRITE);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_HANDSHAKE_WRITE_STRING,
                                                       ResponseCode::WEBSOCKET_HANDSHAKE_WRITE);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_HANDSHAKE_READ);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_HANDSHAKE_READ_STRING,
                                                       ResponseCode::WEBSOCKET_HANDSHAKE_READ);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_HANDSHAKE_VERIFY_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_HANDSHAKE_VERIFY_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_HANDSHAKE_VERIFY_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_FRAME_RECEIVE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_FRAME_RECEIVE_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_FRAME_RECEIVE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_FRAME_TRANSMIT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_FRAME_TRANSMIT_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_FRAME_TRANSMIT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_PROTOCOL_VIOLATION);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_PROTOCOL_VIOLATION_STRING,
                                                       ResponseCode::WEBSOCKET_PROTOCOL_VIOLATION);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_MAX_LIFETIME_REACHED);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_MAX_LIFETIME_REACHED_STRING,
                                                       ResponseCode::WEBSOCKET_MAX_LIFETIME_REACHED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_DISCONNECT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_DISCONNECT_ERROR_STRING,
                                                       ResponseCode::WEBSOCKET_DISCONNECT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::WEBSOCKET_GET_UTC_TIME_FAILED);
                expected_string = ResponseCodeToString(ResponseHelper::WEBSOCKET_GET_UTC_TIME_FAILED_STRING,
                                                       ResponseCode::WEBSOCKET_GET_UTC_TIME_FAILED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_REQUEST_FAILED_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_REQUEST_FAILED_ERROR_STRING,
                                                       ResponseCode::DISCOVER_ACTION_REQUEST_FAILED_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR_STRING,
                                                       ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_UNAUTHORIZED);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_UNAUTHORIZED_STRING,
                                                       ResponseCode::DISCOVER_ACTION_UNAUTHORIZED);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_SERVER_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_SERVER_ERROR_STRING,
                                                       ResponseCode::DISCOVER_ACTION_SERVER_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_ACTION_REQUEST_OVERLOAD);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_ACTION_REQUEST_OVERLOAD_STRING,
                                                       ResponseCode::DISCOVER_ACTION_REQUEST_OVERLOAD);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR_STRING,
                                                       ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR);
                EXPECT_EQ(expected_string, response_string);

                response_string = ResponseHelper::ToString(ResponseCode::JOBS_INVALID_TOPIC_ERROR);
                expected_string = ResponseCodeToString(ResponseHelper::JOBS_INVALID_TOPIC_ERROR_STRING,
                                                       ResponseCode::JOBS_INVALID_TOPIC_ERROR);
                EXPECT_EQ(expected_string, response_string);
            }
        }
    }
}
