## Samples
The SDK comes with various samples that demonstrate how various SDK features can be used. A list of the included samples is below. For all of the samples listed, a target is generated when the build process described in the README.md file is followed. Specific targets are mentioned along with the sample description. Each sample can also be used with any of the provided reference wrappers by specifying the corresponding argument in the CMake call:
 * OpenSSL (Default if no argument is provided)- `cmake <path_to_sdk>` OR `cmake <path_to_sdk> -DNETWORK_LIBRARY=OpenSSL`
 * MbedTLS - `cmake <path_to_sdk> -DNETWORK_LIBRARY=MbedTLS`
 * WebSocket - `cmake <path_to_sdk> -DNETWORK_LIBRARY=WebSocket`

### Basic Subscribe Publish
This is a basic sample that demonstrates how MQTT Subscribe and Publish operations can be performed using the default MQTT Client.
 * Code for this sample is located [here](./PubSub)
 * Target for this sample is `pub-sub-sample`

### Shadow Delta
This sample demonstrates how various Shadow operations can be performed.
 * Code for this sample is located [here](./ShadowDelta)
 * Target for this sample is `shadow-delta-sample`
 
 
For further information about the provided MQTT and Shadow Classes, please refer to the [Development Guide](../DevGuide.md)