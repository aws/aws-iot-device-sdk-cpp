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
 * @file JobsMock.hpp
 * @brief
 *
 */


#pragma once

namespace awsiotsdk {

    class JobsMock : public Jobs {
    protected:
        static util::String last_update_payload_;

    public:
        JobsMock() : Jobs(nullptr, mqtt::QoS::QOS1, "testThingName", "testClientToken") {}

        ResponseCode SendJobsUpdate(const util::String &jobId,
                                    Jobs::JobExecutionStatus status,
                                    const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>(),
                                    int64_t expectedVersion = 0,    // set to 0 to ignore
                                    int64_t executionNumber = 0,    // set to 0 to ignore
                                    bool includeJobExecutionState = false,
                                    bool includeJobDocument = false) {
            util::Map<util::String, util::String> statusDetailsMapCleaned = statusDetailsMap;
            statusDetailsMapCleaned.erase("arch");
            statusDetailsMapCleaned.erase("cwd");
            statusDetailsMapCleaned.erase("platform");
            last_update_payload_ = SerializeJobExecutionUpdatePayload(status, statusDetailsMapCleaned, expectedVersion, executionNumber,
                                                                      includeJobExecutionState, includeJobDocument);
            return ResponseCode::SUCCESS;
        }

        static util::String GetLastUpdatePayload() {
            return last_update_payload_;
        }
    };
}
