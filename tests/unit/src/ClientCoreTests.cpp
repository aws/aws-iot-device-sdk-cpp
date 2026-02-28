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
 * @file ClientCoreTests.cpp
 * @brief
 *
 */

#include <atomic>
#include <gtest/gtest.h>

#include "MockNetworkConnection.hpp"

#include "ClientCore.hpp"

namespace awsiotsdk {
    namespace tests {
        namespace unit {
            class ClientCoreTester : public ::testing::Test {
            protected:
                class TestActionData : public ActionData {
                public:
                    uint16_t action_id_;
                    std::atomic_int perform_action_count_;

                    uint16_t GetActionId() { return action_id_; }
                    void SetActionId(uint16_t action_id) { action_id_ = action_id; }
                    TestActionData() {
                        perform_action_count_ = 0;
                    }
                };

                class TestAction : public Action {
                protected:
                    std::shared_ptr<ClientCoreState> p_client_state_;

                public:
                    static std::atomic_int cur_instance_count_;
                    static std::atomic_int total_instance_count_;
                    static std::atomic_int total_perform_action_call_count_;

                    TestAction(std::shared_ptr<ClientCoreState> p_client_state) : Action(ActionType::RESERVED_ACTION,
                                                                                         "Test Action") {
                        p_client_state_ = p_client_state;
                        cur_instance_count_++;
                        total_instance_count_++;
                    }

                    static std::unique_ptr<Action> Create(std::shared_ptr<ActionState> p_action_state);
                    ResponseCode PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                               std::shared_ptr<ActionData> p_action_data);

                    ~TestAction() {
                        cur_instance_count_--;
                    }

                    static void Reset() {
                        cur_instance_count_ = 0;
                        total_instance_count_ = 0;
                        total_perform_action_call_count_ = 0;
                    }
                };

                std::shared_ptr<ClientCoreState> p_core_state_;
                std::unique_ptr<ClientCore> p_client_core_;
                std::mutex sync_action_response_lock_;
                std::condition_variable sync_action_response_wait_;
                ResponseCode sync_action_response_;

                ClientCoreTester() {
                    p_core_state_ = std::make_shared<ClientCoreState>();
                    std::shared_ptr<NetworkConnection>
                        p_network_connection = std::make_shared<tests::mocks::MockNetworkConnection>();
                    p_client_core_ = ClientCore::Create(p_network_connection, p_core_state_);
                }

            public:
                void SyncActionHandler(uint16_t action_id, ResponseCode rc);
            };

            std::atomic_int ClientCoreTester::TestAction::cur_instance_count_(0);
            std::atomic_int ClientCoreTester::TestAction::total_instance_count_(0);
            std::atomic_int ClientCoreTester::TestAction::total_perform_action_call_count_(0);

            std::unique_ptr<Action> ClientCoreTester::TestAction::Create(std::shared_ptr<ActionState> p_action_state) {
                std::shared_ptr<ClientCoreState>
                    p_client_state = std::dynamic_pointer_cast<ClientCoreState>(p_action_state);
                if (nullptr == p_client_state) {
                    return nullptr;
                }

                return std::unique_ptr<ClientCoreTester::TestAction>(new ClientCoreTester::TestAction(p_client_state));
            }

            ResponseCode ClientCoreTester::TestAction::PerformAction(std::shared_ptr<NetworkConnection> p_network_connection,
                                                                     std::shared_ptr<ActionData> p_action_data) {
                std::shared_ptr<TestActionData>
                    p_test_action_data = std::dynamic_pointer_cast<TestActionData>(p_action_data);
                if (nullptr == p_test_action_data) {
                    return ResponseCode::NULL_VALUE_ERROR;
                }

                p_test_action_data->perform_action_count_++;
                total_perform_action_call_count_++;
                p_client_state_->ForwardReceivedAck(p_test_action_data->GetActionId(), ResponseCode::SUCCESS);
                return ResponseCode::SUCCESS;
            }

            void ClientCoreTester::SyncActionHandler(uint16_t action_id, ResponseCode rc) {
                std::lock_guard<std::mutex> block_handler_lock(sync_action_response_lock_);
                std::cout << std::endl << "Sync Action Handler called" << std::endl;
                sync_action_response_ = rc;
                sync_action_response_wait_.notify_all();
            }

            // Test Client Core create, should fail for invalid parameters
            TEST(ClientCoreCreateTester, CreateFailed) {
                std::unique_ptr<ClientCore> p_client_core = ClientCore::Create(nullptr, nullptr);
                EXPECT_EQ(nullptr, p_client_core);

                std::shared_ptr<ClientCoreState> p_core_state = std::make_shared<ClientCoreState>();
                p_client_core = ClientCore::Create(nullptr, p_core_state);
                EXPECT_EQ(nullptr, p_client_core);

                std::shared_ptr<NetworkConnection>
                    p_network_connection = std::make_shared<tests::mocks::MockNetworkConnection>();
                p_client_core = ClientCore::Create(p_network_connection, nullptr);
                EXPECT_EQ(nullptr, p_client_core);
            }

            // Test Client Core create, should be created successfully
            TEST(ClientCoreCreateTester, CreateSuccess) {
                std::shared_ptr<ClientCoreState> p_core_state = std::make_shared<ClientCoreState>();
                std::shared_ptr<NetworkConnection>
                    p_network_connection = std::make_shared<tests::mocks::MockNetworkConnection>();
                std::unique_ptr<ClientCore> p_client_core = ClientCore::Create(p_network_connection, p_core_state);
                EXPECT_NE(nullptr, p_client_core);
            }

            // Test Register Action - Unregistered Action should be registered successfully,
            // Action should be created ONCE per registration
            // Also tests Sync Action execution
            TEST_F(ClientCoreTester, RegisterOnceSuccess) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                TestAction::Reset();

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = p_client_core_->PerformAction(ActionType::RESERVED_ACTION,
                                                   p_test_action_data,
                                                   std::chrono::milliseconds(200));
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);
                EXPECT_EQ(1, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);

                std::shared_ptr<TestActionData> p_test_action_data_puback = std::make_shared<TestActionData>();
                rc = p_client_core_->RegisterAction(ActionType::PUBACK, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = p_client_core_->PerformAction(ActionType::PUBACK,
                                                   p_test_action_data_puback,
                                                   std::chrono::milliseconds(200));
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(2, TestAction::cur_instance_count_);
                EXPECT_EQ(2, TestAction::total_instance_count_);
                EXPECT_EQ(2, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);
            }

            // Test Register Action - Already registered action should override,
            // New action should be created ONCE, Old action instance count should become ZERO
            // Also tests Sync Action execution
            TEST_F(ClientCoreTester, RegisterAgainSuccess) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                TestAction::Reset();

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = p_client_core_->PerformAction(ActionType::RESERVED_ACTION,
                                                   p_test_action_data,
                                                   std::chrono::milliseconds(200));
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(2, TestAction::total_instance_count_);
                EXPECT_EQ(1, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);
            }

            // Test Async Action Execution  - Action not registered
            TEST_F(ClientCoreTester, TestAsyncFailOnUnregistered) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                TestAction::Reset();

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->PerformAction(ActionType::RESERVED_ACTION,
                                                                p_test_action_data,
                                                                std::chrono::milliseconds(200));
                EXPECT_EQ(ResponseCode::ACTION_NOT_REGISTERED_ERROR, rc);
                EXPECT_EQ(0, TestAction::cur_instance_count_);
                EXPECT_EQ(0, TestAction::total_instance_count_);
            }

            // Test Async Action execution - Action is registered, Total action instance count increments by one
            // for each register action. Current instance count is equal to number of Action Types the Action is
            // registered against
            TEST_F(ClientCoreTester, TestAsyncSuccess) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                uint16_t action_id = 0;

                TestAction::Reset();
                p_client_core_->SetProcessQueuedActions(true);

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);

                std::unique_lock<std::mutex> block_handler_lock(sync_action_response_lock_);
                {
                    sync_action_response_ = ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR;
                    p_test_action_data->p_async_ack_handler_ = std::bind(&ClientCoreTester::SyncActionHandler, this,
                                                                         std::placeholders::_1, std::placeholders::_2);
                    rc = p_client_core_->PerformActionAsync(ActionType::RESERVED_ACTION, p_test_action_data, action_id);
                    EXPECT_EQ(ResponseCode::SUCCESS, rc);
                    EXPECT_EQ(1, action_id);
                }

                sync_action_response_wait_.wait_for(block_handler_lock, std::chrono::milliseconds(2000));
                rc = sync_action_response_;
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);

                sync_action_response_ = ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR;
                rc = p_client_core_->PerformActionAsync(ActionType::RESERVED_ACTION, p_test_action_data, action_id);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(2, action_id);

                sync_action_response_wait_.wait_for(block_handler_lock, std::chrono::milliseconds(2000));
                rc = sync_action_response_;
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);
                EXPECT_EQ(2, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(2, p_test_action_data->perform_action_count_);
            }

            // Test Async Action execution - Action Ack is called for multiple distinct actions
            // Registers TestAction with two different Action Types to test behavior
            TEST_F(ClientCoreTester, MultipleActionsAckSuccess) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                TestAction::Reset();

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_TRUE(ResponseCode::SUCCESS == rc);
                rc = p_client_core_->PerformAction(ActionType::RESERVED_ACTION,
                                                   p_test_action_data,
                                                   std::chrono::milliseconds(200));
                EXPECT_TRUE(ResponseCode::SUCCESS == rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);
                EXPECT_EQ(1, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);

                std::shared_ptr<TestActionData> p_test_action_data_puback = std::make_shared<TestActionData>();
                rc = p_client_core_->RegisterAction(ActionType::PUBACK, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = p_client_core_->PerformAction(ActionType::PUBACK,
                                                   p_test_action_data_puback,
                                                   std::chrono::milliseconds(200));
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(2, TestAction::cur_instance_count_);
                EXPECT_EQ(2, TestAction::total_instance_count_);
                EXPECT_EQ(2, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);
            }

            // Test Action queue full behavior
            TEST_F(ClientCoreTester, ActionQueueFull) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                uint16_t action_id = 0;

                TestAction::Reset();

                size_t cur_max_queue_size = p_core_state_->GetMaxActionQueueSize();

                p_core_state_->SetMaxActionQueueSize(1);
                p_client_core_->SetProcessQueuedActions(false);

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                rc = p_client_core_->PerformActionAsync(ActionType::RESERVED_ACTION, p_test_action_data, action_id);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);

                rc = p_client_core_->PerformActionAsync(ActionType::RESERVED_ACTION, p_test_action_data, action_id);
                EXPECT_EQ(ResponseCode::ACTION_QUEUE_FULL, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);

                p_client_core_->SetProcessQueuedActions(true);
                // Sleep and allow outbound queue to process action
                for (size_t itr = 0; itr < 50; itr++) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if (0 != p_test_action_data->perform_action_count_) {
                        break;
                    }
                }
                EXPECT_EQ(1, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);

                // Attempt to enqueue action again, should Succeed now that action queue is empty
                rc = p_client_core_->PerformActionAsync(ActionType::RESERVED_ACTION, p_test_action_data, action_id);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(1, TestAction::total_instance_count_);

                p_core_state_->SetMaxActionQueueSize(cur_max_queue_size);
            }

            // Test creation of action thread runner, thread should execute successfully,
            // Action instance count is incremented, Action instance count decremented on thread destroy
            TEST_F(ClientCoreTester, ActionRunner) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                TestAction::Reset();

                std::shared_ptr<TestActionData> p_test_action_data = std::make_shared<TestActionData>();

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, TestAction::Create);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                rc = p_client_core_->CreateActionRunner(ActionType::RESERVED_ACTION, p_test_action_data);
                EXPECT_EQ(ResponseCode::SUCCESS, rc);

                // Sleep and allow thread to process action and exit
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                EXPECT_EQ(1, TestAction::total_perform_action_call_count_);
                EXPECT_EQ(1, p_test_action_data->perform_action_count_);
                EXPECT_EQ(1, TestAction::cur_instance_count_);
                EXPECT_EQ(2, TestAction::total_instance_count_);
            }

            // Test Client Core destroy, all threads should successfully stop, no exceptions

            TEST_F(ClientCoreTester, TestNullRegisterAction) {
                EXPECT_NE(nullptr, p_client_core_);
                EXPECT_NE(nullptr, p_core_state_);

                TestAction::Reset();

                std::shared_ptr<TestActionData> p_test_action_data = nullptr;

                ResponseCode rc = p_client_core_->RegisterAction(ActionType::RESERVED_ACTION, nullptr);
                EXPECT_EQ(ResponseCode::NULL_VALUE_ERROR, rc);
            }
        }
    }
}