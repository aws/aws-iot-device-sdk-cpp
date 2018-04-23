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
 * @file ResponseCode.hpp
 * @brief Strongly typed enumeration of return values from functions within the SDK.
 *
 * Contains the return codes used by the SDK and helper functions to convert the ResponseCode into human readable strings
 */

#include "util/memory/stl/String.hpp"

#pragma once

// Used to avoid warnings in case of unused parameters in function pointers
#define IOT_UNUSED(x) (void)(x)

namespace awsiotsdk {
    /**
     * @brief Response Code enum class
     *
     * Strongly typed enumeration of return values from functions within the SDK.
     * Values less than -1 are specific error codes
     * Value of -1 is a generic failure response
     * Value of 0 is a generic success response
     * Values greater than 0 are specific non-error return codes
     * Values have been grouped based on source or type of code
     */
    enum class ResponseCode {
        // Discovery Success Codes

        DISCOVER_ACTION_NO_INFORMATION_PRESENT = 401,           ///< Discover Action response showed no discovery information is present for this thing name
        DISCOVER_ACTION_SUCCESS = 400,                          ///< Discover Action found connectivity information for this thing name

        // Shadow Success Codes

        SHADOW_RECEIVED_DELTA = 301,                             ///< Returned when a delta update is received
        SHADOW_REQUEST_ACCEPTED = 300,                           ///< Returned when the request has been accepted

        // Network Success Codes

        NETWORK_PHYSICAL_LAYER_CONNECTED = 203,                   ///< Returned when the Network physical layer is connected.
        NETWORK_MANUALLY_DISCONNECTED = 202,                      ///< Returned when the Network is manually disconnected.
        NETWORK_ATTEMPTING_RECONNECT = 201,                       ///< Returned when the Network is disconnected and the reconnect attempt is in progress.
        NETWORK_RECONNECTED = 200,                                ///< Return value of yield function to indicate auto-reconnect was successful.

        // MQTT Success Codes

        MQTT_NOTHING_TO_READ = 101,                               ///< Returned when a read attempt is made on the TLS buffer and it is empty.
        MQTT_CONNACK_CONNECTION_ACCEPTED = 100,                   ///< Returned when a connection request is successful and packet response is connection accepted.

        // Generic Response Codes

        SUCCESS = 0,                                              ///< Success return value - no error occurred.
        FAILURE = -1,                                             ///< A generic error. Not enough information for a specific error code.
        NULL_VALUE_ERROR = -2,                                    ///< A required parameter was passed as null.

        // I/O Error Codes

        FILE_OPEN_ERROR = -100,                                    ///< Unable to open the requested file
        FILE_NAME_INVALID = -101,                                  ///< File name is invalid or of zero length

        // Threading Error Codes

        MUTEX_INIT_ERROR = -200,                                   ///< Mutex initialization failed
        MUTEX_LOCK_ERROR = -201,                                   ///< Mutex lock request failed
        MUTEX_UNLOCK_ERROR = -202,                                 ///< Mutex unlock request failed
        MUTEX_DESTROY_ERROR = -203,                                ///< Mutex destroy failed
        THREAD_EXITING = -204,                                     ///< Thread is exiting, returned when thread exits in the middle of an operation

        // Network TCP Error Codes

        NETWORK_TCP_CONNECT_ERROR = -300,                          ///< The TCP socket could not be established.
        NETWORK_TCP_SETUP_ERROR = -301,                            ///< Error associated with setting up the parameters of a Socket.
        NETWORK_TCP_UNKNOWN_HOST = -302,                           ///< Returned when the server is unknown.
        NETWORK_TCP_NO_ENDPOINT_SPECIFIED = -303,                  ///< Returned when the Network connection was not provided an endpoint

        // Network SSL Error Codes

        NETWORK_SSL_INIT_ERROR = -400,                             ///< SSL initialization error at the TLS layer.
        NETWORK_SSL_ROOT_CRT_PARSE_ERROR = -401,                   ///< Returned when the root certificate is invalid.
        NETWORK_SSL_DEVICE_CRT_PARSE_ERROR = -402,                 ///< Returned when the device certificate is invalid.
        NETWORK_SSL_KEY_PARSE_ERROR = -403,                        ///< An error occurred when loading the certificates. The certificates could not be located or are incorrectly formatted.
        NETWORK_SSL_TLS_HANDSHAKE_ERROR = -404,                    ///< The TLS handshake failed due to unknown error.
        NETWORK_SSL_CONNECT_ERROR = -405,                          ///< An unknown occurred while waiting for the TLS handshake to complete.
        NETWORK_SSL_CONNECT_TIMEOUT_ERROR = -406,                  ///< A timeout occurred while waiting for the TLS handshake to complete.
        NETWORK_SSL_CONNECTION_CLOSED_ERROR = -407,                ///< The SSL Connection was closed
        NETWORK_SSL_WRITE_ERROR = -408,                            ///< A Generic write error based on the platform used.
        NETWORK_SSL_WRITE_TIMEOUT_ERROR = -409,                    ///< SSL Write times out.
        NETWORK_SSL_READ_ERROR = -410,                             ///< A Generic error based on the platform used.nerator seeding failed.
        NETWORK_SSL_READ_TIMEOUT_ERROR = -411,                     ///< SSL Read times out.
        NETWORK_SSL_NOTHING_TO_READ = -412,                        ///< Returned when there is nothing to read in the TLS read buffer.
        NETWORK_SSL_UNKNOWN_ERROR = -413,                          ///< A generic error code for Network SSL layer errors.
        NETWORK_SSL_SERVER_VERIFICATION_ERROR = -414,              ///< Server name verification failure.

        // Network Generic Error Codes

        NETWORK_DISCONNECTED_ERROR = -500,                         ///< Returned when the Network is disconnected and reconnect is either disabled or physical layer is disconnected.
        NETWORK_RECONNECT_TIMED_OUT_ERROR = -501,                  ///< Returned when the Network is disconnected and the reconnect attempt has timed out.
        NETWORK_ALREADY_CONNECTED_ERROR = -502,                    ///< Returned when the Network is already connected and a connection attempt is made.
        NETWORK_PHYSICAL_LAYER_DISCONNECTED = -503,                ///< Returned when the physical layer is disconnected.
        NETWORK_NOTHING_TO_WRITE_ERROR = -504,                     ///< Returned when the Network write function is passed an empty buffer as argument

        // ClientCore Error Codes

        ACTION_NOT_REGISTERED_ERROR = -601,                        ///< Requested action is not registered with the core client
        ACTION_QUEUE_FULL = -602,                                  ///< Core Client Action queue is full
        ACTION_CREATE_FAILED = -603,                               ///< Core Client was not able to create the requested action

        // MQTT Error Codes

        MQTT_CONNECTION_ERROR = -701,                              ///< A connection could not be established.
        MQTT_CONNECT_TIMEOUT_ERROR = -702,                         ///< A timeout occurred while waiting for the MQTT connect to complete.
        MQTT_REQUEST_TIMEOUT_ERROR = -703,                         ///< A timeout occurred while waiting for the TLS request to complete.
        MQTT_UNEXPECTED_CLIENT_STATE_ERROR = -704,                 ///< The current client state does not match the expected value.
        MQTT_CLIENT_NOT_IDLE_ERROR = -705,                         ///< The client state is not idle when request is being made.
        MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR = -706,          ///< The MQTT RX buffer received corrupt or unexpected message.
        MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR = -707,               ///< The client is subscribed to the maximum possible number of subscriptions.
        MQTT_DECODE_REMAINING_LENGTH_ERROR = -708,                 ///< Failed to decode the remaining packet length on incoming packet.
        MQTT_CONNACK_UNKNOWN_ERROR = -709,                         ///< Connect request failed with the server returning an unknown error.
        MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR = -710,   ///< Connect request failed with the server returning an unacceptable protocol version error.
        MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR = -711,             ///< Connect request failed with the server returning an identifier rejected error.
        MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR = -712,              ///< Connect request failed with the server returning an unavailable error.
        MQTT_CONNACK_BAD_USERDATA_ERROR = -713,                    ///< Connect request failed with the server returning a bad userdata error.
        MQTT_CONNACK_NOT_AUTHORIZED_ERROR = -714,                  ///< Connect request failed with the server failing to authenticate the request.
        MQTT_NO_SUBSCRIPTION_FOUND = -715,                         ///< No subscription exists for requested topic
        MQTT_SUBSCRIPTION_NOT_ACTIVE = -716,                       ///< Subscription exists but is not active, waiting for Suback or Ack not received
        MQTT_UNEXPECTED_PACKET_FORMAT_ERROR = -717,                ///< Deserialization failed because packet data was in an unexpected format
        MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST = -718,             ///< Too many subscriptions were provided in the Subscribe/Unsubscribe request
        MQTT_INVALID_DATA_ERROR = -719,                            ///< Provided data is invalid/not sufficient for the request
        MQTT_SUBSCRIBE_PARTIALLY_FAILED = -720,                    ///< Failed to subscribe to atleast one of the topics in the subscribe request
        MQTT_SUBSCRIBE_FAILED = -721,                              ///< Unable to subscribe to any of the topics in the subscribe request

        // JSON Parsing Error Codes

        JSON_PARSE_KEY_NOT_FOUND_ERROR = -800,                     ///< JSON Parser was not able to find the requested key in the specified JSON
        JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR = -801,               ///< The value type was different from the expected type
        JSON_PARSING_ERROR = -802,                                 ///< An error occurred while parsing the JSON string.  Usually malformed JSON.
        JSON_MERGE_FAILED = -803,                                  ///< Returned when the JSON merge request fails unexpectedly
        JSON_DIFF_FAILED = -804,                                   ///< Returned when the JSON diff request fails unexpectedly

        // Shadow Error Codes

        SHADOW_WAIT_FOR_PUBLISH = -900,                            ///< Shadow: The response Ack table is currently full waiting for previously published updates
        SHADOW_JSON_BUFFER_TRUNCATED = -901,                       ///< Any time an snprintf writes more than size value, this error will be returned
        SHADOW_JSON_ERROR = -902,                                  ///< Any time an snprintf encounters an encoding error or not enough space in the given buffer
        SHADOW_JSON_EMPTY_ERROR = -903,                            ///< Returned when the provided json document is empty
        SHADOW_REQUEST_MAP_EMPTY = -904,                           ///< Returned when the provided request map is empty
        SHADOW_MQTT_DISCONNECTED_ERROR = -905,                     ///< Returned when the MQTT connection is not active
        SHADOW_UNEXPECTED_RESPONSE_TYPE = -906,                    ///< Returned when the Response type in the recevied payload is unexpected
        SHADOW_UNEXPECTED_RESPONSE_TOPIC = -907,                   ///< Returned when Response is received on an unexpected topic
        SHADOW_REQUEST_REJECTED = -908,                            ///< Returned when the request has been rejected by the server
        SHADOW_MQTT_CLIENT_NOT_SET_ERROR = -909,                   ///< Returned when there is no client set for this shadow
        SHADOW_NOTHING_TO_UPDATE = -910,                           ///< Returned when there is nothing to update for a Shadow Update request
        SHADOW_UNEXPECTED_RESPONSE_PAYLOAD = -911,                 ///< Returned when the response payload is in an unexpected format
        SHADOW_RECEIVED_OLD_VERSION_UPDATE = -912,                 ///< Returned when a version update is received with an older version than the current one on the device

        // WebSocket Error Codes

        WEBSOCKET_SIGN_URL_NO_MEM = -1000,                         ///< Internal buffer overflow when signing secured WebSocket url
        WEBSOCKET_GEN_CLIENT_KEY_ERROR = -1001,                    ///< Error in generating WebSocket handhshake client key
        WEBSOCKET_HANDSHAKE_ERROR = -1002,                         ///< WebSocket handshake generic error
        WEBSOCKET_HANDSHAKE_WRITE = -1003,                         ///< WebSocket handshake error in sending request
        WEBSOCKET_HANDSHAKE_READ = -1004,                          ///< WebSocket handhshake error in receiving request
        WEBSOCKET_HANDSHAKE_VERIFY_ERROR = -1005,                  ///< WebSocket handshake error in verifying server response
        WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR = -1006,                ///< WebSocket wslay context init error
        WEBSOCKET_FRAME_RECEIVE_ERROR = -1007,                     ///< WebSocket error in receiving frames
        WEBSOCKET_FRAME_TRANSMIT_ERROR = -1008,                    ///< WebSocket error in sending frames
        WEBSOCKET_PROTOCOL_VIOLATION = -1009,                      ///< WebSocket protocol violation detected in receiving frames
        WEBSOCKET_MAX_LIFETIME_REACHED = -1010,                    ///< WebSocket connection max life time window reached
        WEBSOCKET_DISCONNECT_ERROR = -1011,                        ///< WebSocket disconnect error
        WEBSOCKET_GET_UTC_TIME_FAILED = -1012,                     ///< Returned when the WebSocket wrapper cannot get UTC time

        // Discovery Error Codes

        DISCOVER_ACTION_REQUEST_FAILED_ERROR = -1100,              ///< Discover Action request failed
        DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR = -1101,           ///< Discover Action request timed out
        DISCOVER_ACTION_UNAUTHORIZED = -1102,                      ///< Discover Action repsonse showed that this device does not have authorization to query the server
        DISCOVER_ACTION_SERVER_ERROR = -1103,                      ///< Discover Action failed due to some server side error
        DISCOVER_ACTION_REQUEST_OVERLOAD = -1104,                  ///< Discover Action failed due to too many requests, try again after some time

        // Discovery Response Parsing Error Codes

        DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR = -1200,  ///< Discover Response Json is missing expected keys

        // Jobs Error Codes

        JOBS_INVALID_TOPIC_ERROR = -1300                           ///< Jobs invalid topic
    };

    /**
         * Overloading the << stream operator for ResponseCode
         *
         * @param os ostream being filled
         * @param rc ResponseCode
         * @return ostream&
         */
    std::ostream &operator<<(std::ostream &os, ResponseCode rc);

    /**
     * @brief Response Helper for converting ResponseCode into Strings
     */
    namespace ResponseHelper {
        const util::String DISCOVER_ACTION_NO_INFORMATION_PRESENT_STRING("No information found for device");
        const util::String DISCOVER_ACTION_SUCCESS_STRING("Discover action successful");
        const util::String SHADOW_RECEIVED_DELTA_STRING("Received the shadow delta");
        const util::String SHADOW_REQUEST_ACCEPTED_STRING("Shadow request accepted");
        const util::String NETWORK_PHYSICAL_LAYER_CONNECTED_STRING("Physical network layer connected");
        const util::String NETWORK_MANUALLY_DISCONNECTED_STRING("Network manually disconnected");
        const util::String NETWORK_ATTEMPTING_RECONNECT_STRING("Attempting to reconnect to the network");
        const util::String NETWORK_RECONNECTED_STRING("Network reconnected");
        const util::String MQTT_NOTHING_TO_READ_STRING("No MQTT packets received");
        const util::String MQTT_CONNACK_CONNECTION_ACCEPTED_STRING("Successfully connected to MQTT server");
        const util::String SUCCESS_STRING("Success");
        const util::String FAILURE_STRING("Failure");
        const util::String NULL_VALUE_ERROR_STRING("One or more parameters were null");
        const util::String FILE_OPEN_ERROR_STRING("Error occurred while trying to open the file");
        const util::String FILE_NAME_INVALID_STRING("File name provided is invalid or of zero length");
        const util::String MUTEX_INIT_ERROR_STRING("Error occurred while initializing the mutex");
        const util::String MUTEX_LOCK_ERROR_STRING("Error occurred while locking the mutex");
        const util::String MUTEX_UNLOCK_ERROR_STRING("Error occurred while unlocking the mutex");
        const util::String MUTEX_DESTROY_ERROR_STRING("Error occurred while destroying the mutex");
        const util::String THREAD_EXITING_STRING("Thread is exiting");
        const util::String NETWORK_TCP_CONNECT_ERROR_STRING("TCP Error occurred while opening a socket");
        const util::String NETWORK_TCP_SETUP_ERROR_STRING("Error occurred while setting up the TCP socket");
        const util::String NETWORK_TCP_UNKNOWN_HOST_STRING("Unable to find host specified");
        const util::String NETWORK_TCP_NO_ENDPOINT_SPECIFIED_STRING("No endpoint specified");
        const util::String NETWORK_SSL_INIT_ERROR_STRING("Error occurred while initializing SSL");
        const util::String NETWORK_SSL_ROOT_CRT_PARSE_ERROR_STRING("Error occurred while parsing the root CRT");
        const util::String NETWORK_SSL_DEVICE_CRT_PARSE_ERROR_STRING("Error occurred while parsing the device CRT");
        const util::String NETWORK_SSL_KEY_PARSE_ERROR_STRING("Error occurred while parsing the private key");
        const util::String NETWORK_SSL_TLS_HANDSHAKE_ERROR_STRING("Error occurred while performing the TLS handshake");
        const util::String NETWORK_SSL_CONNECT_ERROR_STRING("Error occurred during the connect attempt");
        const util::String NETWORK_SSL_CONNECT_TIMEOUT_ERROR_STRING("The connect attempt timed out");
        const util::String NETWORK_SSL_CONNECTION_CLOSED_ERROR_STRING("The SSL connection was closed");
        const util::String NETWORK_SSL_WRITE_ERROR_STRING("Error occurred during the SSL write operation");
        const util::String NETWORK_SSL_WRITE_TIMEOUT_ERROR_STRING("The SSL write operation timed out");
        const util::String NETWORK_SSL_READ_ERROR_STRING("Error occurred during the SSL read operation");
        const util::String NETWORK_SSL_READ_TIMEOUT_ERROR_STRING("The SSL read operation timed out");
        const util::String NETWORK_SSL_NOTHING_TO_READ_STRING("No SSL packets received");
        const util::String NETWORK_SSL_UNKNOWN_ERROR_STRING("Unknown error occurred during an SSL operation");
        const util::String NETWORK_SSL_SERVER_VERIFICATION_ERROR_STRING("Unable to verify server name");
        const util::String NETWORK_DISCONNECTED_ERROR_STRING("Network is disconnected");
        const util::String NETWORK_RECONNECT_TIMED_OUT_ERROR_STRING("Reconnect operation time out");
        const util::String NETWORK_ALREADY_CONNECTED_ERROR_STRING("Network is already connected");
        const util::String NETWORK_PHYSICAL_LAYER_DISCONNECTED_STRING("Physical network layer is disconnected");
        const util::String NETWORK_NOTHING_TO_WRITE_ERROR_STRING("No packets to write to the network");
        const util::String ACTION_NOT_REGISTERED_ERROR_STRING("The action attempted is not registered with the client");
        const util::String ACTION_QUEUE_FULL_STRING("The client action queue is full");
        const util::String ACTION_CREATE_FAILED_STRING("The client was unable to create the action");
        const util::String MQTT_CONNECTION_ERROR_STRING("Unable to establish the MQTT connection");
        const util::String MQTT_CONNECT_TIMEOUT_ERROR_STRING("The MQTT connect operation timed out");
        const util::String MQTT_REQUEST_TIMEOUT_ERROR_STRING("The MQTT request timed out");
        const util::String MQTT_UNEXPECTED_CLIENT_STATE_ERROR_STRING("The MQTT client is in an unexpected state");
        const util::String MQTT_CLIENT_NOT_IDLE_ERROR_STRING("The MQTT client is not idle");
        const util::String MQTT_RX_MESSAGE_PACKET_TYPE_INVALID_ERROR_STRING("The MQTT message is of an invalid type");
        const util::String MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR_STRING("Reached maximum MQTT subscriptions");
        const util::String MQTT_DECODE_REMAINING_LENGTH_ERROR_STRING("Error occurred while decoding the remaining length of the MQTT message");
        const util::String MQTT_CONNACK_UNKNOWN_ERROR_STRING("MQTT connect request failed with server returning an unknown error");
        const util::String MQTT_CONNACK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR_STRING("MQTT connect request failed with server returning an unacceptable protocol error");
        const util::String MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR_STRING("MQTT connect request failed with server returning an identifier rejected error");
        const util::String MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR_STRING("MQTT connect request failed with server returning an unavailable error");
        const util::String MQTT_CONNACK_BAD_USERDATA_ERROR_STRING("MQTT connect request failed with server returning a bad userdata error");
        const util::String MQTT_CONNACK_NOT_AUTHORIZED_ERROR_STRING("MQTT connect request failed with server returning a  not authorized error");
        const util::String MQTT_NO_SUBSCRIPTION_FOUND_STRING("No MQTT subscriptions were found for the requested topic");
        const util::String MQTT_SUBSCRIPTION_NOT_ACTIVE_STRING("The MQTT subscription specified is not active");
        const util::String MQTT_UNEXPECTED_PACKET_FORMAT_ERROR_STRING("Unable to serialize the MQTT packet as the format is unexpected");
        const util::String MQTT_TOO_MANY_SUBSCRIPTIONS_IN_REQUEST_STRING("Too many subscriptions were provided in the MQTT subscribe/unsubscribe request");
        const util::String MQTT_INVALID_DATA_ERROR_STRING("Invalid/Insufficient data was provided in the MQTT request");
        const util::String MQTT_SUBSCRIBE_PARTIALLY_FAILED_STRING("Failed to subscribe to atleast one of the topics in the subscribe request");
        const util::String MQTT_SUBSCRIBE_FAILED_STRING("Failed to subscribe to any of the topics in the subscribe request");
        const util::String JSON_PARSE_KEY_NOT_FOUND_ERROR_STRING("Unable to find the requested key in the JSON");
        const util::String JSON_PARSE_KEY_UNEXPECTED_TYPE_ERROR_STRING("The value for the JSON key was of an unexpected type");
        const util::String JSON_PARSING_ERROR_STRING("Error occurred while parsing the JSON");
        const util::String JSON_MERGE_FAILED_STRING("Failed to merge the JSON");
        const util::String JSON_DIFF_FAILED_STRING("Failed to diff the JSON");
        const util::String SHADOW_WAIT_FOR_PUBLISH_STRING("Waiting for previously published shadow updates");
        const util::String SHADOW_JSON_BUFFER_TRUNCATED_STRING("Shadow JSON is truncated as size specified is less than the size of the JSON");
        const util::String SHADOW_JSON_ERROR_STRING("Encoding error occurred while printing the shadow JSON");
        const util::String SHADOW_JSON_EMPTY_ERROR_STRING("The shadow JSON is empty");
        const util::String SHADOW_REQUEST_MAP_EMPTY_STRING("The shadow request map is empty ");
        const util::String SHADOW_MQTT_DISCONNECTED_ERROR_STRING("The shadow's MQTT connection is inactive");
        const util::String SHADOW_UNEXPECTED_RESPONSE_TYPE_STRING("The shadow response received is of an unexpected type");
        const util::String SHADOW_UNEXPECTED_RESPONSE_TOPIC_STRING("The shadow response was received on an unexpected topic");
        const util::String SHADOW_REQUEST_REJECTED_STRING("The shadow request was rejected by the server");
        const util::String SHADOW_MQTT_CLIENT_NOT_SET_ERROR_STRING("There is no client set for this shadow");
        const util::String SHADOW_NOTHING_TO_UPDATE_STRING("There are no shadow updates to be performed");
        const util::String SHADOW_UNEXPECTED_RESPONSE_PAYLOAD_STRING("The shadow response is in an unexpected format");
        const util::String SHADOW_RECEIVED_OLD_VERSION_UPDATE_STRING("The received shadow version is older than the current one on the device");
        const util::String WEBSOCKET_SIGN_URL_NO_MEM_STRING("Internal buffer overflowed while signing WebSocket URL");
        const util::String WEBSOCKET_GEN_CLIENT_KEY_ERROR_STRING("Error occurred while generating WebSocket handshake client key");
        const util::String WEBSOCKET_HANDSHAKE_ERROR_STRING("Unable to complete WebSocket handshake");
        const util::String WEBSOCKET_HANDSHAKE_WRITE_STRING("Unable to transmit WebSocket handshake request");
        const util::String WEBSOCKET_HANDSHAKE_READ_STRING("Unable to receive WebSocket handshake request");
        const util::String WEBSOCKET_HANDSHAKE_VERIFY_ERROR_STRING("Unable to verify handshake response from the server");
        const util::String WEBSOCKET_WSLAY_CONTEXT_INIT_ERROR_STRING("Erro occurred while initializing the WebSocket WSLay context");
        const util::String WEBSOCKET_FRAME_RECEIVE_ERROR_STRING("Error occurred while receiving WebSocket frame");
        const util::String WEBSOCKET_FRAME_TRANSMIT_ERROR_STRING("Error occurred while transmitting WebSocket frame");
        const util::String WEBSOCKET_PROTOCOL_VIOLATION_STRING("Protocol violation was detected in the received WebSocket frames");
        const util::String WEBSOCKET_MAX_LIFETIME_REACHED_STRING("Max lifetime of the WebSocket connection was reached");
        const util::String WEBSOCKET_DISCONNECT_ERROR_STRING("Error occurred while disconnecting the WebSocket");
        const util::String WEBSOCKET_GET_UTC_TIME_FAILED_STRING("WebSocket wrapper is unable to get the UTC ");
        const util::String DISCOVER_ACTION_REQUEST_FAILED_ERROR_STRING("Unable to perform the discover action");
        const util::String DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR_STRING("The discover action request timed out");
        const util::String DISCOVER_ACTION_UNAUTHORIZED_STRING("The device was unauthorized to perform the discovery action");
        const util::String DISCOVER_ACTION_SERVER_ERROR_STRING("Server returned unknown error while performing the discovery action");
        const util::String DISCOVER_ACTION_REQUEST_OVERLOAD_STRING("The discovery action is overloading the server, try again after some time");
        const util::String DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR_STRING("The discover response JSON is incomplete ");
        const util::String JOBS_INVALID_TOPIC_ERROR_STRING("Invalid jobs topic");
        
        /**
         * Takes in a Response Code and returns the appropriate error/success string
         * @param rc Response Code to be converted
         * @return char* Response String
         */
        util::String ToString(ResponseCode rc);
        
    }
}
