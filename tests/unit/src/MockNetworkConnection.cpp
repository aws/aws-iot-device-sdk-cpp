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
 * @file MockNetworkConnection.cpp
 * @brief
 *
 */

#include "MockNetworkConnection.hpp"

namespace awsiotsdk {
    namespace tests {
        namespace mocks {
            void MockNetworkConnection::ClearNextReadBuf() {
                has_read_buf_ = false;
                next_read_buf_.clear();
            }

            void MockNetworkConnection::SetNextReadBuf(util::String next_read_buf) {
                has_read_buf_ = true;
                was_read_called_ = false;
                next_read_buf_.clear();
                for (char c : next_read_buf) {
                    next_read_buf_.push_back(static_cast<unsigned char>(c));
                }
            }

            util::String MockNetworkConnection::GetNextReadBuf() {
                return util::String(next_read_buf_.begin(), next_read_buf_.end());
            }

            bool MockNetworkConnection::HasReadBufSet() { return has_read_buf_; }

            ResponseCode MockNetworkConnection::WriteInternal(const util::String &buf, size_t &size_written_bytes_out) {
                was_write_called_ = true;
                last_write_buf_.clear();
                last_write_buf_.append(buf);
                return WriteInternalProxy(buf, size_written_bytes_out);
            }

            ResponseCode MockNetworkConnection::ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
                                                             size_t size_bytes_to_read, size_t &size_read_bytes_out) {
                was_read_called_ = true;
                size_read_bytes_out = 0;

                if (has_read_buf_) {
                    size_t remaining_bytes_in_buf = next_read_buf_.size();
                    size_t cur_read_bytes = 0;
                    size_read_bytes_out = ((size_bytes_to_read <= remaining_bytes_in_buf) ? size_bytes_to_read
                                                                                          : remaining_bytes_in_buf);
                    //size_t cur_read_bytes = 0;
                    auto begin_itr = next_read_buf_.begin();
                    auto end_itr = next_read_buf_.begin() + size_read_bytes_out;
                    auto insert_itr = buf.begin() + buf_read_offset;
                    auto insert_end_itr = buf.begin() + buf_read_offset + size_read_bytes_out;
                    buf.erase(insert_itr, insert_end_itr);
                    for (unsigned char c : next_read_buf_) {
                        buf.push_back(c);
                        if (cur_read_bytes >= size_read_bytes_out) {
                            break;
                        }
                    }

                    next_read_buf_.erase(begin_itr, end_itr);

                    if (0 == next_read_buf_.size()) {
                        has_read_buf_ = false;
                    }

                    return ResponseCode::SUCCESS;
                }
                return ReadInternalProxy(buf, size_bytes_to_read, size_read_bytes_out);
            }
        }
    }
}

