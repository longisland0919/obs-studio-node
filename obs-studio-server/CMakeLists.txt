﻿PROJECT(obs-studio-server VERSION ${obs-studio-node_VERSION})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect Architecture (Bitness)
IF( NOT CLANG_ANALYZE_CONFIG)
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
ENDIF()

if(WIN32)
	set(PROJECT_PRODUCT_NAME "OBS Studio for Node.JS (Server)")
	set(PROJECT_COPYRIGHT "Streamlabs © 2017 - 2018")

	configure_file(
		${obs-studio-node_SOURCE_DIR}/cmake/version.rc.in
		${PROJECT_BINARY_DIR}/version.rc
		@ONLY
	)
	set(LIBOBS_PLATFORM "windows64")
endif()

if(APPLE)
	set(LIBOBS_PLATFORM "osx")
endif()

############################
# Setup crashpad binaries
############################

if(WIN32)
	download_project(
		PROJ crashpad
		URL https://obsstudionodes3.streamlabs.com/crashpad/crashpad-release-1.0.21-win-x64.tar.gz
		UPDATE_DISCONNECTED 1
	)
elseif(APPLE)
	download_project(
		PROJ crashpad
		URL https://obsstudionodes3.streamlabs.com/crashpad/crashpad-release-1.0.21-osx.zip
		UPDATE_DISCONNECTED 1
	)
endif()

ExternalProject_Add(
	crashpad_ep
	DOWNLOAD_COMMAND ""
	SOURCE_DIR "${crashpad_SOURCE_DIR}"
	INSTALL_COMMAND ""
	BUILD_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_BYPRODUCTS
		"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}base${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}util${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}client${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"<SOURCE_DIR>/bin/crashpad_database_util${CMAKE_EXECUTABLE_SUFFIX}"
		"<SOURCE_DIR>/bin/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}"
		"<SOURCE_DIR>/bin/crashpad_http_upload${CMAKE_EXECUTABLE_SUFFIX}"
)

# Our crashpad artifacts assume a particular format
# <dir>\bin contains external processes
# <dir>\lib contains static libraries we need to link against
# <dir>\include contains the primary include path
# <dir>\include\third_party\mini_chromium contains chromium include files

add_library(crashpad_util STATIC IMPORTED)
add_library(crashpad_base STATIC IMPORTED)
add_library(crashpad_client STATIC IMPORTED)
add_executable(crashpad_handler IMPORTED)
add_executable(crashpad_database_util IMPORTED)
add_executable(crashpad_http_upload IMPORTED)

# From this, we get three total targets:
#   crashpad_base
#   crashpad_util
#   crashpad_client
# It's recommended to use util but not required as far as I know.

ExternalProject_Get_Property(crashpad_ep source_dir)

set_property(TARGET crashpad_base PROPERTY IMPORTED_LOCATION
	"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}base${CMAKE_STATIC_LIBRARY_SUFFIX}")

set_property(TARGET crashpad_util PROPERTY IMPORTED_LOCATION
	"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}util${CMAKE_STATIC_LIBRARY_SUFFIX}")

set_property(TARGET crashpad_client PROPERTY IMPORTED_LOCATION
	"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}client${CMAKE_STATIC_LIBRARY_SUFFIX}")

set_property(TARGET crashpad_handler PROPERTY IMPORTED_LOCATION
	"${source_dir}/bin/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}")

set_property(TARGET crashpad_database_util PROPERTY IMPORTED_LOCATION
	"${source_dir}/bin/crashpad_database_util${CMAKE_EXECUTABLE_SUFFIX}")

set_property(TARGET crashpad_http_upload PROPERTY IMPORTED_LOCATION
	"${source_dir}/bin/crashpad_http_upload${CMAKE_EXECUTABLE_SUFFIX}")

target_include_directories(
	crashpad_base
	INTERFACE "${source_dir}/include/third_party/mini_chromium/mini_chromium"
)

target_include_directories(
	crashpad_client
	INTERFACE "${source_dir}/include"
)

add_library(crashpad INTERFACE)

target_link_libraries(
	crashpad
	INTERFACE
		crashpad_base
		crashpad_client
		crashpad_util
)

target_link_libraries(crashpad_util INTERFACE crashpad_client)

add_dependencies(crashpad_base crashpad_ep)
add_dependencies(crashpad_client crashpad_base)
add_dependencies(crashpad_util crashpad_client)

if (APPLE)
	find_library(COREFOUNDATION CoreFoundation)
	find_library(COCOA Cocoa)
	find_library(IOSURF IOSurface)
	find_library(GLKIT GLKit)
	find_library(AVFOUNDATION AVFoundation)
	find_library(IOKit IOKit)
	find_library(SECURITY_LIBRARY Security)
	find_library(BSM_LIBRARY bsm)
endif ()

# Getting LIBOBS_VERSION from azure script
file(STRINGS ../azure-pipelines.yml VERSION_LINE REGEX "LibOBSVersion")
string(REGEX MATCH "[^: ]*$" CURRENT_VERSION ${VERSION_LINE})

set(
	LIBOBS_VERSION
	${CURRENT_VERSION}
	CACHE STRING "Version of libobs being used"
)

set(
	LIBOBS_BUILD_TYPE
	"release"
	CACHE STRING "Build type of libobs artifact"
)

string(
	CONCAT OSN_DEFAULT_LIBOBS_URL
	"https://obsstudios3.streamlabs.com/"
	"libobs-${LIBOBS_PLATFORM}-${LIBOBS_BUILD_TYPE}-${LIBOBS_VERSION}.7z"
)

set(
	OSN_LIBOBS_URL
	${OSN_DEFAULT_LIBOBS_URL}
	CACHE STRING "Location of libobs archive to base on"
)

download_project(
	PROJ libobs
	URL ${OSN_LIBOBS_URL}
	UPDATE_DISCONNECTED 1
)

include("${libobs_SOURCE_DIR}/cmake/LibObs/LibObsConfig.cmake")

if (WIN32)
	set(PROJECT_LIBRARIES
		lib-streamlabs-ipc
		${LIBOBS_LIBRARIES}
		dwmapi.lib
	)
	set(PROJECT_INCLUDE_PATHS
		"${CMAKE_SOURCE_DIR}/source"
		"${PROJECT_SOURCE_DIR}/source"
		"${lib-streamlabs-ipc_SOURCE_DIR}/include"
		${LIBOBS_INCLUDE_DIRS}
		"${stackwalker_SOURCE_DIR}/Main/StackWalker"
		"${nlohmannjson_SOURCE_DIR}/single_include"
		"${source_dir}/include/third_party/mini_chromium/mini_chromium"
		"${source_dir}/include"
	)
else ()
	set(PROJECT_LIBRARIES
		lib-streamlabs-ipc
		${LIBOBS_LIBRARIES}
	)
	set(PROJECT_INCLUDE_PATHS
		"${CMAKE_SOURCE_DIR}/source"
		"${PROJECT_SOURCE_DIR}/source"
		"${lib-streamlabs-ipc_SOURCE_DIR}/include"
		${LIBOBS_INCLUDE_DIRS}
		"${nlohmannjson_SOURCE_DIR}/single_include"
		"${source_dir}/include/third_party/mini_chromium/mini_chromium"
		"${source_dir}/include"
	)
endif ()

set(PROJECT_DATA "${PROJECT_SOURCE_DIR}/resources")

SET(osn-server_SOURCES
	"${CMAKE_SOURCE_DIR}/source/error.hpp"
	"${CMAKE_SOURCE_DIR}/source/obs-property.hpp"
	"${CMAKE_SOURCE_DIR}/source/obs-property.cpp"

	###### obs-studio-node ######
	"${PROJECT_SOURCE_DIR}/source/main.cpp"
	"${PROJECT_SOURCE_DIR}/source/shared.cpp"
	"${PROJECT_SOURCE_DIR}/source/shared.hpp"
	"${PROJECT_SOURCE_DIR}/source/utility.cpp"
	"${PROJECT_SOURCE_DIR}/source/utility.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-nodeobs.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-nodeobs.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-audio.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-audio.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-calldata.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-calldata.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-common.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-common.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-display.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-display.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-fader.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-fader.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-filter.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-filter.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-global.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-global.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-iencoder.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-iencoder.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-input.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-input.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-module.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-module.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-output.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-output.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-properties.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-properties.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-scene.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-scene.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-sceneitem.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-sceneitem.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-service.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-service.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-source.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-source.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-transition.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-transition.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-video.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-video.hpp"
	"${PROJECT_SOURCE_DIR}/source/osn-volmeter.cpp"
	"${PROJECT_SOURCE_DIR}/source/osn-volmeter.hpp"

	###### utlity graphics ######
	"${PROJECT_SOURCE_DIR}/source/gs-limits.h"
	"${PROJECT_SOURCE_DIR}/source/gs-vertex.h"
	"${PROJECT_SOURCE_DIR}/source/gs-vertex.cpp"
	"${PROJECT_SOURCE_DIR}/source/gs-vertexbuffer.h"
	"${PROJECT_SOURCE_DIR}/source/gs-vertexbuffer.cpp"

	###### node-obs ######
	"${PROJECT_SOURCE_DIR}/source/nodeobs_api.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_api.h"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_audio_encoders.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_audio_encoders.h"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_autoconfig.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_autoconfig.h"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_configManager.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_configManager.hpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_display.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_display.h"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_content.h"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_common.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_service.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_service.h"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_settings.cpp"
	"${PROJECT_SOURCE_DIR}/source/nodeobs_settings.h"
	"${PROJECT_SOURCE_DIR}/source/util-memory.cpp"
	"${PROJECT_SOURCE_DIR}/source/util-memory.h"

	###### crash-manager ######
	"${PROJECT_SOURCE_DIR}/source/util-crashmanager.cpp"
	"${PROJECT_SOURCE_DIR}/source/util-crashmanager.h"
	"${PROJECT_SOURCE_DIR}/source/util-metricsprovider.cpp"
	"${PROJECT_SOURCE_DIR}/source/util-metricsprovider.h"
	
	###### callback-manager ######
	"${PROJECT_SOURCE_DIR}/source/callback-manager.cpp"
	"${PROJECT_SOURCE_DIR}/source/callback-manager.h"

	###### memory-manager ######
	"${PROJECT_SOURCE_DIR}/source/memory-manager.cpp"
	"${PROJECT_SOURCE_DIR}/source/memory-manager.h"
)

if (APPLE)
	SET(osn-server-osx_SOURCES
		###### osx-util ######
		"${PROJECT_SOURCE_DIR}/source/util-osx.hpp"
		"${PROJECT_SOURCE_DIR}/source/util-osx.cpp"
		"${PROJECT_SOURCE_DIR}/source/util-osx-int.h"
		"${PROJECT_SOURCE_DIR}/source/util-osx-impl.h"
		"${PROJECT_SOURCE_DIR}/source/util-osx-impl.mm"
	)
	LIST(
		APPEND
		osn-server_SOURCES
		${osn-server-osx_SOURCES}
	)
endif ()
find_library(WINDOW_CAPTURE
		NAMES window-capture
		HINTS longisland-plugins)

include_directories("longisland-plugins/include")

add_executable(
	${PROJECT_NAME}
	${osn-server_SOURCES}
)

if(WIN32)
	# Include/link crash manager dependencies
	target_link_libraries(${PROJECT_NAME} StackWalker)

	target_sources(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}/version.rc")
endif()

if(WIN32)
	target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_PATHS})
	target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARIES} optimized crashpad)
else()
	target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_PATHS} ${COREFOUNDATION} ${COCOA} ${IOSURF} ${GLKIT} ${AVFOUNDATION} ${IOKit} ${SECURITY_LIBRARY} ${BSM_LIBRARY})
	target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARIES} crashpad ${WINDOW_CAPTURE} ${COREFOUNDATION} ${COCOA} ${IOSURF} ${GLKIT} ${AVFOUNDATION} ${IOKit} ${SECURITY_LIBRARY} ${BSM_LIBRARY})
endif()

#Define the OSN_VERSION
add_compile_definitions(OSN_VERSION=\"$ENV{tagartifact}\")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "obs${BITS}")

if(WIN32)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION}
	)

	target_compile_definitions(${PROJECT_NAME} PRIVATE)
else()
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION}
	)
	target_compile_definitions(${PROJECT_NAME} PRIVATE)
	set_property(
		TARGET ${PROJECT_NAME}
		APPEND
		PROPERTY INSTALL_RPATH
		"@loader_path/" "@executable_path/"
	)
ENDIF()

IF(WIN32)
	target_compile_definitions(
		${PROJECT_NAME}
		PRIVATE
			WIN32_LEAN_AND_MEAN
			NOMINMAX
			UNICODE
			_UNICODE
	)
ENDIF()

IF( NOT CLANG_ANALYZE_CONFIG)
	cppcheck_add_project(${PROJECT_NAME})
ENDIF()

set(PROGRAM_PERMISSIONS_DEFAULT
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

install(TARGETS obs-studio-server RUNTIME DESTINATION "./" PERMISSIONS ${PROGRAM_PERMISSIONS_DEFAULT} COMPONENT Runtime)
IF(WIN32 AND NOT CLANG_ANALYZE_CONFIG)
	install(FILES $<TARGET_PDB_FILE:obs-studio-server> DESTINATION "./" OPTIONAL)
ENDIF()
install(DIRECTORY ${PROJECT_DATA} DESTINATION "./" OPTIONAL USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${crashpad_SOURCE_DIR}/bin/ DESTINATION "./" USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${libobs_SOURCE_DIR}/data/" DESTINATION "./data" USE_SOURCE_PERMISSIONS)

if (APPLE)
	install(DIRECTORY "${libobs_SOURCE_DIR}/Frameworks/" DESTINATION "./Frameworks" USE_SOURCE_PERMISSIONS)
endif()

if (WIN32)
	install(
		DIRECTORY "${libobs_SOURCE_DIR}/bin/64bit/"
		DESTINATION "./" PATTERN "*.lib" EXCLUDE
	)
else ()
	install(
		DIRECTORY "${libobs_SOURCE_DIR}/bin/"
		DESTINATION "./" USE_SOURCE_PERMISSIONS PATTERN "*.lib" EXCLUDE
	)
	#add longisland plugins
	install(
			DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/longisland-plugins/"
			DESTINATION "./"
			USE_SOURCE_PERMISSIONS
			PATTERN "include" EXCLUDE
	)
endif ()

if (WIN32)
install(
	DIRECTORY "${libobs_SOURCE_DIR}/obs-plugins/"
	DESTINATION "./obs-plugins"
)
else ()
install(
	DIRECTORY "${libobs_SOURCE_DIR}/obs-plugins/"
	DESTINATION "./obs-plugins"
	USE_SOURCE_PERMISSIONS
)
endif()
