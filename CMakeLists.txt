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

FetchContent_Declare(
        lua
        URL      https://www.lua.org/ftp/lua-5.5.0.tar.gz
        URL_HASH SHA256=57ccc32bbbd005cab75bcc52444052535af691789dba2b9016d5c50640d68b3d
        PATCH_COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/lua_CMakeLists.txt" "CMakeLists.txt"
)
FetchContent_MakeAvailable(lua)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

#set(CUMULUS_DIR "${CMAKE_SOURCE_DIR}/..")

add_library(Nuklear INTERFACE)
target_include_directories(Nuklear INTERFACE ${nuklear_SOURCE_DIR})

#add_subdirectory(${lua_SOURCE_DIR})
#add_library(Lua INTERFACE)
#target_include_directories(Lua INTERFACE ${lua_source_SOURCE_DIR})

add_executable(Cumulus main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Nuklear lua)