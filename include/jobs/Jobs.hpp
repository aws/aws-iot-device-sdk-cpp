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
 * @file Jobs.hpp
 * @brief
 *
 */

#pragma once

#include "mqtt/Client.hpp"

namespace awsiotsdk {
    class Jobs {
    public:
        // Disabling default and copy constructors.
        Jobs() = delete;                           // Delete Default constructor
        Jobs(const Jobs &) = delete;               // Delete Copy constructor
        Jobs(Jobs &&) = default;                   // Default Move constructor
        Jobs &operator=(const Jobs &) & = delete;  // Delete Copy assignment operator
        Jobs &operator=(Jobs &&) & = default;      // Default Move assignment operator

       /**
        * @brief Create factory method. Returns a unique instance of Jobs
        *
        * @param p_mqtt_client - mqtt client
        * @param qos - QoS
        * @param thing_name - Thing name
        * @param client_token - Client token for correlating messages (optional)
        *
        * @return std::unique_ptr<Jobs> pointing to a unique Jobs instance
        */
        static std::unique_ptr<Jobs> Create(std::shared_ptr<MqttClient> p_mqtt_client,
                                            mqtt::QoS qos,
                                            const util::String &thing_name,
                                            const util::String &client_token = util::String());

        enum JobExecutionTopicType {
            JOB_UNRECOGNIZED_TOPIC = 0,
            JOB_GET_PENDING_TOPIC,
            JOB_START_NEXT_TOPIC,
            JOB_DESCRIBE_TOPIC,
            JOB_UPDATE_TOPIC,
            JOB_NOTIFY_TOPIC,
            JOB_NOTIFY_NEXT_TOPIC,
            JOB_WILDCARD_TOPIC
        };

        enum JobExecutionTopicReplyType {
            JOB_UNRECOGNIZED_TOPIC_TYPE = 0,
            JOB_REQUEST_TYPE,
            JOB_ACCEPTED_REPLY_TYPE,
            JOB_REJECTED_REPLY_TYPE,
            JOB_WILDCARD_REPLY_TYPE
        };

        enum JobExecutionStatus {
            JOB_EXECUTION_STATUS_NOT_SET = 0,
            JOB_EXECUTION_QUEUED,
            JOB_EXECUTION_IN_PROGRESS,
            JOB_EXECUTION_FAILED,
            JOB_EXECUTION_SUCCEEDED,
            JOB_EXECUTION_CANCELED,
            JOB_EXECUTION_REJECTED,
            /***
             * Used for any status not in the supported list of statuses
             */
            JOB_EXECUTION_UNKNOWN_STATUS = 99
        };

       /**
        * @brief GetJobTopic
        *
        * This function creates a job topic based on the provided parameters.
        *
        * @param topicType - Jobs topic type
        * @param replyType - Topic reply type (optional)
        * @param jobId - Job id, can be $next to indicate next queued or in process job, can also be omitted if N/A
        *
        * @return nullptr on error, unique_ptr pointing to a topic string if successful
        */
        std::unique_ptr<Utf8String> GetJobTopic(JobExecutionTopicType topicType,
                                                JobExecutionTopicReplyType replyType = JOB_REQUEST_TYPE,
                                                const util::String &jobId = util::String());

       /**
        * @brief SendJobsQuery
        *
        * Send a query to the Jobs service using the provided mqtt client
        *
        * @param topicType - Jobs topic type for type of query
        * @param jobId - Job id, can be $next to indicate next queued or in process job, can also be omitted if N/A
        *
        * @return ResponseCode indicating status of publish request
        */
        ResponseCode SendJobsQuery(JobExecutionTopicType topicType,
                                   const util::String &jobId = util::String());

       /**
        * @brief SendJobsStartNext
        *
        * Call Jobs start-next API to start the next pending job execution and trigger response
        *
        * @param statusDetails - Status details to be associated with started job execution (optional)
        *
        * @return ResponseCode indicating status of publish request
        */
        ResponseCode SendJobsStartNext(const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>());

       /**
        * @brief SendJobsDescribe
        *
        * Send request for job execution details
        *
        * @param jobId - Job id, can be $next to indicate next queued or in process job, can also
        *                be omitted to request all pending and in progress job executions
        * @param executionNumber - Specific execution number to describe, omit to match latest
        * @param includeJobDocument - Flag to indicate whether response should include job document
        *
        * @return ResponseCode indicating status of publish request
        */
        ResponseCode SendJobsDescribe(const util::String &jobId = util::String(),
                                      int64_t executionNumber = 0,    // set to 0 to ignore
                                      bool includeJobDocument = true);

       /**
        * @brief SendJobsUpdate
        *
        * Send update for specified job
        *
        * @param jobId - Job id associated with job execution to be updated
        * @param status - New job execution status
        * @param statusDetailsMap - Status details to be associated with job execution (optional)
        * @param expectedVersion - Optional expected current job execution number, error response if mismatched
        * @param executionNumber - Specific execution number to update, omit to match latest
        * @param includeJobExecutionState - Include job execution state in response (optional)
        * @param includeJobDocument - Include job document in response (optional)
        *
        * @return ResponseCode indicating status of publish request
        */
        ResponseCode SendJobsUpdate(const util::String &jobId,
                                    JobExecutionStatus status,
                                    const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>(),
                                    int64_t expectedVersion = 0,    // set to 0 to ignore
                                    int64_t executionNumber = 0,    // set to 0 to ignore
                                    bool includeJobExecutionState = false,
                                    bool includeJobDocument = false);

       /**
        * @brief CreateJobsSubscription
        *
        * Create a Jobs Subscription instance
        *
        * @param p_app_handler - Application Handler instance
        * @param p_app_handler_data - Data to be passed to application handler. Can be nullptr
        * @param topicType - Jobs topic type to subscribe to (defaults to JOB_WILDCARD_TOPIC)
        * @param jobId - Job id, can be $next to indicate next queued or in process job, can also be omitted if N/A
        * @param replyType - Topic reply type (optional, defaults to JOB_REQUEST_TYPE which omits the reply type in the subscription)
        *
        * @return shared_ptr Subscription instance
        */
        std::shared_ptr<mqtt::Subscription> CreateJobsSubscription(mqtt::Subscription::ApplicationCallbackHandlerPtr p_app_handler,
                                                                   std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data,
                                                                   JobExecutionTopicType topicType = JOB_WILDCARD_TOPIC,
                                                                   JobExecutionTopicReplyType replyType = JOB_REQUEST_TYPE,
                                                                   const util::String &jobId = util::String());
    protected:
        std::shared_ptr<MqttClient> p_mqtt_client_;
        mqtt::QoS qos_;
        util::String thing_name_;
        util::String client_token_;

       /**
        * @brief Jobs constructor
        *
        * Create Jobs object storing given parameters in created instance
        *
        * @param p_mqtt_client - mqtt client
        * @param qos - QoS
        * @param thing_name - Thing name
        * @param client_token - Client token for correlating messages (optional)
        */
        Jobs(std::shared_ptr<MqttClient> p_mqtt_client,
             mqtt::QoS qos,
             const util::String &thing_name,
             const util::String &client_token);

        static bool BaseTopicRequiresJobId(JobExecutionTopicType topicType);
        static const util::String GetOperationForBaseTopic(JobExecutionTopicType topicType);
        static const util::String GetSuffixForTopicType(JobExecutionTopicReplyType replyType);
        static const util::String GetExecutionStatus(JobExecutionStatus status);
        static util::String Escape(const util::String &value);
        static util::String SerializeStatusDetails(const util::Map<util::String, util::String> &statusDetailsMap);

        util::String SerializeJobExecutionUpdatePayload(JobExecutionStatus status,
                                                        const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>(),
                                                        int64_t expectedVersion = 0,
                                                        int64_t executionNumber = 0,
                                                        bool includeJobExecutionState = false,
                                                        bool includeJobDocument = false);
        util::String SerializeDescribeJobExecutionPayload(int64_t executionNumber = 0,
                                                          bool includeJobDocument = true);
        util::String SerializeStartNextPendingJobExecutionPayload(const util::Map<util::String, util::String> &statusDetailsMap = util::Map<util::String, util::String>());
        util::String SerializeClientTokenPayload();
    };
}
