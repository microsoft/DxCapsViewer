# DirectX Capabilities Viewer
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# https://github.com/walbourn/dxcapsviewer

cmake_minimum_required (VERSION 3.11)

project (dxcapsviewer LANGUAGES CXX)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/CMake")

if(MSVC)
    # Use max Warning Level 
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # Not using typeid or dynamic_cast, so disable RTTI to save binary size
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

add_executable(${PROJECT_NAME} WIN32
    ddraw.cpp
    dxg.cpp
    dxgi.cpp
    dxprint.cpp
    dxview.h
    dxview.cpp
    resource.h
    dxview.rc)

target_link_libraries(${PROJECT_NAME} PRIVATE dxguid.lib comctl32.lib version.lib)

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wpedantic" "-Wextra"
        "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic"
        "-Wno-language-extension-token" "-Wno-switch"
        "-Wno-missing-field-initializers")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /JMC- /Zc:__cplusplus)

    if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5105)
    endif()
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _MBCS _WIN32_WINNT=0x0601)
endif()
