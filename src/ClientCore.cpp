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
 * @file ClientCore.cpp
 * @brief
 */

#include <iostream>

#include "util/logging/LogMacros.hpp"
#include "ClientCore.hpp"

#define LOG_TAG_CLIENT_CORE "[Client Core]"

namespace awsiotsdk {
    std::unique_ptr<ClientCore> ClientCore::Create(std::shared_ptr<NetworkConnection> p_network_connection,
                                                   std::shared_ptr<ClientCoreState> p_state) {
        if (nullptr == p_network_connection || nullptr == p_state) {
            return nullptr;
        }

        return std::unique_ptr<ClientCore>(new ClientCore(p_network_connection, p_state));
    }

    ClientCore::ClientCore(std::shared_ptr<NetworkConnection> p_network_connection,
                           std::shared_ptr<ClientCoreState> p_state) {
        p_client_core_state_ = p_state;
        p_client_core_state_->p_network_connection_ = p_network_connection;
        p_client_core_state_->SetProcessQueuedActions(false);

        std::shared_ptr<std::atomic_bool> thread_task_out_sync = std::make_shared<std::atomic_bool>(true);
        std::shared_ptr<util::Threading::ThreadTask> thread_task_out = std::shared_ptr<util::Threading::ThreadTask>(
            new util::Threading::ThreadTask(util::Threading::DestructorAction::JOIN,
                                            thread_task_out_sync,
                                            "Outbound Action Processing"));
        thread_map_.insert(std::make_pair(ActionType::CORE_PROCESS_OUTBOUND, thread_task_out));
        thread_task_out->Run(&ClientCoreState::ProcessOutboundActionQueue, p_client_core_state_, thread_task_out_sync);
    }

    ResponseCode ClientCore::RegisterAction(ActionType action_type, Action::CreateHandlerPtr p_action_create_handler) {
        return p_client_core_state_->RegisterAction(action_type, p_action_create_handler, p_client_core_state_);
    }

    ResponseCode ClientCore::PerformAction(ActionType action_type, std::shared_ptr<ActionData> p_action_data,
                                           std::chrono::milliseconds action_reponse_timeout) {
        return p_client_core_state_->PerformAction(action_type, p_action_data, action_reponse_timeout);
    }

    ResponseCode ClientCore::PerformActionAsync(ActionType action_type, std::shared_ptr<ActionData> p_action_data,
                                                uint16_t &action_id_out) {
        return p_client_core_state_->EnqueueOutboundAction(action_type, p_action_data, action_id_out);
    }

    ResponseCode ClientCore::CreateActionRunner(ActionType action_type, std::shared_ptr<ActionData> p_action_data) {
        Action::CreateHandlerPtr p_action_create_handler = nullptr;
        std::unique_ptr<Action> p_action = nullptr;

        ResponseCode rc = p_client_core_state_->GetActionCreateHandler(action_type, &p_action_create_handler);
        if (ResponseCode::SUCCESS != rc) {
            return rc;
        }

        p_action = p_action_create_handler(p_client_core_state_);
        if (nullptr == p_action) {
            rc = ResponseCode::NULL_VALUE_ERROR;
        } else {
            std::shared_ptr<std::atomic_bool> thread_task_sync = std::make_shared<std::atomic_bool>(true);
            p_action->SetParentThreadSync(thread_task_sync);
            std::shared_ptr<util::Threading::ThreadTask> thread_task = std::shared_ptr<util::Threading::ThreadTask>(
                new util::Threading::ThreadTask(util::Threading::DestructorAction::JOIN,
                                                thread_task_sync,
                                                p_action->GetActionInfo()));
            thread_map_.insert(std::make_pair(action_type, thread_task));
            thread_task->Run(&Action::PerformAction, std::move(p_action), p_client_core_state_->p_network_connection_,
                             p_action_data);
        }

        return rc;
    }

    void ClientCore::GracefulShutdownAllThreadTasks() {
        thread_map_.clear();
    }

    ClientCore::~ClientCore() {
        thread_map_.clear();
    }
}
