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
 * @file Jobs.cpp
 * @brief
 *
 */

#include "util/logging/LogMacros.hpp"

#include "jobs/Jobs.hpp"

#define BASE_THINGS_TOPIC "$aws/things/"

#define NOTIFY_OPERATION "notify"
#define NOTIFY_NEXT_OPERATION "notify-next"
#define GET_OPERATION "get"
#define START_NEXT_OPERATION "start-next"
#define WILDCARD_OPERATION "+"
#define UPDATE_OPERATION "update"
#define ACCEPTED_REPLY "accepted"
#define REJECTED_REPLY "rejected"
#define WILDCARD_REPLY "#"

namespace awsiotsdk {
    std::unique_ptr<Jobs> Jobs::Create(std::shared_ptr<MqttClient> p_mqtt_client,
                                       mqtt::QoS qos,
                                       const util::String &thing_name,
                                       const util::String &client_token) {
        if (nullptr == p_mqtt_client) {
            return nullptr;
        }

        return std::unique_ptr<Jobs>(new Jobs(p_mqtt_client, qos, thing_name, client_token));
    }

    Jobs::Jobs(std::shared_ptr<MqttClient> p_mqtt_client,
               mqtt::QoS qos,
               const util::String &thing_name,
               const util::String &client_token) {
        p_mqtt_client_ = p_mqtt_client;
        qos_ = qos;
        thing_name_ = thing_name;
        client_token_ = client_token;
    };

    bool Jobs::BaseTopicRequiresJobId(JobExecutionTopicType topicType) {
        switch (topicType) {
            case JOB_UPDATE_TOPIC:
            case JOB_DESCRIBE_TOPIC:
                return true;
            case JOB_NOTIFY_TOPIC:
            case JOB_NOTIFY_NEXT_TOPIC:
            case JOB_START_NEXT_TOPIC:
            case JOB_GET_PENDING_TOPIC:
            case JOB_WILDCARD_TOPIC:
            case JOB_UNRECOGNIZED_TOPIC:
            default:
                return false;
        }
    };

    const util::String Jobs::GetOperationForBaseTopic(JobExecutionTopicType topicType) {
        switch (topicType) {
            case JOB_UPDATE_TOPIC:
                return UPDATE_OPERATION;
            case JOB_NOTIFY_TOPIC:
                return NOTIFY_OPERATION;
            case JOB_NOTIFY_NEXT_TOPIC:
                return NOTIFY_NEXT_OPERATION;
            case JOB_GET_PENDING_TOPIC:
            case JOB_DESCRIBE_TOPIC:
                return GET_OPERATION;
            case JOB_START_NEXT_TOPIC:
                return START_NEXT_OPERATION;
            case JOB_WILDCARD_TOPIC:
                return WILDCARD_OPERATION;
            case JOB_UNRECOGNIZED_TOPIC:
            default:
                return "";
        }
    };

    const util::String Jobs::GetSuffixForTopicType(JobExecutionTopicReplyType replyType) {
        switch (replyType) {
            case JOB_REQUEST_TYPE:
                return "";
            case JOB_ACCEPTED_REPLY_TYPE:
                return "/" ACCEPTED_REPLY;
            case JOB_REJECTED_REPLY_TYPE:
                return "/" REJECTED_REPLY;
            case JOB_WILDCARD_REPLY_TYPE:
                return "/" WILDCARD_REPLY;
            case JOB_UNRECOGNIZED_TOPIC_TYPE:
            default:
                return "";
        }
    }

    const util::String Jobs::GetExecutionStatus(JobExecutionStatus status) {
        switch (status) {
            case JOB_EXECUTION_QUEUED:
                return "QUEUED";
            case JOB_EXECUTION_IN_PROGRESS:
                return "IN_PROGRESS";
            case JOB_EXECUTION_FAILED:
                return "FAILED";
            case JOB_EXECUTION_SUCCEEDED:
                return "SUCCEEDED";
            case JOB_EXECUTION_CANCELED:
                return "CANCELED";
            case JOB_EXECUTION_REJECTED:
                return "REJECTED";
            case JOB_EXECUTION_STATUS_NOT_SET:
            case JOB_EXECUTION_UNKNOWN_STATUS:
            default:
                return "";
        }
    }

    util::String Jobs::Escape(const util::String &value) {
        util::String result = "";

        for (int i = 0; i < value.length(); i++) {
            switch(value[i]) {
                case '\n':  result += "\\n";    break;
                case '\r':  result += "\\r";    break;
                case '\t':  result += "\\t";    break;
                case '"':   result += "\\\"";   break;
                case '\\':  result += "\\\\";   break;
                default:    result += value[i];
            }
        }
        return result;
    }

    util::String Jobs::SerializeStatusDetails(const util::Map<util::String, util::String> &statusDetailsMap) {
        util::String result = "{";

        util::Map<util::String, util::String>::const_iterator itr = statusDetailsMap.begin();
        while (itr != statusDetailsMap.end()) {
            result += (itr == statusDetailsMap.begin() ? "\"" : ",\"");
            result += Escape(itr->first) + "\":\"" + Escape(itr->second) + "\"";
            itr++;
        }

        result += '}';
        return result;
    }

    std::unique_ptr<Utf8String> Jobs::GetJobTopic(JobExecutionTopicType topicType,
                                                  JobExecutionTopicReplyType replyType,
                                                  const util::String &jobId) {
        if (thing_name_.empty()) {
            return nullptr;
        }

        if ((topicType == JOB_NOTIFY_TOPIC || topicType == JOB_NOTIFY_NEXT_TOPIC) && replyType != JOB_REQUEST_TYPE) {
            return nullptr;
        }

        if ((topicType == JOB_GET_PENDING_TOPIC || topicType == JOB_START_NEXT_TOPIC ||
            topicType == JOB_NOTIFY_TOPIC || topicType == JOB_NOTIFY_NEXT_TOPIC) && !jobId.empty()) {
            return nullptr;
        }

        const bool requireJobId = BaseTopicRequiresJobId(topicType);
        if (jobId.empty() && requireJobId) {
            return nullptr;
        }

        const util::String operation = GetOperationForBaseTopic(topicType);
        if (operation.empty()) {
            return nullptr;
        }

        const util::String suffix = GetSuffixForTopicType(replyType);

        if (requireJobId) {
            return Utf8String::Create(BASE_THINGS_TOPIC + thing_name_ + "/jobs/" + jobId + '/' + operation + suffix);
        } else if (topicType == JOB_WILDCARD_TOPIC) {
            return Utf8String::Create(BASE_THINGS_TOPIC + thing_name_ + "/jobs/#");
        } else {
            return Utf8String::Create(BASE_THINGS_TOPIC + thing_name_ + "/jobs/" + operation + suffix);
        }
    };

    util::String Jobs::SerializeJobExecutionUpdatePayload(JobExecutionStatus status,
                                                          const util::Map<util::String, util::String> &statusDetailsMap,
                                                          int64_t expectedVersion,    // set to 0 to ignore
                                                          int64_t executionNumber,    // set to 0 to ignore
                                                          bool includeJobExecutionState,
                                                          bool includeJobDocument) {
        const util::String executionStatus = GetExecutionStatus(status);

        if (executionStatus.empty()) {
            return "";
        }

        util::String result = "{\"status\":\"" + executionStatus + "\"";
        if (!statusDetailsMap.empty()) {
            result += ",\"statusDetails\":" + SerializeStatusDetails(statusDetailsMap);
        }
        if (expectedVersion > 0) {
            result += ",\"expectedVersion\":\"" + std::to_string(expectedVersion) + "\"";
        }
        if (executionNumber > 0) {
            result += ",\"executionNumber\":\"" + std::to_string(executionNumber) + "\"";
        }
        if (includeJobExecutionState) {
            result += ",\"includeJobExecutionState\":\"true\"";
        }
        if (includeJobDocument) {
            result += ",\"includeJobDocument\":\"true\"";
        }
        if (!client_token_.empty()) {
            result += ",\"clientToken\":\"" + client_token_ + "\"";
        }
        result += '}';

        return result;
    };

    util::String Jobs::SerializeDescribeJobExecutionPayload(int64_t executionNumber,    // set to 0 to ignore
                                                            bool includeJobDocument) {
        util::String result = "{\"includeJobDocument\":\"";
        result += (includeJobDocument ? "true" : "false");
        result += "\"";
        if (executionNumber > 0) {
            result += ",\"executionNumber\":\"" + std::to_string(executionNumber) + "\"";
        }
        if (!client_token_.empty()) {
            result += "\"clientToken\":\"" + client_token_ + "\"";
        }
        result += '}';

        return result;
    };

    util::String Jobs::SerializeStartNextPendingJobExecutionPayload(const util::Map<util::String, util::String> &statusDetailsMap) {
        util::String result = "{";
        if (!statusDetailsMap.empty()) {
            result += "\"statusDetails\":" + SerializeStatusDetails(statusDetailsMap);
        }
        if (!client_token_.empty()) {
            if (!statusDetailsMap.empty()) {
                result += ',';
            }
            result += "\"clientToken\":\"" + client_token_ + "\"";
        }
        result += '}';

        return result;
    };

    util::String Jobs::SerializeClientTokenPayload() {
        if (!client_token_.empty()) {
            return "{\"clientToken\":\"" + client_token_ + "\"}";
        }

        return "{}";
    };

    ResponseCode Jobs::SendJobsQuery(JobExecutionTopicType topicType,
                                     const util::String &jobId) {
        uint16_t packet_id = 0;
        std::unique_ptr<Utf8String> jobTopic = GetJobTopic(topicType, JOB_REQUEST_TYPE, jobId);

        if (jobTopic == nullptr) {
            return ResponseCode::JOBS_INVALID_TOPIC_ERROR;
        }

        return p_mqtt_client_->PublishAsync(std::move(jobTopic), false, false, qos_, SerializeClientTokenPayload(), nullptr, packet_id);
    };

    ResponseCode Jobs::SendJobsStartNext(const util::Map<util::String, util::String> &statusDetailsMap) {
        uint16_t packet_id = 0;
        std::unique_ptr<Utf8String> jobTopic = GetJobTopic(JOB_START_NEXT_TOPIC, JOB_REQUEST_TYPE);

        if (jobTopic == nullptr) {
            return ResponseCode::JOBS_INVALID_TOPIC_ERROR;
        }

        return p_mqtt_client_->PublishAsync(std::move(jobTopic), false, false, qos_, SerializeStartNextPendingJobExecutionPayload(statusDetailsMap), nullptr, packet_id);
    };

    ResponseCode Jobs::SendJobsDescribe(const util::String &jobId,
                                        int64_t executionNumber,    // set to 0 to ignore
                                        bool includeJobDocument) {
        uint16_t packet_id = 0;
        std::unique_ptr<Utf8String> jobTopic = GetJobTopic(JOB_DESCRIBE_TOPIC, JOB_REQUEST_TYPE, jobId);

        if (jobTopic == nullptr) {
            return ResponseCode::JOBS_INVALID_TOPIC_ERROR;
        }

        return p_mqtt_client_->PublishAsync(std::move(jobTopic), false, false, qos_, SerializeDescribeJobExecutionPayload(executionNumber, includeJobDocument), nullptr, packet_id);
    };

    ResponseCode Jobs::SendJobsUpdate(const util::String &jobId,
                                      JobExecutionStatus status,
                                      const util::Map<util::String, util::String> &statusDetailsMap,
                                      int64_t expectedVersion,    // set to 0 to ignore
                                      int64_t executionNumber,    // set to 0 to ignore
                                      bool includeJobExecutionState,
                                      bool includeJobDocument) {
        uint16_t packet_id = 0;
        std::unique_ptr<Utf8String> jobTopic = GetJobTopic(JOB_UPDATE_TOPIC, JOB_REQUEST_TYPE, jobId);

        if (jobTopic == nullptr) {
            return ResponseCode::JOBS_INVALID_TOPIC_ERROR;
        }

        return p_mqtt_client_->PublishAsync(std::move(jobTopic), false, false, qos_,
                                            SerializeJobExecutionUpdatePayload(status, statusDetailsMap, expectedVersion, executionNumber,
                                                                               includeJobExecutionState, includeJobDocument),
                                            nullptr, packet_id);
    };

    std::shared_ptr<mqtt::Subscription> Jobs::CreateJobsSubscription(mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler,
                                                                     std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data,
                                                                     JobExecutionTopicType topicType,
                                                                     JobExecutionTopicReplyType replyType,
                                                                     const util::String &jobId) {
        return mqtt::Subscription::Create(GetJobTopic(topicType, replyType, jobId), qos_, p_app_handler, p_app_handler_data);
    };
}
