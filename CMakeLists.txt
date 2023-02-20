cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(aws-iot-sdk-cpp CXX)
option(BUILD_SHARED_LIBRARY "Build the library as a shared object, will build it as a static library otherwise." OFF)
option(BUILD_CLI "Build the cli." ON)
option(BUILD_SAMPLES "Build the samples." ON)
option(BUILD_TESTS "Build the tests." ON)
option(BUILD_DOCS "Create HTML based API documentation (requires Doxygen)." OFF)

######################################
# Section : Disable in-source builds #
######################################

if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
	message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles folder." )
endif()

########################################
# Section : Common SDK Build setttings #
########################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Set required compiler standard to standard c++11. Disable extensions.
set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Configure Compiler flags
if(UNIX AND NOT APPLE)
	# Prefer pthread if found
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	set(CUSTOM_COMPILER_FLAGS "-fno-exceptions -Wall -Werror")
elseif(APPLE)
    set(CUSTOM_COMPILER_FLAGS "-fno-exceptions -Werror -Wall")
elseif(WIN32)
	set(CUSTOM_COMPILER_FLAGS "/W4")
endif()

if(NOT DEPENDENCY_DIR)
    set(DEPENDENCY_DIR "third_party")
endif()

#############################
# Add SDK Target #
#############################
add_definitions(-DSDK_VERSION_STRING="1.4.0")

# Create library file. The Client applications MUST link to below target if using this CMake file
set(SDK_TARGET_NAME aws-iot-sdk-cpp)
if (BUILD_SHARED_LIBRARY)
    add_library(${SDK_TARGET_NAME} SHARED "")
    set_target_properties(${SDK_TARGET_NAME} PROPERTIES SUFFIX ".so")
else()
    add_library(${SDK_TARGET_NAME} "")
    set_target_properties(${SDK_TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Download and include rapidjson, not optional
configure_file(CMakeLists-rapidjson.txt.in ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/download)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/download)
target_include_directories(${SDK_TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/src/include)

# Download sanitizers, optional
if (SANITIZE_THREAD OR SANITIZE_ADDRESS OR SANITIZE_MEMORY OR SANITIZE_UNDEFINED)
    configure_file(CMakeLists-sanitizers.txt.in ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/sanitizers/download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/sanitizers/download)
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/sanitizers/download)
endif()

# Get Common SDK Sources
file(GLOB_RECURSE SDK_SOURCES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.cpp)
target_include_directories(${SDK_TARGET_NAME} PRIVATE  ${PROJECT_SOURCE_DIR}/include)
target_sources(${SDK_TARGET_NAME} PRIVATE ${SDK_SOURCES})

# Configure Threading library
find_package(Threads REQUIRED)
set(THREAD_LIBRARY_LINK_STRING "Threads::Threads" -ldl)
target_link_libraries(${SDK_TARGET_NAME} "Threads::Threads" -ldl)

if(APPLE)
	target_link_libraries(${SDK_TARGET_NAME} -lresolv)
endif()

if(BUILD_DOCS)
	find_package(Doxygen REQUIRED)
	file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/docs)
	set(DOC_BINARY_DIR ${PROJECT_BINARY_DIR}/docs)
	set(DOC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/.)

	set(doxygen_conf_in ${PROJECT_SOURCE_DIR}/doxygen/doxygen.conf.in)
	set(doxygen_conf ${PROJECT_BINARY_DIR}/doxygen/doxygen.conf)

	configure_file(${doxygen_conf_in} ${doxygen_conf} @ONLY)

	add_custom_target(generate-sdk-docs
			COMMAND ${DOXYGEN_EXECUTABLE} ${doxygen_conf}
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/docs
			COMMENT "Generating SDK docs"
			VERBATIM)
endif()

################################################
# Set up directory structure for Visual Studio #
################################################
# TODO : Figure out a better way of handling Visual Studio solutions
if(MSVC)
	file(GLOB SDK_COMMON_HEADERS "${PROJECT_SOURCE_DIR}/include/*.hpp")
	file(GLOB SDK_UTIL_COMMON_HEADERS "${PROJECT_SOURCE_DIR}/include/util/*.hpp")
	file(GLOB SDK_UTIL_LOGGING_HEADERS "${PROJECT_SOURCE_DIR}/include/util/logging/*.hpp")
	file(GLOB SDK_UTIL_MEMORY_STL_HEADERS "${PROJECT_SOURCE_DIR}/include/util/memory/stl/*.hpp")
	file(GLOB SDK_UTIL_THREADING_HEADERS "${PROJECT_SOURCE_DIR}/include/util/threading/*.hpp")
	file(GLOB SDK_MQTT_HEADERS "${PROJECT_SOURCE_DIR}/include/mqtt/*.hpp")
	file(GLOB SDK_SHADOW_HEADERS "${PROJECT_SOURCE_DIR}/include/shadow/*.hpp")
	file(GLOB SDK_DISCOVERY_HEADERS "${PROJECT_SOURCE_DIR}/include/discovery/*.hpp")

	file(GLOB SDK_COMMON_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
	file(GLOB SDK_UTIL_COMMON_SOURCES "${PROJECT_SOURCE_DIR}/src/util/*.cpp")
	file(GLOB SDK_UTIL_LOGGING_SOURCES "${PROJECT_SOURCE_DIR}/src/util/logging/*.cpp")
	file(GLOB SDK_UTIL_THREADING_SOURCES "${PROJECT_SOURCE_DIR}/src/util/threading/*.cpp")
	file(GLOB SDK_MQTT_SOURCES "${PROJECT_SOURCE_DIR}/src/mqtt/*.cpp")
	file(GLOB SDK_SHADOW_SOURCES "${PROJECT_SOURCE_DIR}/src/shadow/*.cpp")
	file(GLOB SDK_DISCOVERY_SOURCES "${PROJECT_SOURCE_DIR}/src/discovery/*.cpp")

	# Required to make Header files visible in Visual Studio
	file(GLOB_RECURSE SDKHeaders FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/*.hpp)
	target_sources(${SDK_TARGET_NAME} PUBLIC ${SDKHeaders})

	source_group("Header Files\\aws-iot" FILES ${SDK_COMMON_HEADERS})
	source_group("Header Files\\aws-iot\\util" FILES ${SDK_UTIL_COMMON_HEADERS})
	source_group("Header Files\\aws-iot\\util\\logging" FILES ${SDK_UTIL_LOGGING_HEADERS})
	source_group("Header Files\\aws-iot\\util\\memory\\stl" FILES ${SDK_UTIL_MEMORY_STL_HEADERS})
	source_group("Header Files\\aws-iot\\util\\threading" FILES ${SDK_UTIL_THREADING_HEADERS})
	source_group("Header Files\\aws-iot\\mqtt" FILES ${SDK_MQTT_HEADERS})
	source_group("Header Files\\aws-iot\\shadow" FILES ${SDK_SHADOW_HEADERS})
	source_group("Header Files\\aws-iot\\discovery" FILES ${SDK_DISCOVERY_HEADERS})

	source_group("Source Files\\aws-iot" FILES ${SDK_COMMON_SOURCES})
	source_group("Source Files\\aws-iot\\util" FILES ${SDK_UTIL_COMMON_SOURCES})
	source_group("Source Files\\aws-iot\\util\\logging" FILES ${SDK_UTIL_LOGGING_SOURCES})
	source_group("Source Files\\aws-iot\\util\\threading" FILES ${SDK_UTIL_THREADING_SOURCES})
	source_group("Source Files\\aws-iot\\mqtt" FILES ${SDK_MQTT_SOURCES})
	source_group("Source Files\\aws-iot\\shadow" FILES ${SDK_SHADOW_SOURCES})
	source_group("Source Files\\aws-iot\\discovery" FILES ${SDK_DISCOVERY_SOURCES})
endif()

#####################################
# Section : Define specific Targets #
#####################################

if(BUILD_CLI AND UNIX)
	add_subdirectory(cli)
endif()

if(BUILD_TESTS)
	add_subdirectory(tests/integration)
	add_subdirectory(tests/unit)
endif()

if(BUILD_SAMPLES)
	add_subdirectory(samples/PubSub)
	add_subdirectory(samples/ShadowDelta)
	add_subdirectory(samples/Jobs)
	add_subdirectory(samples/JobsAgent)
	add_subdirectory(samples/Discovery EXCLUDE_FROM_ALL)
	add_subdirectory(samples/StoryRobotArm EXCLUDE_FROM_ALL)
	add_subdirectory(samples/StorySwitch EXCLUDE_FROM_ALL)
endif()

##################################
# Section: Define Install Target #
##################################
if(NOT MSVC)
    install(TARGETS aws-iot-sdk-cpp DESTINATION lib)
    install(DIRECTORY include/ DESTINATION include)
endif()

##########################
# Section: Print Summary #
##########################
include(FeatureSummary)

add_feature_info(SharedLibrary BUILD_SHARED_LIBRARY "build the library as shared objects")
add_feature_info(Cli BUILD_CLI "the command line interface")
add_feature_info(Samples BUILD_SAMPLES "example programs making use of the SDK")
add_feature_info(Tests BUILD_TESTS "unit and integration tests")
add_feature_info(Docs BUILD_DOCS "HTML based API documentation")

feature_summary(WHAT ALL)
