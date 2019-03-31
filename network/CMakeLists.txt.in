cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
######################################
# Section : Disable in-source builds #
######################################

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles folder." )
endif()

set(NETWORK_LIBRARY "OpenSSL" CACHE STRING "Network Library to use")
set_property(CACHE NETWORK_LIBRARY PROPERTY STRINGS OpenSSL MbedTLS WebSocket)

#########################
# Add Network libraries #
#########################
if(${NETWORK_LIBRARY} MATCHES "MbedTLS")
	add_definitions(-DUSE_MBEDTLS)

	if(USE_SYSTEM_MBEDTLS)
		find_package(MbedTLS REQUIRED)
		set(SslLibraryIncludePaths "${MBEDTLS_INCLUDE_DIR};${CMAKE_SOURCE_DIR}/network/MbedTLS")
		target_include_directories(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibraryIncludePaths})
		target_link_libraries(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC "${MBEDTLS_LIBS}")
		file(GLOB_RECURSE SslLibrarySourcePaths FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/network/MbedTLS/*.*)
		target_sources(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibrarySourcePaths})
	else()
		if(NOT DEFINED BUILD_MBEDTLS_ONCE)
			set(BUILD_MBEDTLS_ONCE CACHE INTERNAL "Done")
			# Download and unpack googletest at configure time
			configure_file(${CMAKE_SOURCE_DIR}/network/CMakeLists-mbedtls.txt.in
					${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/download/CMakeLists.txt)

			execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
					WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/download)

			execute_process(COMMAND ${CMAKE_COMMAND} --build .
					WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/download)

			option(ENABLE_TESTING Off)
			add_subdirectory(${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/src
					${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/build)
		endif()

		add_custom_command(TARGET ${NETWORK_WRAPPER_DEST_TARGET} PRE_BUILD
				COMMAND $(MAKE) -C ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/src)
		set(MBEDTLS_LIB_DIR "${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/src/library")
		target_link_libraries(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC mbedtls)
		target_link_libraries(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC mbedcrypto)
		target_link_libraries(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC mbedx509)

		set(SslLibraryIncludePaths "${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/mbedtls/src/include;${CMAKE_SOURCE_DIR}/network/MbedTLS")
		file(GLOB_RECURSE SslLibrarySourcePaths FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/network/MbedTLS/*.*)
		target_include_directories(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibraryIncludePaths})
		target_sources(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibrarySourcePaths})
		if(MSVC)
			file(GLOB_RECURSE SslLibraryHeaders FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/network/MbedTLS/*.hpp)
			file(GLOB_RECURSE SslLibrarySources FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/network/MbedTLS/*.cpp)
			target_sources(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibraryHeaders})
			source_group("Header Files\\network\\OpenSSL" FILES ${SslLibraryHeaders})
			source_group("Source Files\\network\\OpenSSL" FILES ${SslLibrarySources})
		endif()
	endif(USE_SYSTEM_MBEDTLS)
else()
	# Check for required packages (OpenSSL)
	if(NOT APPLE)
		find_package(OpenSSL REQUIRED)
	else()
		# Try using Homebrew's location as a hint
		if(NOT OPENSSL_ROOT_DIR)
			set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
		endif()
		find_package(OpenSSL REQUIRED)
		#if using Mac OS, you can comment find_package(OpenSSL REQUIRED) and uncomment following two lines. Change the path to your openssl 1.0.2 directory.
		#set(OPENSSL_INCLUDE_DIR "YOUR_OPENSSL_PATH/include")
		#set(OPENSSL_LIBRARIES "YOUR_OPENSSL_PATH/lib/libssl.a;YOUR_OPENSSL_PATH/lib/libcrypto.a")
	endif()

	#TODO: Need proper fix for various library configurations
    	if(WIN32)
    		set(library_config "general")
    		foreach(library ${OPENSSL_LIBRARIES})
    			if(library MATCHES "debug|general|optimized")
    				set(library_config ${library})
    			else()
    				if(library_config MATCHES "general|optimized")
    					target_link_libraries(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC "${library}")
    				endif()
    			endif()
    		endforeach()
    	else()
    		target_link_libraries(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC "${OPENSSL_LIBRARIES}")
    	endif()

    	# Default
    	set(SslLibraryIncludePaths "${OPENSSL_INCLUDE_DIR}\;${CMAKE_CURRENT_LIST_DIR}/OpenSSL")

    	file(GLOB_RECURSE SslLibrarySourcePaths FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/OpenSSL/*.*)
    	target_include_directories(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibraryIncludePaths})
    	target_sources(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibrarySourcePaths})
    	if(MSVC)
    		file(GLOB_RECURSE SslLibraryHeaders FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/OpenSSL/*.hpp)
    		file(GLOB_RECURSE SslLibrarySources FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/OpenSSL/*.cpp)
    		target_sources(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibraryHeaders})
    		source_group("Header Files\\network\\OpenSSL" FILES ${SslLibraryHeaders})
    		source_group("Source Files\\network\\OpenSSL" FILES ${SslLibrarySources})
    	endif()

    	if(${NETWORK_LIBRARY} MATCHES "WebSocket")
    		add_definitions(-DUSE_WEBSOCKETS)
    		target_include_directories(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/WebSocket)
    		target_include_directories(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/WebSocket/wslay)

    		set(SslLibraryIncludePaths "${OPENSSL_INCLUDE_DIR};network/WebSocket/;network/WebSocket/wslay/")
    		file(GLOB_RECURSE SslLibrarySourcePaths FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/WebSocket/*.*)
    		target_include_directories(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibraryIncludePaths})
    		target_sources(${NETWORK_WRAPPER_DEST_TARGET} PUBLIC ${SslLibrarySourcePaths})

    		if(MSVC)
    			file(GLOB_RECURSE WebSocket FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/WebSocket/*.cpp)
    			file(GLOB_RECURSE WebSocketLayer FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/WebSocket/wslay/*.cpp)

    			file(GLOB_RECURSE WebSocketIncludes FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/WebSocket/*.hpp)
    			file(GLOB_RECURSE WebSocketLayerIncludes FOLLOW_SYMLINKS ${CMAKE_CURRENT_LIST_DIR}/WebSocket/wslay/*.hpp)
    			source_group("Header Files\\network\\WebSocket" FILES ${WebSocketIncludes})
    			source_group("Header Files\\network\\WebSocket\\Wslay" FILES ${WebSocketLayerIncludes})
    			source_group("Source Files\\network\\WebSocket" FILES ${WebSocket})
    			source_group("Source Files\\network\\WebSocket\\Wslay" FILES ${WebSocketLayer})
    		endif()
    	endif()
    endif()
