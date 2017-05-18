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
 * @file ProxyType.hpp
 * @brief Strongly typed enumeration of available proxy types used by the SDK
 *
 * Contains the proxy types used by the SDK
 */

#pragma once

namespace awsiotsdk {
	/**
	 * @brief Proxy Type enum class
	 *
	 * Strongly typed enumeration of available proxy types used by the SDK
	 */
	enum class ProxyType {
		NONE = 0,
		HTTP = 1
	};
}
