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

FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.14
)
FetchContent_MakeAvailable(cgltf)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

#set(CUMULUS_DIR "${CMAKE_SOURCE_DIR}/..")

add_library(Nuklear INTERFACE)
target_include_directories(Nuklear INTERFACE ${nuklear_SOURCE_DIR})

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

set(MACOSX_BUNDLE_ICON_FILE icon.icns)
set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/icon.icns")
set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

add_executable(Cumulus MACOSX_BUNDLE main.c ${APP_ICON_MACOS})

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Nuklear lua cgltf)