cmake_minimum_required(VERSION 4.2)
project(codotaku_vibe)

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/cpm" CACHE PATH "Force CPM cache directory" FORCE)
file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)


CPMAddPackage("gh:libsdl-org/SDL#release-3.4.12")
CPMAddPackage("gh:g-truc/glm#1.0.3")
CPMAddPackage("gh:skypjack/entt@3.16.0")

add_executable(codotaku_vibe
        src/main.cpp
        src/renderer.cpp
        src/gpu_context.cpp
        src/gbuffer.cpp
        src/resource_manager.cpp
        src/camera.cpp
        src/uploader.cpp
        src/scene.cpp
        src/material.cpp
        src/geometry.cpp
)
target_compile_options(codotaku_vibe PRIVATE /std:c++latest /EHsc)
set_target_properties(codotaku_vibe PROPERTIES CXX_STANDARD 26 CXX_EXTENSIONS OFF)
target_link_libraries(codotaku_vibe
        PRIVATE SDL3::SDL3
        PRIVATE glm::glm
        PRIVATE EnTT::EnTT
)
target_include_directories(codotaku_vibe PRIVATE include)
add_custom_command(
        TARGET codotaku_vibe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:codotaku_vibe> $<TARGET_RUNTIME_DLLS:codotaku_vibe>
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:codotaku_vibe>/assets"
        COMMAND_EXPAND_LISTS
)