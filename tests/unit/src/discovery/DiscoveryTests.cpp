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

#define DISCOVERY_TEST_LOG_TAG "[Discovery Unit Test]"

#define DISCOVER_ACTION_MAX_RESPONSE_WAIT_TIME_MS 5000

//"GET /things/CppSdkTestClient/gg/discover HTTP/1.1\r\n\r\n"
#define DISCOVER_ACTION_REQUEST_PREFIX "GET "
#define DISCOVER_ACTION_PAYLOAD_PREFIX "/greengrass/discover/thing/"
#define DISCOVER_ACTION_PAYLOAD_SUFFIX " HTTP/1.1\r\n\r\n"

#define DISCOVERY_SUCCESS_RESPONSE_HEADER_PREFIX "HTTP/1.1 200 OK\r\n"
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
                    expected_write_request.append(DISCOVER_ACTION_PAYLOAD_PREFIX);
                    expected_write_request.append(test_thing_name_);
                    expected_write_request.append(DISCOVER_ACTION_PAYLOAD_SUFFIX);
                    EXPECT_CALL(*p_network_mock_, WriteInternalProxy(::testing::_,
                                                                     ::testing::_)).WillOnce(::testing::DoAll(::testing::SetArgReferee<
                        1>(
                        expected_write_request.length()), ::testing::Return(ResponseCode::SUCCESS)));

                    std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
                    p_discover_action->SetParentThreadSync(thread_task_out_sync);

                    rc = p_discover_action->PerformAction(p_network_connection_, p_discover_request_data);
                    EXPECT_EQ(ResponseCode::DISCOVER_ACTION_SUCCESS, rc);
                    EXPECT_TRUE(p_network_connection_->was_write_called_);
                    EXPECT_TRUE(p_network_connection_->was_read_called_);

                    util::JsonDocument received_response_payload
                        = p_discover_request_data->discovery_response_.GetResponseDocument();
                    EXPECT_TRUE(expected_response_payload == received_response_payload);
                }
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
