cmake_minimum_required(VERSION 4.2)
project(codotaku_vibe)

set(CMAKE_CXX_STANDARD 26)

include(FetchContent)

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.12
)

FetchContent_MakeAvailable(SDL3)

add_executable(codotaku_vibe src/main.cpp)
target_link_libraries(codotaku_vibe
        PRIVATE SDL3::SDL3
)
target_include_directories(codotaku_vibe PRIVATE include)
add_custom_command(
        TARGET codotaku_vibe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:codotaku_vibe> $<TARGET_RUNTIME_DLLS:codotaku_vibe>
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:codotaku_vibe>/assets"
        COMMAND_EXPAND_LISTS
)