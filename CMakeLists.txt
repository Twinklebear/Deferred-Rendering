cmake_minimum_required(VERSION 2.6)

project(DeferredRenderer)

# Use modified FindSDL2 and FindGLEW that will work with my windows setup
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${DeferredRenderer_SOURCE_DIR}/cmake")

# Bump up warning levels appropriately for each compiler
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O2 -std=c++11")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	add_definitions(-DDEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# On windows we need to find GLM too
if (WIN32)
	find_package(GLM REQUIRED)
	include_directories(${GLM_INCLUDE_DIRS})
endif()

include_directories(include)
include_directories(${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

add_subdirectory(src)

