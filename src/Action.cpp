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
 * @file Action.cpp
 * @brief Action Base class and related definitions for IoT Client
 *
 * Defines a base class to be used by all Actions that can be run by the
 * IoT Client. Also contains definitions for related Action types like
 * ActionType, ActionState and ActionData
 *
 */

#include "Action.hpp"

namespace awsiotsdk {
    Action::Action(ActionType action_type, util::String action_info_string) {
        p_thread_continue_ = std::make_shared<std::atomic_bool>(false); // Only one execution by default
        action_type_ = action_type;
        action_info_string_ = action_info_string;
    }

    ResponseCode Action::ReadFromNetworkBuffer(std::shared_ptr<NetworkConnection> p_network_connection,
                                               util::Vector<unsigned char> &read_buf,
                                               size_t bytes_to_read) {
        // TODO : Check if there are corner cases for this function not terminating until it has read the required number of bytes
        // TODO : Is there a more efficient way to do this?

        if (nullptr == p_network_connection) {
            return ResponseCode::NULL_VALUE_ERROR;
        }

        size_t total_read_bytes = 0;
        size_t cur_read_bytes = 0;
        ResponseCode rc = ResponseCode::FAILURE;

        std::atomic_bool &_p_thread_continue_ = *p_thread_continue_;
        read_buf.resize(bytes_to_read);
        do {
            rc = p_network_connection->Read(read_buf,
                                            total_read_bytes,
                                            bytes_to_read - total_read_bytes,
                                            cur_read_bytes);
            total_read_bytes += cur_read_bytes;
            if (total_read_bytes != bytes_to_read) {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NETWORK_ACTION_THREAD_SLEEP_DURATION_MS));
            }
        } while (_p_thread_continue_ && total_read_bytes != bytes_to_read && ResponseCode::SUCCESS == rc);

        if (ResponseCode::SUCCESS == rc && total_read_bytes != bytes_to_read) {
            if (!_p_thread_continue_) {
                rc = ResponseCode::THREAD_EXITING;
            } else {
                rc = ResponseCode::FAILURE;
            }
        }

        return rc;
    }

    ResponseCode Action::WriteToNetworkBuffer(std::shared_ptr<NetworkConnection> p_network_connection,
                                              const util::String &write_buf) {
        // TODO : Check if there are corner cases for this function not terminating until it has written the required number of bytes
        // TODO : Is there a more efficient way to do this?

        if (nullptr == p_network_connection) {
            return ResponseCode::NULL_VALUE_ERROR;
        }

        if (0 == write_buf.length()) {
            return ResponseCode::NETWORK_NOTHING_TO_WRITE_ERROR;
        }

        size_t total_written_bytes = 0;
        size_t cur_written_bytes = 0;
        size_t bytes_to_write = write_buf.length();
        ResponseCode rc = ResponseCode::FAILURE;

        std::atomic_bool &_p_thread_continue_ = *p_thread_continue_;
        util::String temp_buf = write_buf;
        do {
            rc = p_network_connection->Write(temp_buf, cur_written_bytes);
            total_written_bytes += cur_written_bytes;
            if (total_written_bytes != bytes_to_write) {
                temp_buf = write_buf.substr(total_written_bytes);
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_NETWORK_ACTION_THREAD_SLEEP_DURATION_MS));
            }
        } while (_p_thread_continue_ && total_written_bytes != cur_written_bytes && ResponseCode::SUCCESS == rc);

        if (ResponseCode::SUCCESS == rc && total_written_bytes != bytes_to_write) {
            if (!_p_thread_continue_) {
                rc = ResponseCode::THREAD_EXITING;
            } else {
                rc = ResponseCode::FAILURE;
            }
        }

        return rc;
    }
}