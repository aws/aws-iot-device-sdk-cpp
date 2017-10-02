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
 * @file Discovery.cpp
 * @brief
 *
 */

#include "util/logging/LogMacros.hpp"

#include "discovery/Discovery.hpp"

#define DISCOVER_ACTION_DESCRIPTION "AWS Greengrass Discover Action"

#define DISCOVER_LOG_TAG "[Discover]"

#define DISCOVER_ACTION_REQUEST_TYPE_PREFIX "GET "
#define DISCOVER_ACTION_EXPECTED_SUCCESS_RESPONSE "HTTP/1.1 200 OK"
#define DISCOVER_ACTION_FAIL_INFO_NOT_PRESENT "HTTP/1.1 404"
#define DISCOVER_ACTION_FAIL_UNAUTHORIZED "HTTP/1.1 401"
#define DISCOVER_ACTION_FAIL_TOO_MANY_REQUESTS "HTTP/1.1 429"
#define HTTP_CONTENT_LENGTH_STRING "content-length: "

#define DISCOVER_PACKET_PAYLOAD_PREFIX "/greengrass/discover/thing/"
#define DISCOVER_PACKET_PAYLOAD_SUFFIX " HTTP/1.1\r\n\r\n"

namespace awsiotsdk {
    namespace discovery {
        /****************************************************
         * DiscoverRequestData class function definitions *
         ***************************************************/
        DiscoverRequestData::DiscoverRequestData(std::unique_ptr<Utf8String> p_thing_name,
                                                 std::chrono::milliseconds max_response_wait_time) {
            discovery_request_data_ = DISCOVER_PACKET_PAYLOAD_PREFIX;
            discovery_request_data_.append(p_thing_name->ToStdString());
            discovery_request_data_.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
            max_response_wait_time_ = max_response_wait_time;
        }

        std::shared_ptr<DiscoverRequestData> DiscoverRequestData::Create(std::unique_ptr<Utf8String> p_thing_name,
                                                                         std::chrono::milliseconds max_response_wait_time) {
            return std::make_shared<DiscoverRequestData>(std::move(p_thing_name), max_response_wait_time);
        }

        util::String DiscoverRequestData::ToString() {
            return discovery_request_data_;
        }

        /*********************************************
         * DiscoverAction class function definitions *
         ********************************************/
        DiscoverAction::DiscoverAction(std::shared_ptr<mqtt::ClientState> p_client_state)
            : Action(ActionType::GREENGRASS_DISCOVER, DISCOVER_ACTION_DESCRIPTION) {
            p_client_state_ = p_client_state;
        }

        std::unique_ptr<Action> DiscoverAction::Create(std::shared_ptr<ActionState> p_action_state) {
            std::shared_ptr<mqtt::ClientState>
                p_client_state = std::dynamic_pointer_cast<mqtt::ClientState>(p_action_state);
            if (nullptr == p_client_state) {
                return nullptr;
            }

            return std::unique_ptr<DiscoverAction>(new DiscoverAction(p_client_state));
        }

        ResponseCode DiscoverAction::ReadResponseFromNetwork(std::shared_ptr<NetworkConnection> p_network_connection,
                                                             util::String &sent_packet,
                                                             util::String &read_payload,
                                                             std::chrono::milliseconds max_response_wait_time) {
            ResponseCode rc;

            util::Vector<unsigned char> temp_buf;
            util::Vector<unsigned char> read_buf;

            bool received_crlf = false;
            size_t read_bytes = 0;

            auto max_wait = std::chrono::steady_clock::now() + max_response_wait_time;
            do {
                rc = ReadFromNetworkBuffer(p_network_connection, temp_buf, 1);

                if (ResponseCode::SUCCESS == rc) {
                    read_bytes++;
                    read_buf.push_back(temp_buf[0]);

                    if ((read_bytes > 1) && read_buf[read_bytes - 1] == '\n' && read_buf[read_bytes - 2] == '\r') {
                        received_crlf = true;
                    }
                }

                if (std::chrono::steady_clock::now() > max_wait) {
                    rc = ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR;
                    break;
                }
            } while (!received_crlf);

            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            received_crlf = false;
            read_bytes = 0;
            // Parse header
            util::String response_header(read_buf.begin(), read_buf.end());
            if (util::String::npos == response_header.find(DISCOVER_ACTION_EXPECTED_SUCCESS_RESPONSE)) {
                if (util::String::npos != response_header.find(DISCOVER_ACTION_FAIL_INFO_NOT_PRESENT)) {
                    return ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT;
                } else if (util::String::npos != response_header.find(DISCOVER_ACTION_FAIL_TOO_MANY_REQUESTS)) {
                    return ResponseCode::DISCOVER_ACTION_REQUEST_OVERLOAD;
                } else if (util::String::npos != response_header.find(DISCOVER_ACTION_FAIL_UNAUTHORIZED)) {
                    return ResponseCode::DISCOVER_ACTION_UNAUTHORIZED;
                } else {
                    AWS_LOG_ERROR(DISCOVER_LOG_TAG, "Discover Action HTTP request failed with a Response Header : \n%s",
                                  response_header.c_str());
                    return ResponseCode::DISCOVER_ACTION_SERVER_ERROR;
                }
            }

            util::String content_length_string(HTTP_CONTENT_LENGTH_STRING);
            size_t content_length_index = std::string::npos;
            read_buf.clear();
            do {
                rc = ReadFromNetworkBuffer(p_network_connection, temp_buf, 1);
                if (ResponseCode::SUCCESS != rc) {
                    break;
                }

                read_bytes++;
                read_buf.push_back(temp_buf[0]);

                if ((read_bytes > 1) && read_buf[read_bytes - 1] == '\n' && read_buf[read_bytes - 2] == '\r') {
                    util::String header_line(read_buf.begin(), read_buf.end());
                    content_length_index = header_line.find(content_length_string);
                    if (util::String::npos == content_length_index) {
                        read_buf.clear();
                        read_bytes = 0;
                        continue;
                    }
                    received_crlf = true;
                }

                if (std::chrono::steady_clock::now() > max_wait) {
                    rc = ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR;
                    break;
                }
            } while (!received_crlf);

            if (content_length_index == std::string::npos) {
                if (ResponseCode::SUCCESS != rc) {
                    return rc;
                }
                return ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT;
            }

            size_t content_length_begin_offset = content_length_index + content_length_string.length();
            size_t content_length_end_offset = content_length_begin_offset;
            while ('\r' != (char) read_buf[content_length_end_offset]) {
                content_length_end_offset++;
            }
            util::String content_length_str
                (read_buf.begin() + content_length_begin_offset, read_buf.begin() + content_length_end_offset);
            size_t content_length = std::stoul(content_length_str);

            // Skip the rest of the header
            received_crlf = false;
            do {
                rc = ReadFromNetworkBuffer(p_network_connection, temp_buf, 1);
                if (ResponseCode::SUCCESS != rc) {
                    break;
                }

                read_bytes++;
                read_buf.push_back(temp_buf[0]);

                if ((read_bytes > 1) && read_buf[read_bytes - 1] == '\n' && read_buf[read_bytes - 2] == '\r'
                    && read_buf[read_bytes - 3] == '\n' && read_buf[read_bytes - 4] == '\r') {
                    received_crlf = true;
                }

                if (std::chrono::steady_clock::now() > max_wait) {
                    rc = ResponseCode::DISCOVER_ACTION_REQUEST_TIMED_OUT_ERROR;
                    break;
                }
            } while (!received_crlf);

            read_buf.clear();

            if (0 < content_length) {
                rc = ReadFromNetworkBuffer(p_network_connection, read_buf, content_length);
                if (ResponseCode::SUCCESS == rc) {
                    read_payload.append(read_buf.begin(), read_buf.end());
                    size_t received_len = read_payload.length();
                    IOT_UNUSED(received_len);
                    read_buf.clear();
                }
            } else {
                rc = ResponseCode::DISCOVER_ACTION_REQUEST_FAILED_ERROR;
            }

            return rc;
        }

        ResponseCode DiscoverAction::MakeDiscoveryRequest(std::shared_ptr<NetworkConnection> p_network_connection,
                                                          const util::String packet_data) {
            util::String get_request(DISCOVER_ACTION_REQUEST_TYPE_PREFIX);
            get_request.append(packet_data);
            ResponseCode rc = WriteToNetworkBuffer(p_network_connection, get_request);
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(DISCOVER_LOG_TAG, "Discover Write to Network failed. %s",
                              ResponseHelper::ToString(rc).c_str());
            }
            return rc;
        }

        ResponseCode DiscoverAction::InitializeDiscoveryResponseJson(const util::String received_response,
                                                                     std::shared_ptr<DiscoverRequestData> discover_packet) {
            util::JsonDocument received_response_json;
            ResponseCode rc = util::JsonParser::InitializeFromJsonString(received_response_json, received_response);

            if (ResponseCode::SUCCESS == rc) {
                discover_packet->discovery_response_.SetResponseDocument(std::move(received_response_json));
                rc = ResponseCode::DISCOVER_ACTION_SUCCESS;
            }

            return rc;
        }

        ResponseCode DiscoverAction::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                   std::shared_ptr<ActionData> p_action_data) {
            std::shared_ptr<DiscoverRequestData>
                p_discover_packet = std::dynamic_pointer_cast<DiscoverRequestData>(p_action_data);
            util::String packet_data = p_discover_packet->ToString();

            if (nullptr == p_discover_packet) {
                return ResponseCode::NULL_VALUE_ERROR;
            }

            ResponseCode rc = p_network_connection->Connect();
            if (ResponseCode::SUCCESS != rc) {
                return rc;
            }

            rc = MakeDiscoveryRequest(p_network_connection, packet_data);
            if (ResponseCode::SUCCESS != rc) {
                p_network_connection->Disconnect();
                return rc;
            }

            util::String received_response;
            rc = ReadResponseFromNetwork(p_network_connection,
                                         packet_data,
                                         received_response,
                                         p_discover_packet->GetMaxResponseWaitTime());
            if (ResponseCode::SUCCESS != rc) {
                AWS_LOG_ERROR(DISCOVER_LOG_TAG, "Discover Read from Network failed. %s",
                              ResponseHelper::ToString(rc).c_str());
                p_network_connection->Disconnect();
                return rc;
            }

            rc = InitializeDiscoveryResponseJson(received_response, p_discover_packet);
            if (ResponseCode::DISCOVER_ACTION_SUCCESS != rc) {
                AWS_LOG_ERROR(DISCOVER_LOG_TAG, "Discover Read Parse failed. %s",
                              ResponseHelper::ToString(rc).c_str());
            }
            p_network_connection->Disconnect();
            return rc;
        }
    }
}
 
