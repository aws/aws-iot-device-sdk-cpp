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

#pragma once

#include "util/memory/stl/Queue.hpp"
#include <mutex>

namespace awsiotsdk {
    namespace util {
        template <class T>
        class BlockingQueue {
        public:
            void Enqueue (T value) {
                std::lock_guard<std::mutex> queue_lock(queue_mutex_);
                locked_queue_.push(value);
                queue_exit_ = false;
                unblock_.notify_one();
            }

            bool Dequeue (T& value) {
                std::unique_lock<std::mutex> queue_lock(queue_mutex_);
                unblock_.wait(queue_lock, [this] { return (!locked_queue_.empty() || queue_exit_); }); 
                if (queue_exit_) {
                    return false;
                }
                value = locked_queue_.front();
                locked_queue_.pop();
                queue_lock.unlock();
                return true;
            }

            uint16_t Size() {
                return locked_queue_.size();
            }

            void ClearAndExit() {
                std::lock_guard<std::mutex> queue_lock(queue_mutex_);
                util::Queue<T>().swap(locked_queue_);
                queue_exit_ = true;
                unblock_.notify_one();
            }

        private:
            std::mutex queue_mutex_;
            util::Queue<T> locked_queue_;
            std::condition_variable unblock_;
            bool queue_exit_;
        };
    }
}


