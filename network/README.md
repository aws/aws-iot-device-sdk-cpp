## Network Layer Implementation Guide

### Design
The Network interface for the SDK has been designed to give control of how the Network is initialized to the application. The SDK provides a base class for [Network Connection](https://github.com/aws/aws-iot-device-sdk-cpp/blob/master/include/NetworkConnection.hpp). The SDK itself interacts with shared_ptr instances of Concrete implementations of this base class.

The SDK will only ever call the functions that are defined in the NetworkConnection base class as public members. The only exception to this is the Destroy function. Further description of each function's purpose is below. Each Network function should return a certain set of Response Codes that the SDK will understand and process. All other codes are treated as unknown and a disconnect occurs.

### APIs
The Network Base class defines the following functions that are a Concrete implementation is required to provide:
 
 * virtual bool IsConnected() - Pure virtual function, public, directly called by SDK, implementation should return the state of the underlying Network Connection 	 
 * virtual bool IsPhysicalLayerConnected() - Pure virtual function, public, directly called by SDK, implementation should return the state of the Network itself. Used to determine if a connection attempt is even possible at this time. This is an optional call and provided for convenience. The reference implementations simply returns true
 * virtual ResponseCode ConnectInternal() - Pure virtual function, Protected, Not called by SDK directly. This function should contain the Connect implementation. It will also be used for auto-reconnect
 * virtual ResponseCode WriteInternal(const util::String &buf, size_t &size_written_bytes_out) - Pure virtual function, Protected, Not called by SDK directly. This function function is used for Write operations.
 * virtual ResponseCode ReadInternal(util::Vector<unsigned char> &buf, size_t buf_read_offset, size_t size_bytes_to_read, size_t &size_read_bytes_out) - Pure virtual function, Protected, Not called by SDK directly. This function is used for Read operations. The buffer is resized to size_bytes_to_read before it is passed to the function. The size of the buffer should not be updated before returning.
 * virtual ResponseCode DisconnectInternal() - Pure virtual function, Protected, Not called by SDK directly. This function should Disconnect the TLS layer but should not destroy the instance. The SDK expects to be able to call Connect afterwards to perform a Reconnect if required. For complete cleanup, please use destructor

It also defines the following functions that are called by the SDK:
 * virtual ResponseCode Connect() final - Final function. Implementation in base class blocks on obtaining read and write locks. Calls ConnectInternal when successful.
 * virtual ResponseCode Write(const util::String &buf, size_t &size_written_bytes_out) final - Final function. Implementation in base class blocks on obtaining write lock. It then verifies if the Network is Connected and if it is, calls WriteInternal.
 * virtual ResponseCode Read(util::Vector<unsigned char> &buf, size_t buf_read_offset, size_t size_bytes_to_read, size_t &size_read_bytes_out) final - Final function. Implementation in base class blocks on obtaining read lock. It then verifies if the Network is Connected and if it is, calls ReadInternal.
 * virtual ResponseCode Disconnect() final - Final function. Checks if Network is connected. Returns error if it isn't. Calls DisconnectInternal if connected.

### Response Codes
The [ResponseCode](https://github.com/aws/aws-iot-device-sdk-cpp/blob/master/include/ResponseCode.hpp) enum class contains strongly typed Response Codes used by the SDK. They are divided into sections. The NetworkConnection implementations are expected to return response codes defined in the below sections
 
 * Network Success Codes (Values in the +200 range)
 * Generic Response Codes (Values in the 0 - 99 range). Please be aware that anything except SUCCESS here would be treated as a reason to Reconnect or Failed request as appropriate
 * Network TCP Error Codes (Values in the 300-399 range)
 * Network SSL Error Codes (Values in the 400-499 range)
 * Network Generic Error Codes (Values in the 500-599 range)
 
Any unexpected behavior is always treated as an issue with the Network Connection at the moment and a Reconnect occurs.

### Thread Safety
Since the SDK itself cannot guarantee thread safety within the Network libraries that are being used, we apply mutex guards at the Base class level. Only one read and one write request can be in progress at a time.

### ALPN
AWS IoT supports connections using MQTT over TLS on port 443. This requires that ALPN support be enabled in the TLS layer. The provided reference network layers for MbedTLS and OpenSSL provide an additional constructor that allows enabling ALPN when port 443 is being used. The MBEDTLS_SSL_ALPN macro should be uncommented (which it is by default) in MbedTLS [config.h](https://github.com/ARMmbed/mbedtls/blob/development/include/mbedtls/config.h) to enable ALPN.
