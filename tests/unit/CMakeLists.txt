cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(aws-iot-cpp-unit-tests CXX)
add_definitions(-DUNIT_TESTS)

######################################
# Section : Disable in-source builds #
######################################

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles folder.")
endif ()

########################################
# Section : Common SDK Build setttings #
########################################

# Set required compiler standard to standard c++11. Disable extensions.
set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archive)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Configure Compiler flags
if (UNIX AND NOT APPLE)
    # Prefer pthread if found
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    set(CUSTOM_COMPILER_FLAGS "-fno-exceptions -Wall -Werror")
elseif (APPLE)
    set(CUSTOM_COMPILER_FLAGS "-fno-exceptions -Wall -Werror")
elseif (WIN32)
    set(CUSTOM_COMPILER_FLAGS "/W4")
endif ()

if(SANITIZE_THREAD OR SANITIZE_MEMORY OR SANITIZE_ADDRESS OR SANITIZE_MEMORY)
    set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/third_party/sanitizers/src/cmake" ${CMAKE_MODULE_PATH})
    find_package(Sanitizers)
endif()
 
#############################
# Add SDK Testing Target #
#############################
enable_testing()
set(UNIT_TEST_TARGET_NAME aws-iot-unit-tests)
add_executable(${UNIT_TEST_TARGET_NAME} "${PROJECT_SOURCE_DIR}/../../samples/JobsAgent/JobsAgentOperations.cpp;${PROJECT_SOURCE_DIR}/../../common/ConfigCommon.cpp")
# add_executable(${UNIT_TEST_TARGET_NAME} "${PROJECT_SOURCE_DIR}/../../common/ConfigCommon.cpp")

target_include_directories(${SDK_TARGET_NAME} PUBLIC ${CMAKE_BINARY_DIR}/third_party/rapidjson/src/include)
target_include_directories(${SDK_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
# target_include_directories(${SDK_TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../samples/JobsAgent)

# Configure Threading library
find_package(Threads REQUIRED)
target_link_libraries(${UNIT_TEST_TARGET_NAME} "Threads::Threads")

# Configure CURL library
find_package(CURL REQUIRED)
target_link_libraries(${UNIT_TEST_TARGET_NAME} ${CURL_LIBRARIES})

# Download and unpack googletest at configure time
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeLists-gtest.txt.in
        ${CMAKE_BINARY_DIR}/third_party/googletest/download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third_party/googletest/download)

execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third_party/googletest/download)

# Prevent GoogleTest from overriding compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# This adds the following targets: gtest, gtest_main, gmock and gmock_main
add_subdirectory(${CMAKE_BINARY_DIR}/third_party/googletest/src
        ${CMAKE_BINARY_DIR}/third_party/googletest/build EXCLUDE_FROM_ALL)

file(GLOB_RECURSE SDK_UNIT_TEST_SOURCES FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/tests/unit/src/*.cpp)
target_sources(${UNIT_TEST_TARGET_NAME} PUBLIC ${SDK_UNIT_TEST_SOURCES})
target_include_directories(${UNIT_TEST_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(${UNIT_TEST_TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/tests/unit/include)
target_link_libraries(${UNIT_TEST_TARGET_NAME} gtest gtest_main gmock gmock_main)
target_link_libraries(${UNIT_TEST_TARGET_NAME} ${THREAD_LIBRARY_LINK_STRING})
target_link_libraries(${UNIT_TEST_TARGET_NAME} ${SDK_TARGET_NAME})

if(SANITIZE_THREAD OR SANITIZE_MEMORY OR SANITIZE_ADDRESS OR SANITIZE_MEMORY)
    add_sanitizers(${UNIT_TEST_TARGET_NAME})
endif()

find_program(GCOV gcov)
find_program(LCOV lcov)
find_program(GENHTML genhtml)

if (GCOV AND LCOV AND GENHTML)
    set(UNIT_TEST_COVERAGE_TARGET_NAME "${UNIT_TEST_TARGET_NAME}-coverage")
    target_link_libraries(${UNIT_TEST_TARGET_NAME} -fprofile-arcs)
    target_link_libraries(${UNIT_TEST_TARGET_NAME} -ftest-coverage)

    set(UNIT_TEST_OUTPUT_DIR_PATH ${CMAKE_BINARY_DIR}/unit_test_results)
    set(UNIT_TEST_OUTPUT_NAME unit_test)

    file(MAKE_DIRECTORY ${UNIT_TEST_OUTPUT_DIR_PATH})

    set(CUSTOM_COMPILER_FLAGS "${CUSTOM_COMPILER_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")

    set(LCOV_INFO "${UNIT_TEST_OUTPUT_DIR_PATH}/${UNIT_TEST_OUTPUT_NAME}.info")
    set(LCOV_CLEAN "${LCOV_INFO}.clean")

    # Setup target
    add_custom_target(${UNIT_TEST_COVERAGE_TARGET_NAME}
            DEPENDS ${UNIT_TEST_TARGET_NAME}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            # Cleanup old lcov runs
            COMMAND ${LCOV} --directory ${UNIT_TEST_OUTPUT_DIR_PATH} --zerocounters --rc lcov_branch_coverage=1

            # Run tests
            COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${UNIT_TEST_TARGET_NAME}

            # Capturing lcov counters and generating report
            COMMAND ${LCOV} --rc lcov_branch_coverage=1 --directory ${CMAKE_BINARY_DIR} --capture --output-file ${LCOV_INFO}
            COMMAND ${LCOV} --rc lcov_branch_coverage=1 --remove ${LCOV_INFO} 'tests/*' 'build/*' '/usr/*' '*logging/*' --output-file ${LCOV_CLEAN}
            COMMAND ${GENHTML} -o ${UNIT_TEST_OUTPUT_NAME} ${LCOV_CLEAN}
            COMMAND ${CMAKE_COMMAND} -E remove ${LCOV_INFO} ${LCOV_CLEAN}
            COMMENT "Open ${CMAKE_BINARY_DIR}/${UNIT_TEST_OUTPUT_NAME}/index.html in your browser to view the coverage report."
            )
endif ()

add_custom_command(TARGET ${UNIT_TEST_TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        copy ${CMAKE_SOURCE_DIR}/tests/unit/src/util/TestParser.json $<TARGET_FILE_DIR:${UNIT_TEST_TARGET_NAME}>/TestParser.json)

set_property(TARGET ${UNIT_TEST_TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${CUSTOM_COMPILER_FLAGS})

# Enable 'make test'
add_test(NAME Run-All-Tests COMMAND ${UNIT_TEST_TARGET_NAME})
