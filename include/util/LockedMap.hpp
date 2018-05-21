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

#include "util/memory/stl/Map.hpp"
#include <mutex>

namespace awsiotsdk {
    namespace util {
        template<typename T, typename V>
        class LockedMap{
        public:
            void Insert(T index, std::unique_ptr<V> value) {
                std::lock_guard<std::mutex> map_lock(map_mutex_);
                locked_map_.insert(std::make_pair(index, std::move(value)));
            }

            bool Exists(T index) {
                std::lock_guard<std::mutex> map_lock(map_mutex_);
                typename util::Map<T, std::unique_ptr<V>>::const_iterator itr = locked_map_.find(index);
                if(itr != locked_map_.end()) {
                    return true;
                }
                return false;
            }

            V* Get(T index) {
                std::lock_guard<std::mutex> map_lock(map_mutex_);
                typename util::Map<T, std::unique_ptr<V>>::const_iterator itr = locked_map_.find(index);
                if(itr != locked_map_.end()) {
                    V *return_value = itr->second.get();
                    return return_value;
                }
                else {
                    return nullptr;
                }
            }

            void Delete(T index) {
                std::lock_guard<std::mutex> map_lock(map_mutex_);
                typename util::Map<T, std::unique_ptr<V>>::const_iterator itr = locked_map_.find(index);
                if(itr != locked_map_.end()) {
                    locked_map_.erase(itr);
                }
            }

            void Clear() {
                std::lock_guard<std::mutex> map_lock(map_mutex_);
                locked_map_.clear();
            }

        private:
            std::mutex map_mutex_;
            typename util::Map<T, std::unique_ptr<V>> locked_map_;
        };
    }
}
