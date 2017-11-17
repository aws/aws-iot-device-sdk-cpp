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
 * @file MockNetworkConnection.hpp
 * @brief
 *
 */


#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "TestHelper.hpp"
#include "ResponseCode.hpp"
#include "NetworkConnection.hpp"

namespace awsiotsdk {
    namespace tests {
        namespace mocks {
            class MockNetworkConnection : public NetworkConnection {
            protected:
                util::Vector<unsigned char> next_read_buf_;
                std::atomic_bool has_read_buf_;

            public:
                std::atomic_bool was_read_called_;
                std::atomic_bool was_write_called_;
                util::String last_write_buf_;

                void ClearNextReadBuf();
                void SetNextReadBuf(util::String next_read_buf);
                util::String GetNextReadBuf();
                bool HasReadBufSet();

                virtual ResponseCode WriteInternal(const util::String &buf, size_t &size_written_bytes_out);

                virtual ResponseCode ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset,
                                                  size_t size_bytes_to_read, size_t &size_read_bytes_out);

                MOCK_METHOD0(ConnectInternal, ResponseCode());
                MOCK_METHOD2(WriteInternalProxy, ResponseCode(
                    const util::String &, size_t &));
                MOCK_METHOD3(ReadInternalProxy, ResponseCode(util::Vector<unsigned char> & , size_t, size_t & ));
                MOCK_METHOD0(DisconnectInternal, ResponseCode());
                MOCK_METHOD0(IsConnected, bool());
                MOCK_METHOD0(IsPhysicalLayerConnected, bool());
                MOCK_METHOD0(Destroy, ResponseCode());
            };
        }
    }
}

