cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
	execute_process(
			COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
			COMMAND head -n 1
			COMMAND cut -c9-
			OUTPUT_VARIABLE SDK
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game Name Customization
set(PLAYDATE_GAME_NAME hello_world_c)
set(PLAYDATE_GAME_DEVICE hello_world_c_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)


# - Run pre-build scripts
add_custom_target(copy_assets_playdate
	COMMAND ${CMAKE_COMMAND} -DPLAYDATE_GAME_NAME:STRING=${PLAYDATE_GAME_NAME} -P ${CMAKE_CURRENT_LIST_DIR}/copy-assets-playdate.cmake
)

if (TOOLCHAIN STREQUAL "armgcc")
	add_executable(
		${PLAYDATE_GAME_DEVICE}
		# - Edit to add project's C source files
		src/main.c
		src/text_manager.c
	)
	target_include_directories(${PLAYDATE_GAME_DEVICE} PUBLIC
		include
	)
	add_dependencies(${PLAYDATE_GAME_DEVICE} copy_assets_playdate)
else()
	add_library(
		${PLAYDATE_GAME_NAME} SHARED
		# - Edit to add project's C source and header files
		src/main.c
		src/text_manager.c
		include/text_manager.h
	)
	target_include_directories(${PLAYDATE_GAME_NAME} PUBLIC
		include
	)
	add_dependencies(${PLAYDATE_GAME_NAME} copy_assets_playdate)
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)
