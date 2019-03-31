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
 * @file ThreadTask.cpp
 * @brief
 *
 */

#include "util/threading/ThreadTask.hpp"
#include "util/logging/LogMacros.hpp"
#define THREAD_TASK_LOG_TAG "[Thread Task]"

namespace awsiotsdk {
    namespace util {
        namespace Threading {
            ThreadTask::ThreadTask(DestructorAction destructor_action, std::shared_ptr<std::atomic_bool> sync_point,
                                   util::String thread_descriptor)
                : destructor_action_(destructor_action), m_continue_(std::move(sync_point)),
                  thread_descriptor_(std::move(thread_descriptor)) {
                AWS_LOGSTREAM_DEBUG(THREAD_TASK_LOG_TAG, "Creating Thread " << thread_descriptor_ << "!!");
            }

            ThreadTask::~ThreadTask() {
                Stop();
                AWS_LOGSTREAM_DEBUG(THREAD_TASK_LOG_TAG, "Exiting Thread " << thread_descriptor_ << "!!");
                if (m_thread_.joinable()) {
                    if (destructor_action_ == DestructorAction::JOIN) {
                        m_thread_.join();
                    } else {
                        m_thread_.detach();
                    }
                }
                AWS_LOGSTREAM_DEBUG(THREAD_TASK_LOG_TAG, "Successfully Exited Thread " << thread_descriptor_ << "!!");
            }

            void ThreadTask::Stop() {
                std::atomic_bool &_m_continue_ = *m_continue_;
                _m_continue_ = false;
            }
        }
    }
}
