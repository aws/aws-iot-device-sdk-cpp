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
 * @file ConnectTests.cpp
 * @brief
 *
 */

#include <chrono>

#include <gtest/gtest.h>

#include "util/logging/LogMacros.hpp"

#include "TestHelper.hpp"
#include "MockNetworkConnection.hpp"

#include "discovery/Discovery.hpp"
#include "mqtt/ClientState.hpp"
#include "ConfigCommon.hpp"
#include "mqtt/GreengrassMqttClient.hpp"

#define DISCOVERY_TEST_LOG_TAG "[Discovery Unit Test]"

#define DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS 5000

//"GET /things/CppSdkTestClient/gg/discover HTTP/1.1\r\n\r\n"
#define DISCOVER_ACTION_REQUEST_PREFIX "GET "
#define DISCOVER_PACKET_PAYLOAD_PREFIX "/greengrass/discover/thing/"
#define DISCOVER_PACKET_PAYLOAD_SUFFIX " HTTP/1.1\r\n\r\n"

#define DISCOVERY_SUCCESS_RESPONSE_HEADER_PREFIX "HTTP/1.1 200 OK\r\n"
#define DISCOVER_ACTION_FAIL_INFO_NOT_PRESENT "HTTP/1.1 404"
#define DISCOVER_ACTION_FAIL_UNAUTHORIZED "HTTP/1.1 401"
#define DISCOVER_ACTION_FAIL_TOO_MANY_REQUESTS "HTTP/1.1 429"
#define DISCOVERY_SUCCESS_RESPONSE_HEADER_SUFFIX "content-length: "

#define DISCOVERY_SUCCESS_RESPONSE_PAYLOAD "{"\
"\"GGGroups\": [{"\
    "\"GGGroupId\": \"TestGroupName1\","\
    "\"Cores\": [{"\
        "\"thingArn\": \"arn: aws: iot: us-west-2: 12345678901: thing\\/AnyThing_0\","\
        "\"Connectivity\": [{"\
            "\"Id\": \"<ID 1>\","\
            "\"HostAddress\": \"192.168.101.0\","\
            "\"PortNumber\": 8080,"\
            "\"Metadata\": \"<Description 1>\""\
            "}, {"\
            "\"Id\": \"<ID 2>\","\
            "\"HostAddress\": \"192.168.101.1\","\
            "\"PortNumber\": 8443,"\
            "\"Metadata\": \"<Description 2>\""\
        "}]"\
        "}, {"\
        "\"thingArn\": \"arn: aws: iot: us-west-2: 12345678901: thing\\/AnyThing_1\","\
        "\"Connectivity\": [{"\
            "\"Id\": \"<ID 3>\","\
            "\"HostAddress\": \"192.168.101.2\","\
            "\"PortNumber\": 8443,"\
            "\"Metadata\": \"<Description 3>\""\
            "}, {"\
            "\"Id\": \"<ID 4>\","\
            "\"HostAddress\": \"192.168.101.3\","\
            "\"PortNumber\": 8443,"\
            "\"Metadata\": \"<Description 4>\""\
        "}]"\
    "}],"\
    "\"CAs\": ["\
        "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\""\
        "]"\
    "}]"\
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_CA "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"," \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"Id\": \"<ID 1>\"," \
                            "\"HostAddress\": \"10.10.10.10\"," \
                            "\"PortNumber\": 8443," \
                            "\"Metadata\": \"metadata\"" \
                        "}" \
                    "]" \
                "}" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_GROUP_ID "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"," \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"Id\": \"<ID 1>\"," \
                            "\"HostAddress\": \"10.10.10.10\"," \
                            "\"PortNumber\": 8443," \
                            "\"Metadata\": \"metadata\"" \
                        "}" \
                    "]" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_CORES "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_GGC_THING_ARN "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"Id\": \"<ID 1>\"," \
                            "\"HostAddress\": \"10.10.10.10\"," \
                            "\"PortNumber\": 8443," \
                            "\"Metadata\": \"metadata\"" \
                        "}" \
                    "]" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_CONNECTIVITY_INFO_ARRAY "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_ID "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"," \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"HostAddress\": \"10.10.10.10\"," \
                            "\"PortNumber\": 8443," \
                            "\"Metadata\": \"metadata\"" \
                        "}" \
                    "]" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_HOST_ADDRESS "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"," \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"Id\": \"<ID 1>\"," \
                            "\"PortNumber\": 8443," \
                            "\"Metadata\": \"metadata\"" \
                        "}" \
                    "]" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_PORT "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"," \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"Id\": \"<ID 1>\"," \
                            "\"HostAddress\": \"10.10.10.10\"," \
                            "\"Metadata\": \"metadata\"" \
                        "}" \
                    "]" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

#define DISCOVERY_RESPONSE_PAYLOAD_NO_METADATA "{" \
    "\"GGGroups\": [" \
        "{" \
            "\"GGGroupId\": \"TestGroupName\"," \
            "\"Cores\": [" \
                "{" \
                    "\"thingArn\": \"arn:aws;iot:us-west-2:12345678901:thing/anything_0\"," \
                    "\"Connectivity\": [" \
                        "{" \
                            "\"Id\": \"<ID 1>\"," \
                            "\"HostAddress\": \"10.10.10.10\"," \
                            "\"PortNumber\": 8443" \
                        "}" \
                    "]" \
                "}" \
            "]," \
            "\"CAs\": [" \
                "\"-----BEGIN CERTIFICATE-----\\\\nsLongStringHere\\\\n-----END CERTIFICATE-----\\\\n\"" \
            "]" \
        "}" \
    "]" \
"}"

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class DiscoverActionTester : public ::testing::Test {
            protected:
                std::shared_ptr<mqtt::ClientState> p_core_state_;
                std::shared_ptr<tests::mocks::MockNetworkConnection> p_network_connection_;
                tests::mocks::MockNetworkConnection *p_network_mock_;

                static const util::String test_thing_name_;

                DiscoverActionTester() {
                    p_core_state_ = mqtt::ClientState::Create(std::chrono::milliseconds(200));
                    p_network_connection_ = std::make_shared<tests::mocks::MockNetworkConnection>();
                    p_network_mock_ = p_network_connection_.get();
                }
            };

            const util::String DiscoverActionTester::test_thing_name_ = "CppSdkTestClient";

            TEST_F(DiscoverActionTester, SetAndGetDummyActionIDTest) {
                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                    = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                             std::chrono::milliseconds(
                                                                 DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                p_discover_request_data->SetActionId(0);
                uint16_t action_id = p_discover_request_data->GetActionId();
                EXPECT_EQ(0, action_id);
            }

            TEST_F(DiscoverActionTester, GetMaxResponseWaitTimeTest) {
                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                    = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                             std::chrono::milliseconds(
                                                                 DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                std::chrono::milliseconds max_response_wait_time = p_discover_request_data->GetMaxResponseWaitTime();
                EXPECT_EQ((std::chrono::milliseconds) DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS,
                          max_response_wait_time);
            }

            TEST_F(DiscoverActionTester, NullClientStateTest) {
                std::unique_ptr<Action> discover_action = discovery::DiscoverAction::Create(nullptr);
                EXPECT_EQ(nullptr, discover_action);
            }

            TEST_F(DiscoverActionTester, TestConstructorAndDestructor) {
                util::JsonDocument response_document;
                ResponseCode rc =
                    util::JsonParser::InitializeFromJsonString(response_document, DISCOVERY_SUCCESS_RESPONSE_PAYLOAD);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                DiscoveryResponse response(std::move(response_document));
            }

            TEST_F(DiscoverActionTester, IncompleteDiscoveryResponseTest) {
                util::JsonDocument request_payload_1;
                util::String payload_1(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_CA);
                util::Vector<ConnectivityInfo> connectivity_info_list;
                util::Map<util::String, util::Vector<util::String>> root_ca_map;
                ResponseCode rc = util::JsonParser::InitializeFromJsonString(request_payload_1, payload_1);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_1));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_2(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_CONNECTIVITY_INFO_ARRAY);
                util::JsonDocument request_payload_2;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_2, payload_2);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_2));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_3(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_CORES);
                util::JsonDocument request_payload_3;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_3, payload_3);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_3));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_4(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_GGC_THING_ARN);
                util::JsonDocument request_payload_4;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_4, payload_2);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_4));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_5(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_GROUP_ID);
                util::JsonDocument request_payload_5;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_5, payload_5);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_5));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_6(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_HOST_ADDRESS);
                util::JsonDocument request_payload_6;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_6, payload_6);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_6));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_7(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_ID);
                util::JsonDocument request_payload_7;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_7, payload_7);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_7));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_8(BROKEN_DISCOVERY_RESPONSE_PAYLOAD_NO_PORT);
                util::JsonDocument request_payload_8;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_8, payload_8);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_8));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::DISCOVER_RESPONSE_UNEXPECTED_JSON_STRUCTURE_ERROR, rc);
                }

                util::String payload_9(DISCOVERY_RESPONSE_PAYLOAD_NO_METADATA);
                util::JsonDocument request_payload_9;
                rc = util::JsonParser::InitializeFromJsonString(request_payload_9, payload_9);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload_9));
                    rc = discovery_response.GetParsedResponse(connectivity_info_list, root_ca_map);

                    EXPECT_EQ(ResponseCode::SUCCESS, rc);
                }
            }

            TEST_F(DiscoverActionTester, TestOutputToFile) {
                util::String payload(DISCOVERY_SUCCESS_RESPONSE_PAYLOAD);
                util::JsonDocument request_payload;
                ResponseCode rc = util::JsonParser::InitializeFromJsonString(request_payload, payload);
                if (ResponseCode::SUCCESS == rc) {
                    DiscoveryResponse discovery_response;
                    discovery_response.SetResponseDocument(std::move(request_payload));

                    util::String current_working_directory = ConfigCommon::GetCurrentPath();
#ifdef WIN32
                    current_working_directory.append("\\");
#else
                    current_working_directory.append("/");
#endif
                    // Write complete Discovery Response JSON out to a file
                    util::String discovery_response_output_path = current_working_directory;
                    discovery_response_output_path.append("discovery_test_output.json");
                    rc = discovery_response.WriteToPath(discovery_response_output_path);

                    EXPECT_EQ(ResponseCode::SUCCESS, rc);
                }
            }

            TEST_F(DiscoverActionTester, ServerOverloadResponseTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                util::String next_read_buf(DISCOVER_ACTION_FAIL_TOO_MANY_REQUESTS);
                next_read_buf.append("\r\n");

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(next_read_buf);
                p_network_connection_->was_read_called_ = false;

                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                std::unique_ptr<Action> p_discover_action = discovery::DiscoverAction::Create(p_core_state_);
                std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                    = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                             std::chrono::milliseconds(
                                                                 DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_,
                            DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                util::String expected_write_request(DISCOVER_ACTION_REQUEST_PREFIX);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_PREFIX);
                expected_write_request.append(test_thing_name_);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                 ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                    1>(
                    expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                p_discover_action->SetParentThreadSync(thread_task_out_sync);

                ResponseCode rc = p_discover_action->PerformAction(p_network_connection_, p_discover_request_data);
                EXPECT_EQ(ResponseCode::DISCOVER_ACTION_REQUEST_OVERLOAD, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
            }

            TEST_F(DiscoverActionTester, ConnectivityInformationNotFoundTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                util::String next_read_buf(DISCOVER_ACTION_FAIL_INFO_NOT_PRESENT);
                next_read_buf.append("\r\n");

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(next_read_buf);
                p_network_connection_->was_read_called_ = false;

                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                std::unique_ptr<Action> p_discover_action = discovery::DiscoverAction::Create(p_core_state_);
                std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                    = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                             std::chrono::milliseconds(
                                                                 DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_,
                            DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                
                util::String expected_write_request(DISCOVER_ACTION_REQUEST_PREFIX);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_PREFIX);
                expected_write_request.append(test_thing_name_);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                 ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                    1>(
                    expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                p_discover_action->SetParentThreadSync(thread_task_out_sync);

                ResponseCode rc = p_discover_action->PerformAction(p_network_connection_, p_discover_request_data);
                EXPECT_EQ(ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
            }

            TEST_F(DiscoverActionTester, DiscoverActionUnauthorizedTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                util::String next_read_buf(DISCOVER_ACTION_FAIL_UNAUTHORIZED);
                next_read_buf.append("\r\n");

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(next_read_buf);
                p_network_connection_->was_read_called_ = false;

                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                std::unique_ptr<Action> p_discover_action = discovery::DiscoverAction::Create(p_core_state_);
                std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                    = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                             std::chrono::milliseconds(
                                                                 DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_,
                            DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                util::String expected_write_request(DISCOVER_ACTION_REQUEST_PREFIX);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_PREFIX);
                expected_write_request.append(test_thing_name_);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                 ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                    1>(
                    expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                p_discover_action->SetParentThreadSync(thread_task_out_sync);

                ResponseCode rc = p_discover_action->PerformAction(p_network_connection_, p_discover_request_data);
                EXPECT_EQ(ResponseCode::DISCOVER_ACTION_UNAUTHORIZED, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
            }

            TEST_F(DiscoverActionTester, DiscoverActionUnknownErrorTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                // use a blank response string
                util::String next_read_buf("\r\n");
                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(next_read_buf);
                p_network_connection_->was_read_called_ = false;

                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                std::unique_ptr<Action> p_discover_action = discovery::DiscoverAction::Create(p_core_state_);
                std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                    = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                             std::chrono::milliseconds(
                                                                 DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_,
                            DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                util::String expected_write_request(DISCOVER_ACTION_REQUEST_PREFIX);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_PREFIX);
                expected_write_request.append(test_thing_name_);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                 ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                    1>(
                    expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                p_discover_action->SetParentThreadSync(thread_task_out_sync);

                ResponseCode rc = p_discover_action->PerformAction(p_network_connection_, p_discover_request_data);
                EXPECT_EQ(ResponseCode::DISCOVER_ACTION_SERVER_ERROR, rc);
                EXPECT_TRUE(p_network_connection_->was_write_called_);
                EXPECT_TRUE(p_network_connection_->was_read_called_);
            }

            TEST_F(DiscoverActionTester, DiscoverTest) {
                EXPECT_NE(nullptr, p_network_connection_);
                EXPECT_NE(nullptr, p_core_state_);

                util::JsonDocument request_payload;
                util::JsonDocument expected_response_payload;
                util::String payload(DISCOVERY_SUCCESS_RESPONSE_PAYLOAD);
                ResponseCode rc = util::JsonParser::InitializeFromJsonString(request_payload, payload);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(DISCOVERY_TEST_LOG_TAG,
                                  "\"Error in Parsing. %s\n parse error code : %d, offset : %u",
                                  ResponseHelper::ToString(rc).c_str(),
                                  static_cast<int>(util::JsonParser::GetParseErrorCode(request_payload)),
                                  static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(request_payload)));
                    GTEST_FAIL();
                } else {
                    rc = util::JsonParser::InitializeFromJsonString(expected_response_payload, payload);
                    if (ResponseCode::SUCCESS != rc) {
                        AWS_LOG_ERROR(DISCOVERY_TEST_LOG_TAG,
                                      "\"Error in Parsing. %s\n parse error code : %d, offset : %u",
                                      ResponseHelper::ToString(rc).c_str(),
                                      static_cast<int>(util::JsonParser::GetParseErrorCode(request_payload)),
                                      static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(request_payload)));
                        GTEST_FAIL();
                    }

                    p_network_connection_->last_write_buf_.clear();
                    p_network_connection_->was_write_called_ = false;

                    util::String next_read_buf(DISCOVERY_SUCCESS_RESPONSE_HEADER_PREFIX);
                    next_read_buf.append(test_thing_name_);
                    next_read_buf.append(DISCOVERY_SUCCESS_RESPONSE_HEADER_SUFFIX);
                    next_read_buf.append(std::to_string(payload.length()));
                    next_read_buf.append("\r\n\r\n");
                    next_read_buf.append(payload);
                    next_read_buf.append("\r\n");

                    p_network_connection_->ClearNextReadBuf();
                    p_network_connection_->SetNextReadBuf(next_read_buf);
                    p_network_connection_->was_read_called_ = false;

                    std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);
                    std::unique_ptr<Action> p_discover_action = discovery::DiscoverAction::Create(p_core_state_);
                    std::shared_ptr<discovery::DiscoverRequestData> p_discover_request_data
                        = discovery::DiscoverRequestData::Create(std::move(p_test_thing_name),
                                                                 std::chrono::milliseconds(
                                                                     DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS));

                    EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                    EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                    EXPECT_CALL(*p_network_mock_,
                                DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                    util::String expected_write_request(DISCOVER_ACTION_REQUEST_PREFIX);
                    expected_write_request.append(DISCOVER_PACKET_PAYLOAD_PREFIX);
                    expected_write_request.append(test_thing_name_);
                    expected_write_request.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
                    EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                     ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                        1>(
                        expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                    std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                    p_discover_action->SetParentThreadSync(thread_task_out_sync);

                    rc = p_discover_action->PerformAction(p_network_connection_, p_discover_request_data);
                    EXPECT_TRUE(p_network_connection_->was_write_called_);
                    EXPECT_TRUE(p_network_connection_->was_read_called_);
                    util::JsonDocument received_response_payload
                        = p_discover_request_data->discovery_response_.GetResponseDocument();
                    EXPECT_TRUE(expected_response_payload == received_response_payload);
                }
            }

            TEST_F(DiscoverActionTester, GreengrassClientDiscoverActionTest) {
                EXPECT_NE(nullptr, p_network_connection_);

                std::shared_ptr<GreengrassMqttClient> p_iot_greengrass_client =
                    std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                                       std::chrono::milliseconds(2000)));
                util::JsonDocument request_payload;
                util::JsonDocument expected_response_payload;

                p_network_connection_->last_write_buf_.clear();
                p_network_connection_->was_write_called_ = false;

                const util::String test_thing_name_ = "CppSdkTestClient";

                util::String payload(DISCOVERY_SUCCESS_RESPONSE_PAYLOAD);

                util::String next_read_buf(DISCOVERY_SUCCESS_RESPONSE_HEADER_PREFIX);
                next_read_buf.append(test_thing_name_);
                next_read_buf.append(DISCOVERY_SUCCESS_RESPONSE_HEADER_SUFFIX);
                next_read_buf.append(std::to_string(payload.length()));
                next_read_buf.append("\r\n\r\n");
                next_read_buf.append(payload);
                next_read_buf.append("\r\n");

                p_network_connection_->ClearNextReadBuf();
                p_network_connection_->SetNextReadBuf(next_read_buf);
                p_network_connection_->was_read_called_ = false;

                std::unique_ptr<Utf8String> p_test_thing_name = Utf8String::Create(test_thing_name_);

                EXPECT_CALL(*p_network_mock_, IsConnected()).WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*p_network_mock_, ConnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));
                EXPECT_CALL(*p_network_mock_,
                            DisconnectInternal()).WillOnce(::testing::Return(ResponseCode::SUCCESS));

                util::String expected_write_request(DISCOVER_ACTION_REQUEST_PREFIX);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_PREFIX);
                expected_write_request.append(test_thing_name_);
                expected_write_request.append(DISCOVER_PACKET_PAYLOAD_SUFFIX);
                EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                 ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                    1>(
                    expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                DiscoveryResponse discovery_response;
                ResponseCode rc = p_iot_greengrass_client->Discover(std::chrono::milliseconds(10000),
                                                                     std::move(p_test_thing_name),
                                                                     discovery_response);

                EXPECT_EQ(ResponseCode::DISCOVER_ACTION_SUCCESS, rc);
            }

            TEST_F(DiscoverActionTester, ResponseParserTest) {
                util::JsonDocument request_payload;
                util::String payload(DISCOVERY_SUCCESS_RESPONSE_PAYLOAD);
                ResponseCode rc = util::JsonParser::InitializeFromJsonString(request_payload, payload);
                if (ResponseCode::SUCCESS != rc) {
                    AWS_LOG_ERROR(DISCOVERY_TEST_LOG_TAG,
                                  "\"Error in Parsing. %s\n parse error code : %d, offset : %u",
                                  ResponseHelper::ToString(rc).c_str(),
                                  static_cast<int>(util::JsonParser::GetParseErrorCode(request_payload)),
                                  static_cast<unsigned int>(util::JsonParser::GetParseErrorOffset(request_payload)));
                    GTEST_FAIL();
                }

                DiscoveryResponse discovery_response;
                discovery_response.SetResponseDocument(std::move(request_payload));
                util::Vector<ConnectivityInfo> parsed_response;
                util::Map<util::String, util::Vector<util::String>> ca_map;
                rc = discovery_response.GetParsedResponse(parsed_response, ca_map);

                EXPECT_EQ(parsed_response[0].group_name_, "TestGroupName1");
                EXPECT_EQ(parsed_response[0].ggc_name_, "arn: aws: iot: us-west-2: 12345678901: thing/AnyThing_0");
                EXPECT_EQ(parsed_response[0].id_, "<ID 1>");
                EXPECT_EQ(parsed_response[0].host_address_, "192.168.101.0");
                EXPECT_EQ(parsed_response[0].port_, 8080);
                EXPECT_EQ(parsed_response[0].metadata_, "<Description 1>");

                EXPECT_EQ(parsed_response[1].group_name_, "TestGroupName1");
                EXPECT_EQ(parsed_response[1].ggc_name_, "arn: aws: iot: us-west-2: 12345678901: thing/AnyThing_0");
                EXPECT_EQ(parsed_response[1].id_, "<ID 2>");
                EXPECT_EQ(parsed_response[1].host_address_, "192.168.101.1");
                EXPECT_EQ(parsed_response[1].port_, 8443);
                EXPECT_EQ(parsed_response[1].metadata_, "<Description 2>");

                EXPECT_EQ(parsed_response[2].group_name_, "TestGroupName1");
                EXPECT_EQ(parsed_response[2].ggc_name_, "arn: aws: iot: us-west-2: 12345678901: thing/AnyThing_1");
                EXPECT_EQ(parsed_response[2].id_, "<ID 3>");
                EXPECT_EQ(parsed_response[2].host_address_, "192.168.101.2");
                EXPECT_EQ(parsed_response[2].port_, 8443);
                EXPECT_EQ(parsed_response[2].metadata_, "<Description 3>");

                EXPECT_EQ(parsed_response[3].group_name_, "TestGroupName1");
                EXPECT_EQ(parsed_response[3].ggc_name_, "arn: aws: iot: us-west-2: 12345678901: thing/AnyThing_1");
                EXPECT_EQ(parsed_response[3].id_, "<ID 4>");
                EXPECT_EQ(parsed_response[3].host_address_, "192.168.101.3");
                EXPECT_EQ(parsed_response[3].port_, 8443);
                EXPECT_EQ(parsed_response[3].metadata_, "<Description 4>");

                EXPECT_EQ(ca_map[parsed_response[0].group_name_][0],
                          "-----BEGIN CERTIFICATE-----\\nsLongStringHere\\n-----END CERTIFICATE-----\\n");
            }
        }
    }
}
