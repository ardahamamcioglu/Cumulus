cmake_minimum_required(VERSION 4.1)
project(Cumulus C)

include(FetchContent)

set(CMAKE_C_STANDARD 23)

#Fetch SDL
FetchContent_Declare(
        sdl
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.28
)
FetchContent_MakeAvailable(sdl)

FetchContent_Declare(
        nuklear
        GIT_REPOSITORY https://github.com/Immediate-Mode-UI/Nuklear.git
        GIT_TAG 4.12.8
)
FetchContent_MakeAvailable(nuklear)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

#set(CUMULUS_DIR "${CMAKE_SOURCE_DIR}/..")

add_library(Nuklear INTERFACE)
target_include_directories(Nuklear INTERFACE ${nuklear_SOURCE_DIR})

add_executable(Cumulus main.c
        nuklear_impl.c)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Nuklear)