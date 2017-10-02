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
 * @file NetworkConnection.cpp
 * @brief Network interface base class for IoT Client
 *
 * Defines an interface to the Network layer to be used by the MQTT client.
 * These functions should not be implemented/modified by the derived classes
 *
 */

#include "util/memory/stl/String.hpp"
#include "NetworkConnection.hpp"

namespace awsiotsdk {
    ResponseCode NetworkConnection::Connect() {
        std::lock(read_mutex, write_mutex);
        std::lock_guard<std::mutex> read_guard(read_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> write_guard(write_mutex, std::adopt_lock);
        return ConnectInternal();
    }

    ResponseCode NetworkConnection::Write(const util::String &buf, size_t &size_written_bytes_out) {
        ResponseCode rc;
        std::lock_guard<std::mutex> write_guard(write_mutex);
        {
            // Check connection state before calling internal write
            if (IsConnected()) {
                rc = WriteInternal(buf, size_written_bytes_out);
            } else {
                rc = ResponseCode::NETWORK_DISCONNECTED_ERROR;
            }
        }
        return rc;
    }

    ResponseCode NetworkConnection::Read(util::Vector<unsigned char> &buf, size_t buf_read_offset,
                                         size_t size_bytes_to_read, size_t &size_read_bytes_out) {
        ResponseCode rc;
        std::lock_guard<std::mutex> read_guard(read_mutex);
        {
            // Check connection state before calling internal read
            if (IsConnected()) {
                rc = ReadInternal(buf, buf_read_offset, size_bytes_to_read, size_read_bytes_out);
            } else {
                rc = ResponseCode::NETWORK_DISCONNECTED_ERROR;
            }
        }
        return rc;
    }

    ResponseCode NetworkConnection::Disconnect() {
        // Disconnect irrespective of state of other requests
        std::lock(read_mutex, write_mutex);
        std::lock_guard<std::mutex> read_guard(read_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> write_guard(write_mutex, std::adopt_lock);
        return DisconnectInternal();
    }
}
