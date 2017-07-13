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
        continue_execution_ = std::make_shared<std::atomic_bool>(true);
        max_queue_size_ = DEFAULT_MAX_QUEUE_SIZE;
        max_hardware_threads_ = std::thread::hardware_concurrency();
        cur_core_threads_ = 0;
        next_action_id_ = 1;
    }

    ClientCoreState::~ClientCoreState() {
        std::atomic_bool &_continue_execution_ = *continue_execution_;
        _continue_execution_ = false;
    }

    ResponseCode
    ClientCoreState::RegisterAction(ActionType action_type, Action::CreateHandlerPtr p_action_create_handler,
                                    std::shared_ptr<ActionState> p_action_state) {
        if (nullptr == p_action_create_handler) {
            return ResponseCode::NULL_VALUE_ERROR;
        }

        std::lock_guard<std::mutex> register_action_lock_guard(register_action_lock_);

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
        if (outbound_action_queue_.size() >= max_queue_size_) {
            // TODO : Add option to overwrite oldest action
            return ResponseCode::ACTION_QUEUE_FULL;
        }

        action_id_out = GetNextActionId();
        p_action_data->SetActionId(action_id_out);
        outbound_action_queue_.push(std::make_pair(action_type, p_action_data));

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

    void ClientCoreState::SyncActionHandler(uint16_t action_id, ResponseCode rc) {
        std::lock_guard<std::mutex> block_handler_lock(sync_action_response_lock_);
        sync_action_response_ = rc;
        sync_action_response_wait_.notify_all();
    }

    ResponseCode ClientCoreState::PerformAction(ActionType action_type, std::shared_ptr<ActionData> p_action_data,
                                                std::chrono::milliseconds action_reponse_timeout) {
        std::lock_guard<std::mutex> sync_action_lock(sync_action_request_lock_);
        ResponseCode rc = ResponseCode::FAILURE;

        util::Map<ActionType, std::unique_ptr<Action>>::const_iterator itr = action_map_.find(action_type);
        if (itr == action_map_.end()) {
            rc = ResponseCode::ACTION_NOT_REGISTERED_ERROR;
        } else {
            std::unique_lock<std::mutex> block_handler_lock(sync_action_response_lock_);
            {
                sync_action_response_ = ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR;
                p_action_data->p_async_ack_handler_ = std::bind(&ClientCoreState::SyncActionHandler, this,
                                                                std::placeholders::_1, std::placeholders::_2);
                p_action_data->SetActionId(GetNextActionId());
                rc = itr->second->PerformAction(p_network_connection_, p_action_data);
            }

            if (ResponseCode::SUCCESS == rc
                && pending_ack_map_.find(p_action_data->GetActionId()) != pending_ack_map_.end()) {
                sync_action_response_wait_.wait_for(block_handler_lock, action_reponse_timeout);
                rc = sync_action_response_;
            }
        }

        return rc;
    }

    void ClientCoreState::ProcessOutboundActionQueue(std::shared_ptr<std::atomic_bool> thread_task_out_sync) {
        ResponseCode rc = ResponseCode::SUCCESS;
        int action_execution_delay = 1000 / MAX_CORE_ACTION_PROCESSING_RATE_HZ;
        std::atomic_bool &_thread_task_out_sync = *thread_task_out_sync;
        do {
            // Reset ResponseCode state
            rc = ResponseCode::SUCCESS;
            if (/*!process_queued_actions_ || */outbound_action_queue_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_CORE_THREAD_SLEEP_DURATION_MS));
                continue;
            }
            std::lock_guard<std::mutex> sync_action_lock(sync_action_request_lock_);
            auto next = std::chrono::system_clock::now() + std::chrono::milliseconds(action_execution_delay);
            ActionType action_type = outbound_action_queue_.front().first;
            std::shared_ptr<ActionData> p_action_data = outbound_action_queue_.front().second;

            outbound_action_queue_.pop();
            util::Map<ActionType, std::unique_ptr<Action>>::const_iterator itr = action_map_.find(action_type);
            ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler = p_action_data->p_async_ack_handler_;
            if (itr != action_map_.end()) {
                if (nullptr != p_async_ack_handler) {
                    // Add Ack before sending request. Read request runs in separate thread and may receive response
                    // before ack is added, if we add it after sending the request.
                    rc = RegisterPendingAck(p_action_data->GetActionId(), p_async_ack_handler);
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
                    }
                }
            } else {
                rc = ResponseCode::ACTION_NOT_REGISTERED_ERROR;
                AWS_LOG_ERROR(LOG_TAG_CLIENT_CORE_STATE,
                              "Performing Outbound Queued Action failed. %s",
                              ResponseHelper::ToString(rc).c_str());
            }
            // This is not perfect since we have no control over how long an action takes.
            // But it will definitely ensure that we don't exceed the max rate
            std::this_thread::sleep_until(next);
        } while (_thread_task_out_sync);
    }

    ResponseCode ClientCoreState::RegisterPendingAck(uint16_t action_id,
                                                     ActionData::AsyncAckNotificationHandlerPtr p_async_ack_handler) {
        if (nullptr == p_async_ack_handler) {
            return ResponseCode::NULL_VALUE_ERROR;
        }

        std::unique_ptr<PendingAckData> p_pending_ack_data = std::unique_ptr<PendingAckData>(new PendingAckData());
        p_pending_ack_data->p_async_ack_handler_ = p_async_ack_handler;
        p_pending_ack_data->time_of_request_ = std::chrono::system_clock::now();

        std::lock_guard<std::mutex> sync_action_lock(ack_map_lock_);
        pending_ack_map_.insert(std::make_pair(action_id, std::move(p_pending_ack_data)));
        return ResponseCode::SUCCESS;
    }

    void ClientCoreState::DeletePendingAck(uint16_t action_id) {
        std::lock_guard<std::mutex> sync_action_lock(ack_map_lock_);
        util::Map<uint16_t, std::unique_ptr<PendingAckData>>::const_iterator itr = pending_ack_map_.find(action_id);
        if (itr != pending_ack_map_.end()) {
            pending_ack_map_.erase(itr);
        }
    }

    void ClientCoreState::DeleteExpiredAcks() {
        std::lock_guard<std::mutex> sync_action_lock(ack_map_lock_);
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        util::Map<uint16_t, std::unique_ptr<PendingAckData>>::const_iterator itr = pending_ack_map_.begin();
        while (itr != pending_ack_map_.end()) {
            std::chrono::seconds diff = std::chrono::duration_cast<std::chrono::seconds>(
                now - itr->second->time_of_request_);
            if (diff > ack_timeout_) {
                itr->second->p_async_ack_handler_(itr->first, ResponseCode::MQTT_REQUEST_TIMEOUT_ERROR);
                pending_ack_map_.erase(itr);
            }
        }
    }

    void ClientCoreState::ForwardReceivedAck(uint16_t action_id, ResponseCode rc) {
        std::lock_guard<std::mutex> sync_action_lock(ack_map_lock_);
        // No response code because all Acks might not have registered handlers. No other possible error
        util::Map<uint16_t, std::unique_ptr<PendingAckData>>::const_iterator itr = pending_ack_map_.find(action_id);
        if (itr != pending_ack_map_.end()) {
            itr->second->p_async_ack_handler_(action_id, rc);
            pending_ack_map_.erase(itr);
        }
    }

    void ClientCoreState::ClearRegisteredActions() {
        action_map_.clear();
    }

    void ClientCoreState::ClearOutboundActionQueue() {
        util::Queue<std::pair<ActionType, std::shared_ptr<ActionData>>>().swap(outbound_action_queue_);
    }
}