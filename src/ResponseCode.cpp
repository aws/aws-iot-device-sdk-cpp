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
 * @file ResponseCode.cpp
 * @brief Response Code class with helper functions
 *
 * Defines a helper functions to convert Response Codes to strings
 *
 */

#include "ResponseCode.hpp"
#include "util/logging/LogSystemInterface.hpp"
#include <iostream>

namespace awsiotsdk {
    std::ostream &operator<<(std::ostream &os, ResponseCode rc) {
        switch (rc) {
            case ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_NO_INFORMATION_PRESENT_STRING;
                break;
            case ResponseCode::DISCOVER_ACTION_SUCCESS:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_SUCCESS_STRING;
                break;
            case ResponseCode::SHADOW_RECEIVED_DELTA:
                os << awsiotsdk::ResponseHelper::SHADOW_RECEIVED_DELTA_STRING;
                break;
            case ResponseCode::SHADOW_REQUEST_ACCEPTED:
                os << awsiotsdk::ResponseHelper::SHADOW_REQUEST_ACCEPTED_STRING;
                break;
            case ResponseCode::NETWORK_PHYSICAL_LAYER_CONNECTED:
                os << awsiotsdk::ResponseHelper::NETWORK_PHYSICAL_LAYER_CONNECTED_STRING;
                break;
            case ResponseCode::NETWORK_MANUALLY_DISCONNECTED:
                os << awsiotsdk::ResponseHelper::NETWORK_MANUALLY_DISCONNECTED_STRING;
                break;
            case ResponseCode::NETWORK_ATTEMPTING_RECONNECT:
                os << awsiotsdk::ResponseHelper::NETWORK_ATTEMPTING_RECONNECT_STRING;
                break;
            case ResponseCode::NETWORK_RECONNECTED:
                os << awsiotsdk::ResponseHelper::NETWORK_RECONNECTED_STRING;
                break;
            case ResponseCode::MQTT_NOTHING_TO_READ:
                os << awsiotsdk::ResponseHelper::MQTT_NOTHING_TO_READ_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_CONNECTION_ACCEPTED_STRING;
                break;
            case ResponseCode::SUCCESS:
                os << awsiotsdk::ResponseHelper::SUCCESS_STRING;
                break;
            case ResponseCode::FAILURE:
                os << awsiotsdk::ResponseHelper::FAILURE_STRING;
                break;
            case ResponseCode::NULL_VALUE_ERROR:
                os << awsiotsdk::ResponseHelper::NULL_VALUE_ERROR_STRING;
                break;
            case ResponseCode::FILE_OPEN_ERROR:
                os << awsiotsdk::ResponseHelper::FILE_OPEN_ERROR_STRING;
                break;
            case ResponseCode::FILE_NAME_INVALID:
                os << awsiotsdk::ResponseHelper::FILE_NAME_INVALID_STRING;
                break;
            case ResponseCode::MUTEX_INIT_ERROR:
                os << awsiotsdk::ResponseHelper::MUTEX_INIT_ERROR_STRING;
                break;
            case ResponseCode::MUTEX_LOCK_ERROR:
                os << awsiotsdk::ResponseHelper::MUTEX_LOCK_ERROR_STRING;
                break;
            case ResponseCode::MUTEX_UNLOCK_ERROR:
                os << awsiotsdk::ResponseHelper::MUTEX_UNLOCK_ERROR_STRING;
                break;
            case ResponseCode::MUTEX_DESTROY_ERROR:
                os << awsiotsdk::ResponseHelper::MUTEX_DESTROY_ERROR_STRING;
                break;
            case ResponseCode::THREAD_EXITING:
                os << awsiotsdk::ResponseHelper::THREAD_EXITING_STRING;
                break;
            case ResponseCode::NETWORK_TCP_CONNECT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_TCP_CONNECT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_TCP_SETUP_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_TCP_SETUP_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_TCP_UNKNOWN_HOST:
                os << awsiotsdk::ResponseHelper::NETWORK_TCP_UNKNOWN_HOST_STRING;
                break;
            case ResponseCode::NETWORK_TCP_NO_ENDPOINT_SPECIFIED:
                os << awsiotsdk::ResponseHelper::NETWORK_TCP_NO_ENDPOINT_SPECIFIED_STRING;
                break;
            case ResponseCode::NETWORK_SSL_INIT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_INIT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_ROOT_CRT_PARSE_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_ROOT_CRT_PARSE_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_KEY_PARSE_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_KEY_PARSE_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_TLS_HANDSHAKE_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_TLS_HANDSHAKE_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_CONNECT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_CONNECT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_CONNECT_TIMEOUT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_CONNECTION_CLOSED_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_CONNECTION_CLOSED_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_WRITE_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_WRITE_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_WRITE_TIMEOUT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_READ_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_READ_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_READ_TIMEOUT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_READ_TIMEOUT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_NOTHING_TO_READ:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_NOTHING_TO_READ_STRING;
                break;
            case ResponseCode::NETWORK_SSL_UNKNOWN_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_UNKNOWN_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_SSL_SERVER_VERIFICATION_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_SSL_SERVER_VERIFICATION_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_DISCONNECTED_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_DISCONNECTED_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_RECONNECT_TIMED_OUT_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_RECONNECT_TIMED_OUT_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_ALREADY_CONNECTED_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_ALREADY_CONNECTED_ERROR_STRING;
                break;
            case ResponseCode::NETWORK_PHYSICAL_LAYER_DISCONNECTED:
                os << awsiotsdk::ResponseHelper::NETWORK_PHYSICAL_LAYER_DISCONNECTED_STRING;
                break;
            case ResponseCode::NETWORK_NOTHING_TO_WRITE_ERROR:
                os << awsiotsdk::ResponseHelper::NETWORK_NOTHING_TO_WRITE_ERROR_STRING;
                break;
            case ResponseCode::ACTION_NOT_REGISTERED_ERROR:
                os << awsiotsdk::ResponseHelper::ACTION_NOT_REGISTERED_ERROR_STRING;
                break;
            case ResponseCode::ACTION_QUEUE_FULL:
                os << awsiotsdk::ResponseHelper::ACTION_QUEUE_FULL_STRING;
                break;
            case ResponseCode::ACTION_CREATE_FAILED:
                os << awsiotsdk::ResponseHelper::ACTION_CREATE_FAILED_STRING;
                break;
            case ResponseCode::MQTT_CONNECTION_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNECTION_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNECT_TIMEOUT_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNECT_TIMEOUT_ERROR_STRING;
                break;
            case ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_REQUEST_TIMEOUT_ERROR_STRING;
                break;
            case ResponseCode::MQTT_UNEXPECTED_CLIENT_STATE_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_UNEXPECTED_CLIENT_STATE_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CLIENT_NOT_IDLE_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CLIENT_NOT_IDLE_ERROR_STRING;
                break;
            case ResponseCode::MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR_STRING;
                break;
            case ResponseCode::MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR_STRING;
                break;
            case ResponseCode::MQTT_DECODE_REMAINING_LENGTH_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_DECODE_REMAINING_LENGTH_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_UNKNOWN_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_UNKNOWN_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_BAD_USERDATA_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_BAD_USERDATA_ERROR_STRING;
                break;
            case ResponseCode::MQTT_CONNACK_NOT_AUTHORIZED_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_CONNACK_NOT_AUTHORIZED_ERROR_STRING;
                break;
            case ResponseCode::MQTT_NO_SUBSCRIPTION_FOUND:
                os << awsiotsdk::ResponseHelper::MQTT_NO_SUBSCRIPTION_FOUND_STRING;
                break;
            case ResponseCode::MQTT_SUBSCRIPTION_NOT_ACTIVE:
                os << awsiotsdk::ResponseHelper::MQTT_SUBSCRIPTION_NOT_ACTIVE_STRING;
                break;
            case ResponseCode::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR_STRING;
                break;
            case ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST:
                os << awsiotsdk::ResponseHelper::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST_STRING;
                break;
            case ResponseCode::MQTT_INVALID_DATA_ERROR:
                os << awsiotsdk::ResponseHelper::MQTT_INVALID_DATA_ERROR_STRING;
                break;
            case ResponseCode::MQTT_SUBSCRIBE_PARTIALLY_FAILED:
                os << awsiotsdk::ResponseHelper::MQTT_SUBSCRIBE_PARTIALLY_FAILED_STRING;
                break;
            case ResponseCode::MQTT_SUBSCRIBE_FAILED:
                os << awsiotsdk::ResponseHelper::MQTT_SUBSCRIBE_FAILED_STRING;
                break;
            case ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR:
                os << awsiotsdk::ResponseHelper::JSON_PARSE_KEY_NOT_FOUND_ERROR_STRING;
                break;
            case ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR:
                os << awsiotsdk::ResponseHelper::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR_STRING;
                break;
            case ResponseCode::JSON_PARSING_ERROR:
                os << awsiotsdk::ResponseHelper::JSON_PARSING_ERROR_STRING;
                break;
            case ResponseCode::JSON_MERGE_FAILED:
                os << awsiotsdk::ResponseHelper::JSON_MERGE_FAILED_STRING;
                break;
            case ResponseCode::JSON_DIFF_FAILED:
                os << awsiotsdk::ResponseHelper::JSON_DIFF_FAILED_STRING;
                break;
            case ResponseCode::SHADOW_WAIT_FOR_PUBLISH:
                os << awsiotsdk::ResponseHelper::SHADOW_WAIT_FOR_PUBLISH_STRING;
                break;
            case ResponseCode::SHADOW_JSON_BUFFER_TRUNCATED:
                os << awsiotsdk::ResponseHelper::SHADOW_JSON_BUFFER_TRUNCATED_STRING;
                break;
            case ResponseCode::SHADOW_JSON_ERROR:
                os << awsiotsdk::ResponseHelper::SHADOW_JSON_ERROR_STRING;
                break;
            case ResponseCode::SHADOW_JSON_EMPTY_ERROR:
                os << awsiotsdk::ResponseHelper::SHADOW_JSON_EMPTY_ERROR_STRING;
                break;
            case ResponseCode::SHADOW_REQUEST_MAP_EMPTY:
                os << awsiotsdk::ResponseHelper::SHADOW_REQUEST_MAP_EMPTY_STRING;
                break;
            case ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR:
                os << awsiotsdk::ResponseHelper::SHADOW_MQTT_DISCONNECTED_ERROR_STRING;
                break;
            case ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE:
                os << awsiotsdk::ResponseHelper::SHADOW_UNEXPECTED_RESPONSE_TYPE_STRING;
                break;
            case ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TOPIC:
                os << awsiotsdk::ResponseHelper::SHADOW_UNEXPECTED_RESPONSE_TOPIC_STRING;
                break;
            case ResponseCode::SHADOW_REQUEST_REJECTED:
                os << awsiotsdk::ResponseHelper::SHADOW_REQUEST_REJECTED_STRING;
                break;
            case ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR:
                os << awsiotsdk::ResponseHelper::SHADOW_MQTT_CLIENT_NOT_SET_ERROR_STRING;
                break;
            case ResponseCode::SHADOW_NOTHING_TO_UPDATE:
                os << awsiotsdk::ResponseHelper::SHADOW_NOTHING_TO_UPDATE_STRING;
                break;
            case ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD:
                os << awsiotsdk::ResponseHelper::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD_STRING;
                break;
            case ResponseCode::SHADOW_RECEIVED_OLD_VERSION_UPDATE:
                os << awsiotsdk::ResponseHelper::SHADOW_RECEIVED_OLD_VERSION_UPDATE_STRING;
                break;
            case ResponseCode::WEBSOCKET_SIGN_URL_NO_MEM:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_SIGN_URL_NO_MEM_STRING;
                break;
            case ResponseCode::WEBSOCKET_GEN_CLIENT_KEY_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_GEN_CLIENT_KEY_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_HANDSHAKE_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_WRITE:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_HANDSHAKE_WRITE_STRING;
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_READ:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_HANDSHAKE_READ_STRING;
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_VERIFY_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_HANDSHAKE_VERIFY_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_FRAME_RECEIVE_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_FRAME_RECEIVE_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_FRAME_TRANSMIT_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_FRAME_TRANSMIT_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_PROTOCOL_VIOLATION:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_PROTOCOL_VIOLATION_STRING;
                break;
            case ResponseCode::WEBSOCKET_MAX_LIFETIME_REACHED:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_MAX_LIFETIME_REACHED_STRING;
                break;
            case ResponseCode::WEBSOCKET_DISCONNECT_ERROR:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_DISCONNECT_ERROR_STRING;
                break;
            case ResponseCode::WEBSOCKET_GET_UTC_TIME_FAILED:
                os << awsiotsdk::ResponseHelper::WEBSOCKET_GET_UTC_TIME_FAILED_STRING;
                break;
            case ResponseCode::DISCOVER_ACTION_REQUEST_FAILED_ERROR:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_REQUEST_FAILED_ERROR_STRING;
                break;
            case ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR_STRING;
                break;
            case ResponseCode::DISCOVER_ACTION_UNAUTHORIZED:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_UNAUTHORIZED_STRING;
                break;
            case ResponseCode::DISCOVER_ACTION_SERVER_ERROR:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_SERVER_ERROR_STRING;
                break;
            case ResponseCode::DISCOVER_ACTION_REQUEST_OVERLOAD:
                os << awsiotsdk::ResponseHelper::DISCOVER_ACTION_REQUEST_OVERLOAD_STRING;
                break;
            case ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR:
                os << awsiotsdk::ResponseHelper::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR_STRING;
                break;
            case ResponseCode::JOBS_INVALID_TOPIC_ERROR:
                os << awsiotsdk::ResponseHelper::JOBS_INVALID_TOPIC_ERROR_STRING;
                break;
        }
        os << " : SDK Code " << static_cast<int>(rc) << ".";
        return os;
    }

    util::String ResponseHelper::ToString(ResponseCode rc) {
        util::OStringStream responseText;
        responseText << rc;
        return responseText.str();
    }
}

