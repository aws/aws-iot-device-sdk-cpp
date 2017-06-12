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
                os << "No information found for device";
                break;
            case ResponseCode::DISCOVER_ACTION_SUCCESS:
                os << "Discover action successful";
                break;
            case ResponseCode::SHADOW_RECEIVED_DELTA:
                os << "Received the shadow delta";
                break;
            case ResponseCode::SHADOW_REQUEST_ACCEPTED:
                os << "Shadow request accepted";
                break;
            case ResponseCode::NETWORK_PHYSICAL_LAYER_CONNECTED:
                os << "Physical network layer connected";
                break;
            case ResponseCode::NETWORK_MANUALLY_DISCONNECTED:
                os << "Network manually disconnected";
                break;
            case ResponseCode::NETWORK_ATTEMPTING_RECONNECT:
                os << "Attempting to reconnect to the network";
                break;
            case ResponseCode::NETWORK_RECONNECTED:
                os << "Network reconnected";
                break;
            case ResponseCode::MQTT_NOTHING_TO_READ:
                os << "No MQTT packets received";
                break;
            case ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED:
                os << "Successfully connected to MQTT server";
                break;
            case ResponseCode::SUCCESS:
                os << "Success";
                break;
            case ResponseCode::FAILURE:
                os << "Failure";
                break;
            case ResponseCode::NULL_VALUE_ERROR:
                os << "One or more parameters were null";
                break;
            case ResponseCode::FILE_OPEN_ERROR:
                os << "Error occurred while trying to open the file";
                break;
            case ResponseCode::FILE_NAME_INVALID:
                os << "File name provided is invalid or of zero length";
                break;
            case ResponseCode::MUTEX_INIT_ERROR:
                os << "Error occurred while initializing the mutex";
                break;
            case ResponseCode::MUTEX_LOCK_ERROR:
                os << "Error occurred while locking the mutex";
                break;
            case ResponseCode::MUTEX_UNLOCK_ERROR:
                os << "Error occurred while unlocking the mutex";
                break;
            case ResponseCode::MUTEX_DESTROY_ERROR:
                os << "Error occurred while destroying the mutex";
                break;
            case ResponseCode::THREAD_EXITING:
                os << "Thread is exiting";
                break;
            case ResponseCode::NETWORK_TCP_CONNECT_ERROR:
                os << "TCP Error occurred while opening a socket";
                break;
            case ResponseCode::NETWORK_TCP_SETUP_ERROR:
                os << "Error occurred while setting up the TCP socket";
                break;
            case ResponseCode::NETWORK_TCP_UNKNOWN_HOST:
                os << "Unable to find host specified";
                break;
            case ResponseCode::NETWORK_TCP_NO_ENDPOINT_SPECIFIED:
                os << "No endpoint specified";
                break;
            case ResponseCode::NETWORK_SSL_INIT_ERROR:
                os << "Error occurred while initializing SSL";
                break;
            case ResponseCode::NETWORK_SSL_ROOT_CRT_PARSE_ERROR:
                os << "Error occurred while parsing the root CRT";
                break;
            case ResponseCode::NETWORK_SSL_DEVICE_CRT_PARSE_ERROR:
                os << "Error occurred while parsing the device CRT";
                break;
            case ResponseCode::NETWORK_SSL_KEY_PARSE_ERROR:
                os << "Error occurred while parsing the private key";
                break;
            case ResponseCode::NETWORK_SSL_TLS_HANDSHAKE_ERROR:
                os << "Error occurred while performing the TLS handshake";
                break;
            case ResponseCode::NETWORK_SSL_CONNECT_ERROR:
                os << "Error occurred during the connect attempt";
                break;
            case ResponseCode::NETWORK_SSL_CONNECT_TIMEOUT_ERROR:
                os << "The connect attempt timed out";
                break;
            case ResponseCode::NETWORK_SSL_CONNECTION_CLOSED_ERROR:
                os << "The SSL connection was closed";
                break;
            case ResponseCode::NETWORK_SSL_WRITE_ERROR:
                os << "Error occurred during the SSL write operation";
                break;
            case ResponseCode::NETWORK_SSL_WRITE_TIMEOUT_ERROR:
                os << "The SSL write operation timed out";
                break;
            case ResponseCode::NETWORK_SSL_READ_ERROR:
                os << "Error occurred during the SSL read operation";
                break;
            case ResponseCode::NETWORK_SSL_READ_TIMEOUT_ERROR:
                os << "The SSL read operation timed out";
                break;
            case ResponseCode::NETWORK_SSL_NOTHING_TO_READ:
                os << "No SSL packets received";
                break;
            case ResponseCode::NETWORK_SSL_UNKNOWN_ERROR:
                os << "Unknown error occurred during an SSL operation";
                break;
            case ResponseCode::NETWORK_SSL_SERVER_VERIFICATION_ERROR:
                os << "Unable to verify server name";
                break;
            case ResponseCode::NETWORK_DISCONNECTED_ERROR:
                os << "Network is disconnected";
                break;
            case ResponseCode::NETWORK_RECONNECT_TIMED_OUT_ERROR:
                os << "Reconnect operation time out";
                break;
            case ResponseCode::NETWORK_ALREADY_CONNECTED_ERROR:
                os << "Network is already connected";
                break;
            case ResponseCode::NETWORK_PHYSICAL_LAYER_DISCONNECTED:
                os << "Physical network layer is disconnected";
                break;
            case ResponseCode::NETWORK_NOTHING_TO_WRITE_ERROR:
                os << "No packets to write to the network";
                break;
            case ResponseCode::ACTION_NOT_REGISTERED_ERROR:
                os << "The action attempted is not registered with the client";
                break;
            case ResponseCode::ACTION_QUEUE_FULL:
                os << "The client action queue is full";
                break;
            case ResponseCode::ACTION_CREATE_FAILED:
                os << "The client was unable to create the action";
                break;
            case ResponseCode::MQTT_CONNECTION_ERROR:
                os << "Unable to establish the MQTT connection";
                break;
            case ResponseCode::MQTT_CONNECT_TIMEOUT_ERROR:
                os << "The MQTT connect operation timed out";
                break;
            case ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR:
                os << "The MQTT request timed out";
                break;
            case ResponseCode::MQTT_UNEXPECTED_CLIENT_STATE_ERROR:
                os << "The MQTT client is in an unexpected state";
                break;
            case ResponseCode::MQTT_CLIENT_NOT_IDLE_ERROR:
                os << "The MQTT client is not idle";
                break;
            case ResponseCode::MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR:
                os << "The MQTT message is of an invalid type";
                break;
            case ResponseCode::MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR:
                os << "Reached maximum MQTT subscriptions";
                break;
            case ResponseCode::MQTT_DECODE_REMAINING_LENGTH_ERROR:
                os << "Error occurred while decoding the remaining length of the MQTT message";
                break;
            case ResponseCode::MQTT_CONNACK_UNKNOWN_ERROR:
                os << "MQTT connect request failed with server returning an unknown error";
                break;
            case ResponseCode::MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR:
                os << "MQTT connect request failed with server returning an unacceptable protocol error";
                break;
            case ResponseCode::MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR:
                os << "MQTT connect request failed with server returning an identifier rejected error";
                break;
            case ResponseCode::MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR:
                os << "MQTT connect request failed with server returning an unavailable error";
                break;
            case ResponseCode::MQTT_CONNACK_BAD_USERDATA_ERROR:
                os << "MQTT connect request failed with server returning a bad userdata error";
                break;
            case ResponseCode::MQTT_CONNACK_NOT_AUTHORIZED_ERROR:
                os << "MQTT connect request failed with server returning a  not authorized error";
                break;
            case ResponseCode::MQTT_NO_SUBSCRIPTION_FOUND:
                os << "No MQTT subscriptions were found for the requested topic";
                break;
            case ResponseCode::MQTT_SUBSCRIPTION_NOT_ACTIVE:
                os << "The MQTT subscription specified is not active";
                break;
            case ResponseCode::MQTT_UNEXPECTED_PACKET_FORMAT_ERROR:
                os << "Unable to serialize the MQTT packet as the format is unexpected";
                break;
            case ResponseCode::MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST:
                os << "Too many subscriptions were provided in the MQTT subscribe/unsubscribe request";
                break;
            case ResponseCode::MQTT_INVALID_DATA_ERROR:
                os << "Invalid/Insufficient data was provided in the MQTT request";
                break;
            case ResponseCode::MQTT_SUBSCRIBE_PARTIALLY_FAILED:
                os << "Failed to subscribe to atleast one of the topics in the subscribe request";
                break;
            case ResponseCode::MQTT_SUBSCRIBE_FAILED:
                os << "Failed to subscribe to any of the topics in the subscribe request";
                break;
            case ResponseCode::JSON_PARSE_KEY_NOT_FOUND_ERROR:
                os << "Unable to find the requested key in the JSON";
                break;
            case ResponseCode::JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR:
                os << "The value for the JSON key was of an unexpected type";
                break;
            case ResponseCode::JSON_PARSING_ERROR:
                os << "Error occurred while parsing the JSON";
                break;
            case ResponseCode::JSON_MERGE_FAILED:
                os << "Failed to merge the JSON";
                break;
            case ResponseCode::JSON_DIFF_FAILED:
                os << "Failed to diff the JSON";
                break;
            case ResponseCode::SHADOW_WAIT_FOR_PUBLISH:
                os << "Waiting for previously published shadow updates";
                break;
            case ResponseCode::SHADOW_JSON_BUFFER_TRUNCATED:
                os << "Shadow JSON is truncated as size specified is less than the size of the JSON";
                break;
            case ResponseCode::SHADOW_JSON_ERROR:
                os << "Encoding error occurred while printing the shadow JSON";
                break;
            case ResponseCode::SHADOW_JSON_EMPTY_ERROR :
                os << "The shadow JSON is empty";
                break;
            case ResponseCode::SHADOW_REQUEST_MAP_EMPTY:
                os << "The shadow request map is empty ";
                break;
            case ResponseCode::SHADOW_MQTT_DISCONNECTED_ERROR:
                os << "The shadow's MQTT connection is inactive";
                break;
            case ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TYPE:
                os << "The shadow response received is of an unexpected type";
                break;
            case ResponseCode::SHADOW_UNEXPECTED_RESPONSE_TOPIC:
                os << "The shadow response was received on an unexpected topic";
                break;
            case ResponseCode::SHADOW_REQUEST_REJECTED:
                os << "The shadow request was rejected by the server";
                break;
            case ResponseCode::SHADOW_MQTT_CLIENT_NOT_SET_ERROR:
                os << "There is no client set for this shadow";
                break;
            case ResponseCode::SHADOW_NOTHING_TO_UPDATE:
                os << "There are no shadow updates to be performed";
                break;
            case ResponseCode::SHADOW_UNEXPECTED_RESPONSE_PAYLOAD:
                os << "The shadow response is in an unexpected format";
                break;
            case ResponseCode::SHADOW_RECEIVED_OLD_VERSION_UPDATE:
                os << "The received shadow version is older than the current one on the device";
                break;
            case ResponseCode::WEBSOCKET_SIGN_URL_NO_MEM:
                os << "Internal buffer overflowed while signing WebSocket URL";
                break;
            case ResponseCode::WEBSOCKET_GEN_CLIENT_KEY_ERROR:
                os << "Error occurred while generating WebSocket handshake client key";
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_ERROR:
                os << "Unable to complete WebSocket handshake";
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_WRITE:
                os << "Unable to transmit WebSocket handshake request";
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_READ:
                os << "Unable to receive WebSocket handshake request";
                break;
            case ResponseCode::WEBSOCKET_HANDSHAKE_VERIFY_ERROR:
                os << "Unable to verify handshake response from the server";
                break;
            case ResponseCode::WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR:
                os << "Erro occurred while initializing the WebSocket WSLay context";
                break;
            case ResponseCode::WEBSOCKET_FRAME_RECEIVE_ERROR:
                os << "Error occurred while receiving WebSocket frame";
                break;
            case ResponseCode::WEBSOCKET_FRAME_TRANSMIT_ERROR:
                os << "Error occurred while transmitting WebSocket frame";
                break;
            case ResponseCode::WEBSOCKET_PROTOCOL_VIOLATION:
                os << "Protocol violation was detected in the received WebSocket frames";
                break;
            case ResponseCode::WEBSOCKET_MAX_LIFETIME_REACHED:
                os << "Max lifetime of the WebSocket connection was reached";
                break;
            case ResponseCode::WEBSOCKET_DISCONNECT_ERROR:
                os << "Error occurred while disconnecting the WebSocket";
                break;
            case ResponseCode::WEBSOCKET_GET_UTC_TIME_FAILED:
                os << "WebSocket wrapper is unable to get the UTC ";
                break;
            case ResponseCode::DISCOVER_ACTION_REQUEST_FAILED_ERROR:
                os << "Unable to perform the discover action";
                break;
            case ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR:
                os << "The discover action request timed out";
                break;
            case ResponseCode::DISCOVER_ACTION_UNAUTHORIZED:
                os << "The device was unauthorized to perform the discovery action";
                break;
            case ResponseCode::DISCOVER_ACTION_SERVER_ERROR:
                os << "Server returned unknown error while performing the discovery action";
                break;
            case ResponseCode::DISCOVER_ACTION_REQUEST_OVERLOAD:
                os << "The discovery action is overloading the server, try again after some time";
                break;
            case ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR:
                os << "The discover response JSON is incomplete ";
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

