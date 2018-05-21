/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * @file ClientCoreState.cpp
 * @brief
 *
 */

#include "util/logging/LogMacros.hpp"

#include "ClientCoreState.hpp"

#ifndef MAX_CORE_ACTION_PROCESSING_RATE_HZ
#define MAX_CORE_ACTION_PROCESSING_RATE_HZ 5
#endif

#define LOG_TAG_CLIENT_CORE_STATE "[Client Core State]"

namespace awsiotsdk {
    ClientCoreState::ClientCoreState() {
        max_queue_size_ = DEFAULT_MAX_QUEUE_SIZE;
        next_action_id_ = 1;
    }

    ClientCoreState::~ClientCoreState() {
    }

    ResponseCode
    ClientCoreState::RegisterAction(ActionType action_type, Action::CreateHandlerPtr p_action_create_handler,
                                    std::shared_ptr<ActionState> p_action_state) {
        if (nullptr == p_action_create_handler) {
            return ResponseCode::NULL_VALUE_ERROR;
        }

        action_create_handler_map_.insert(std::make_pair(action_type, p_action_create_handler));
        ResponseCode rc = GetActionCreateHandler(action_type, &p_action_create_handler);
        if (ResponseCode::SUCCESS == rc) {
            std::unique_ptr<Action> p_action = p_action_create_handler(p_action_state);
            if (nullptr == p_action) {
                rc = ResponseCode::ACTION_CREATE_FAILED;
            } else {
                action_map_.insert(std::make_pair(action_type, std::move(p_action)));
            }
        }

        return rc;
    }

    ResponseCode
    ClientCoreState::EnqueueOutboundAction(ActionType action_type, std::shared_ptr<ActionData> p_action_data,
                                           uint16_t &action_id_out) {
        if (outbound_action_queue_.Size() >= max_queue_size_) {
            // TODO : Add option to overwrite oldest action
            return ResponseCode::ACTION_QUEUE_FULL;
        }

        action_id_out = GetNextActionId();
        p_action_data->SetActionId(action_id_out);
        outbound_action_queue_.Enqueue(std::make_pair(action_type, p_action_data));

        return ResponseCode::SUCCESS;
    }

    ResponseCode
    ClientCoreState::GetActionCreateHandler(ActionType action_type, Action::CreateHandlerPtr *p_action_create_handler) {
        ResponseCode rc = ResponseCode::FAILURE;
        util::Map<ActionType, Action::CreateHandlerPtr>::const_iterator itr = action_create_handler_map_.find(
            action_type);
        if (itr != action_create_handler_map_.end()) {
            *p_action_create_handler = itr->second;
            rc = ResponseCode::SUCCESS;
        }

        return rc;
    }

    ResponseCode ClientCoreState::PerformActionAndBlock(ActionType action_type,
                                                        std::shared_ptr<ActionData> p_action_data,
                                                        std::chrono::milliseconds action_reponse_timeout) {
        uint16_t action_id_out;
        p_action_data->action_mutex_.sync_action_response = ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR;
        std::unique_lock<std::mutex> sync_action_lock(p_action_data->action_mutex_.sync_action_response_lock);
        ResponseCode rc = EnqueueOutboundAction(action_type, p_action_data, action_id_out);
        if (ResponseCode::SUCCESS != rc) {
            return rc;
        }
        p_action_data->action_mutex_.sync_action_response_wait.wait_for(sync_action_lock, action_reponse_timeout);
        return p_action_data->action_mutex_.sync_action_response;
    }

    void ClientCoreState::ProcessOutboundActionQueue(std::shared_ptr<std::atomic_bool> thread_task_out_sync) {
        ResponseCode rc;
        int action_execution_delay = 1000 / MAX_CORE_ACTION_PROCESSING_RATE_HZ;
        std::atomic_bool &_thread_task_out_sync = *thread_task_out_sync;
        do {
            // Reset ResponseCode state
            rc = ResponseCode::SUCCESS;
            auto next = std::chrono::system_clock::now() + std::chrono::milliseconds(action_execution_delay);
            std::pair<ActionType, std::shared_ptr<ActionData>> queue_front;
            if (!outbound_action_queue_.Dequeue(queue_front)) {
                continue;
            }
            ActionType action_type = queue_front.first;
            std::shared_ptr<ActionData> p_action_data = queue_front.second;

            util::Map<ActionType, std::unique_ptr<Action>>::const_iterator itr = action_map_.find(action_type);
            ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler = p_action_data->p_async_ack_handler_;
            if (itr != action_map_.end()) {
                if (nullptr != p_async_ack_handler) {
                    // Add Ack before sending request. Read request runs in separate thread and may receive response
                    // before ack is added, if we add it after sending the request.
                    rc = RegisterPendingAck(p_action_data->GetActionId(), p_action_data);
                    if (ResponseCode::SUCCESS != rc) {
                        p_async_ack_handler(p_action_data->GetActionId(), rc);
                        AWS_LOG_ERROR(LOG_TAG_CLIENT_CORE_STATE,
                                      "Registering Ack Handler for Outbound Queued Action failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                    }
                }
                // rc will be ResponseCode::SUCCESS by default at this point if no Ack handler was provided
                if (ResponseCode::SUCCESS == rc) {
                    rc = itr->second->PerformAction(p_network_connection_, p_action_data);
                    if (ResponseCode::SUCCESS != rc) {
                        if (nullptr != p_async_ack_handler) {
                            // Delete waiting for Ack for Failed Actions
                            DeletePendingAck(p_action_data->GetActionId());
                            p_async_ack_handler(p_action_data->GetActionId(), rc);
                        }
                        AWS_LOG_ERROR(LOG_TAG_CLIENT_CORE_STATE,
                                      "Performing Outbound Queued Action failed. %s",
                                      ResponseHelper::ToString(rc).c_str());
                        if (p_action_data->is_sync) {
                            std::lock_guard<std::mutex>
                                block_handler_lock(p_action_data->action_mutex_.sync_action_response_lock);
                            p_action_data->action_mutex_.sync_action_response = rc;
                            p_action_data->action_mutex_.sync_action_response_wait.notify_all();
                        }
                    } else {
                        if (p_action_data->is_sync && !pending_ack_map_.Exists(p_action_data->GetActionId())) {
                            std::lock_guard<std::mutex>
                                block_handler_lock(p_action_data->action_mutex_.sync_action_response_lock);
                            p_action_data->action_mutex_.sync_action_response = rc;
                            p_action_data->action_mutex_.sync_action_response_wait.notify_all();
                        }
                    }
                }
            } else {
                rc = ResponseCode::ACTION_NOT_REGISTERED_ERROR;
                AWS_LOG_ERROR(LOG_TAG_CLIENT_CORE_STATE,
                              "Performing Outbound Queued Action failed. %s",
                              ResponseHelper::ToString(rc).c_str());
                if(p_action_data->is_sync) {
                    std::lock_guard<std::mutex>
                        block_handler_lock(p_action_data->action_mutex_.sync_action_response_lock);
                    p_action_data->action_mutex_.sync_action_response = rc;
                    p_action_data->action_mutex_.sync_action_response_wait.notify_all();
                }
            }
            // This is not perfect since we have no control over how long an action takes.
            // But it will definitely ensure that we don't exceed the max rate
            std::this_thread::sleep_until(next);
        } while (_thread_task_out_sync);
    }

    ResponseCode ClientCoreState::RegisterPendingAck(uint16_t action_id,
                                                     std::shared_ptr<ActionData> action_data) {
        if (nullptr == action_data) {
            return ResponseCode::NULL_VALUE_ERROR;
        }

        std::unique_ptr<PendingAckData> p_pending_ack_data = std::unique_ptr<PendingAckData>(new PendingAckData());
        p_pending_ack_data->action_data_ = action_data;
        p_pending_ack_data->time_of_request_ = std::chrono::system_clock::now();

        pending_ack_map_.Insert(action_id, std::move(p_pending_ack_data));
        return ResponseCode::SUCCESS;
    }

    void ClientCoreState::DeletePendingAck(uint16_t action_id) {
        pending_ack_map_.Delete(action_id);
    }

    void ClientCoreState::ForwardReceivedAck(uint16_t action_id, ResponseCode rc) {
        // No response code because all Acks might not have registered handlers. No other possible error
        PendingAckData *itr = pending_ack_map_.Get(action_id);
        if (itr != nullptr) {
            ActionData::AsyncAckNotificationHandlerPtr ack_handler = itr->action_data_->p_async_ack_handler_;
            if (ack_handler != nullptr) {
                ack_handler(action_id, rc);
            } else if (itr->action_data_->is_sync){
                std::lock_guard<std::mutex>
                    block_handler_lock(itr->action_data_->action_mutex_.sync_action_response_lock);
                itr->action_data_->action_mutex_.sync_action_response = rc;
                itr->action_data_->action_mutex_.sync_action_response_wait.notify_all();
            }
            pending_ack_map_.Delete(action_id);
        }
    }

    void ClientCoreState::ClearRegisteredActions() {
        action_map_.clear();
    }

    void ClientCoreState::ClearOutboundActionQueue() {
        outbound_action_queue_.ClearAndExit();
    }
}