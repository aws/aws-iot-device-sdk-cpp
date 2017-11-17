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
 * @file Utf8String.cpp
 * @brief
 *
 */

#include <rapidjson/encodings.h>
#include <rapidjson/stream.h>
#include <rapidjson/stringbuffer.h>
#include "util/Utf8String.hpp"

namespace awsiotsdk {
    namespace utf8 {
        // The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
        // You may need to change them to match your system.
        // These typedefs have the same names as ones from cstdint, or boost/cstdint
        typedef unsigned char uint8_t;
        typedef unsigned short uint16_t;
        typedef unsigned int uint32_t;

        // Helper code - not intended to be directly called by the library users. May be changed at any time
        namespace internal {
            // Unicode constants
            // Leading (high) surrogates: 0xd800 - 0xdbff
            // Trailing (low) surrogates: 0xdc00 - 0xdfff
            const uint16_t LEAD_SURROGATE_MIN = 0xd800u;
            const uint16_t LEAD_SURROGATE_MAX = 0xdbffu;
            const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
            const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
            const uint16_t LEAD_OFFSET = LEAD_SURROGATE_MIN - (0x10000 >> 10);
            const uint32_t SURROGATE_OFFSET = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

            // Maximum valid value for a Unicode code point
            const uint32_t CODE_POINT_MAX = 0x0010ffffu;

            template<typename octet_type>
            inline uint8_t mask8(octet_type oc) {
                return static_cast<uint8_t>(0xff & oc);
            }
            template<typename u16_type>
            inline uint16_t mask16(u16_type oc) {
                return static_cast<uint16_t>(0xffff & oc);
            }
            template<typename octet_type>
            inline bool is_trail(octet_type oc) {
                return ((utf8::internal::mask8(oc) >> 6) == 0x2);
            }

            template<typename u16>
            inline bool is_lead_surrogate(u16 cp) {
                return (cp >= LEAD_SURROGATE_MIN && cp <= LEAD_SURROGATE_MAX);
            }

            template<typename u16>
            inline bool is_trail_surrogate(u16 cp) {
                return (cp >= TRAIL_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
            }

            template<typename u16>
            inline bool is_surrogate(u16 cp) {
                return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
            }

            template<typename u32>
            inline bool is_code_point_valid(u32 cp) {
                return (cp <= CODE_POINT_MAX && !utf8::internal::is_surrogate(cp));
            }

            template<typename octet_iterator>
            inline typename std::iterator_traits<octet_iterator>::difference_type
            sequence_length(octet_iterator lead_it) {
                uint8_t lead = utf8::internal::mask8(*lead_it);
                if (lead < 0x80)
                    return 1;
                else if ((lead >> 5) == 0x6)
                    return 2;
                else if ((lead >> 4) == 0xe)
                    return 3;
                else if ((lead >> 3) == 0x1e)
                    return 4;
                else
                    return 0;
            }

            template<typename octet_difference_type>
            inline bool is_overlong_sequence(uint32_t cp, octet_difference_type length) {
                if (cp < 0x80) {
                    if (length != 1)
                        return true;
                } else if (cp < 0x800) {
                    if (length != 2)
                        return true;
                } else if (cp < 0x10000) {
                    if (length != 3)
                        return true;
                }

                return false;
            }

            enum utf_error {
                UTF8_OK,
                NOT_ENOUGH_ROOM,
                INVALID_LEAD,
                INCOMPLETE_SEQUENCE,
                OVERLONG_SEQUENCE,
                INVALID_CODE_POINT
            };

            /// Helper for get_sequence_x
            template<typename octet_iterator>
            utf_error increase_safely(octet_iterator &it, octet_iterator end) {
                if (++it == end)
                    return NOT_ENOUGH_ROOM;

                if (!utf8::internal::is_trail(*it))
                    return INCOMPLETE_SEQUENCE;

                return UTF8_OK;
            }

#define UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(IT, END) {utf_error ret = increase_safely(IT, END); if (ret != UTF8_OK) return ret;}

            /// get_sequence_x functions decode utf-8 sequences of the length x
            template<typename octet_iterator>
            utf_error get_sequence_1(octet_iterator &it, octet_iterator end, uint32_t &code_point) {
                if (it == end)
                    return NOT_ENOUGH_ROOM;

                code_point = utf8::internal::mask8(*it);

                return UTF8_OK;
            }

            template<typename octet_iterator>
            utf_error get_sequence_2(octet_iterator &it, octet_iterator end, uint32_t &code_point) {
                if (it == end)
                    return NOT_ENOUGH_ROOM;

                code_point = utf8::internal::mask8(*it);

                UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

                code_point = ((code_point << 6) & 0x7ff) + ((*it) & 0x3f);

                return UTF8_OK;
            }

            template<typename octet_iterator>
            utf_error get_sequence_3(octet_iterator &it, octet_iterator end, uint32_t &code_point) {
                if (it == end)
                    return NOT_ENOUGH_ROOM;

                code_point = utf8::internal::mask8(*it);

                UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

                code_point = ((code_point << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);

                UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

                code_point += (*it) & 0x3f;

                return UTF8_OK;
            }

            template<typename octet_iterator>
            utf_error get_sequence_4(octet_iterator &it, octet_iterator end, uint32_t &code_point) {
                if (it == end)
                    return NOT_ENOUGH_ROOM;

                code_point = utf8::internal::mask8(*it);

                UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

                code_point = ((code_point << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);

                UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

                code_point += (utf8::internal::mask8(*it) << 6) & 0xfff;

                UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)

                code_point += (*it) & 0x3f;

                return UTF8_OK;
            }

#undef UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR

            template<typename octet_iterator>
            utf_error validate_next(octet_iterator &it, octet_iterator end, uint32_t &code_point) {
                // Save the original value of it so we can go back in case of failure
                // Of course, it does not make much sense with i.e. stream iterators
                octet_iterator original_it = it;

                uint32_t cp = 0;
                // Determine the sequence length based on the lead octet
                typedef typename std::iterator_traits<octet_iterator>::difference_type octet_difference_type;
                const octet_difference_type length = utf8::internal::sequence_length(it);

                // Get trail octets and calculate the code point
                utf_error err = UTF8_OK;
                switch (length) {
                    case 0:
                        return INVALID_LEAD;
                    case 1:
                        err = utf8::internal::get_sequence_1(it, end, cp);
                        break;
                    case 2:
                        err = utf8::internal::get_sequence_2(it, end, cp);
                        break;
                    case 3:
                        err = utf8::internal::get_sequence_3(it, end, cp);
                        break;
                    case 4:
                        err = utf8::internal::get_sequence_4(it, end, cp);
                        break;
                }

                if (err == UTF8_OK) {
                    // Decoding succeeded. Now, security checks...
                    if (utf8::internal::is_code_point_valid(cp)) {
                        if (!utf8::internal::is_overlong_sequence(cp, length)) {
                            // Passed! Return here.
                            code_point = cp;
                            ++it;
                            return UTF8_OK;
                        } else
                            err = OVERLONG_SEQUENCE;
                    } else
                        err = INVALID_CODE_POINT;
                }

                // Failure branch - restore the original value of the iterator
                it = original_it;
                return err;
            }

            template<typename octet_iterator>
            inline utf_error validate_next(octet_iterator &it, octet_iterator end) {
                uint32_t ignored;
                return utf8::internal::validate_next(it, end, ignored);
            }

        } // namespace internal

        /// The library API - functions intended to be called by the users
        template<typename octet_iterator>
        octet_iterator find_invalid(octet_iterator start, octet_iterator end) {
            octet_iterator result = start;
            while (result != end) {
                utf8::internal::utf_error err_code = utf8::internal::validate_next(result, end);
                if (err_code != internal::UTF8_OK)
                    return result;
            }
            return result;
        }

        template<typename octet_iterator>
        inline bool is_valid(octet_iterator start, octet_iterator end) {
            return (utf8::find_invalid(start, end) == end);
        }
    } // namespace utf8

    bool Utf8String::IsValidInput(util::String str) {
        return utf8::is_valid(str.begin(), str.end());
    }

    std::unique_ptr<Utf8String> Utf8String::Create(util::String str) {
        if (!IsValidInput(str)) {
            return nullptr;
        }
        return std::unique_ptr<Utf8String>(new Utf8String(str));
    }

    std::unique_ptr<Utf8String> Utf8String::Create(const char *str, std::size_t length) {
        if (!IsValidInput(util::String(str, length))) {
            return nullptr;
        }
        return std::unique_ptr<Utf8String>(new Utf8String(str, length));
    }

    Utf8String::Utf8String(util::String str) {
        this->data = str;
        this->length = str.length();
    }

    Utf8String::Utf8String(const char *str, std::size_t length) {
        this->data = util::String(str, (size_t) length);
        this->length = length;
    }

    std::size_t Utf8String::Length() {
        return length;
    }

    util::String Utf8String::ToStdString() {
        return data;
    }
}

