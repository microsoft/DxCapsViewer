# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.21)

project (dxcapsviewer
  DESCRIPTION "DirectX Capabilities Viewer"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?linkid=2136896"
  LANGUAGES CXX)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

option(BUILD_WITH_NEW_WARP "Use the WARP package on NuGet" OFF)
set(DIRECTX_WARP_VERSION 1.0.13)

option(BUILD_WITH_NEW_DX12 "Use the DirectX 12 Agility SDK Binaries" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(build/CompilerAndLinker.cmake)

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

find_package(directx-headers CONFIG QUIET)

if(directx-headers_FOUND)
    message(STATUS "Using DirectX-Headers package")
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

if(BUILD_WITH_NEW_DX12)
    message(STATUS "Using DirectX 12 Agility SDK")
    find_package(directx12-agility CONFIG REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_D3D12_AGILITY_SDK)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX12-Agility)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /GR-)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE ${COMPILER_DEFINES})
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILER_SWITCHES})
target_link_options(${PROJECT_NAME} PRIVATE ${LINKER_SWITCHES})

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM" )
    target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wpedantic" "-Wextra"
        "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic"
        "-Wno-language-extension-token" "-Wno-switch"
        "-Wno-missing-field-initializers")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsEXE})
elseif ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze /WX)
    endif()
endif()

if(WIN32)
    if(${DIRECTX_ARCH} MATCHES "^arm64")
        set(WINVER 0x0A00)
    elseif(${DIRECTX_ARCH} MATCHES "^arm")
        set(WINVER 0x0602)
    else()
        set(WINVER 0x0601)
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=${WINVER})
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WIN32)
    if(${DIRECTX_ARCH} STREQUAL "x86")
        set(NUGET_ARCH win32)
    else()
        set(NUGET_ARCH ${DIRECTX_ARCH})
    endif()

    if(BUILD_WITH_NEW_WARP)
        message(STATUS "Downloading Microsoft.Direct3D.WARP...")
        set(DIRECTX_WARP_ARCHIVE "${CMAKE_BINARY_DIR}/Microsoft.Direct3D.WARP.${DIRECTX_WARP_VERSION}.zip")
        file(DOWNLOAD "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.WARP/${DIRECTX_WARP_VERSION}/"
            ${DIRECTX_WARP_ARCHIVE}
            EXPECTED_HASH SHA512=9f5e480947cbdd433f46ffd1a00805db5b8545938867c4a2becf1b49d85dd58234914c45caf3b84686dfd642e9d655f1df56d05a065f5ddc551bf6f1aed58162
            )

        file(ARCHIVE_EXTRACT INPUT ${DIRECTX_WARP_ARCHIVE} DESTINATION "${CMAKE_BINARY_DIR}/temp" PATTERNS *${NUGET_ARCH}*dll *${NUGET_ARCH}*pdb)

        file(GLOB_RECURSE DXBINS "${CMAKE_BINARY_DIR}/temp/*.dll" $<IF:$<CONFIG:DEBUG>,"${CMAKE_BINARY_DIR}/temp/*.pdb">)

        file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

        foreach(FILE ${DXBINS})
            get_filename_component(FILENAME ${FILE} NAME)
            file(COPY_FILE ${FILE} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILENAME}")
        endforeach(FILE)

        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/temp")
    endif()
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>>,copy,true>
    $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
    )

if(TARGET Microsoft::DirectX12-Agility)
   file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:${PROJECT_NAME}>/D3D12
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:${PROJECT_NAME}>/D3D12
      COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE_DIR:${PROJECT_NAME}>/D3D12Core.dll
      COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE_DIR:${PROJECT_NAME}>/d3d12SDKLayers.dll
      COMMAND_EXPAND_LISTS
      )
endif()
