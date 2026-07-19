cmake_minimum_required(VERSION 4.1)
project(Cumulus C)

include(FetchContent)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Fetch SDL
FetchContent_Declare(
        sdl
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(sdl)

#Fetch SDL Shadercross
FetchContent_Declare(
					shadercross
					GIT_REPOSITORY https://github.com/libsdl-org/SDL_shadercross.git
        GIT_TAG main
        GIT_PROGRESS TRUE
)
set(SDLSHADERCROSS_VENDORED ON)
set(SDLSHADERCROSS_DXC OFF CACHE BOOL "")
set(SDLSHADERCROSS_CLI OFF CACHE BOOL "")
set(SDLSHADERCROSS_INSTALL OFF CACHE BOOL "")
set(SDLSHADERCROSS_TESTS OFF CACHE BOOL "")
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "")
FetchContent_MakeAvailable(shadercross)

FetchContent_Declare(
        microui
        GIT_REPOSITORY https://github.com/rxi/microui.git
        GIT_TAG master
)
FetchContent_MakeAvailable(microui)

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
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(cgltf)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

#set(CUMULUS_DIR "${CMAKE_SOURCE_DIR}/..")

add_library(microui STATIC
    ${microui_SOURCE_DIR}/src/microui.c
)
target_include_directories(microui PUBLIC ${microui_SOURCE_DIR}/src)
set_property(TARGET microui PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/icon.icns")
    if(EXISTS ${APP_ICON_MACOS})
        set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        set(PLATFORM_RESOURCES ${APP_ICON_MACOS})
    endif()
    add_executable(Cumulus MACOSX_BUNDLE main.c app.c lua_script.c model_import.c ${PLATFORM_RESOURCES})
elseif(WIN32)
    set(APP_ICON_WINDOWS "${CMAKE_SOURCE_DIR}/resources.rc")
    if(EXISTS ${APP_ICON_WINDOWS})
        set(PLATFORM_RESOURCES ${APP_ICON_WINDOWS})
    endif()
    add_executable(Cumulus WIN32 main.c app.c lua_script.c model_import.c ${PLATFORM_RESOURCES})
else()
    # Linux - no embedded icon, uses .desktop file
    add_executable(Cumulus main.c app.c lua_script.c model_import.c)
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS Cumulus DESTINATION bin)
    install(FILES "${CMAKE_SOURCE_DIR}/cumulus.desktop" DESTINATION share/applications OPTIONAL)
    install(FILES "${CMAKE_SOURCE_DIR}/icon.png" DESTINATION share/icons/hicolor/256x256/apps RENAME cumulus.png OPTIONAL)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_shadercross::SDL3_shadercross microui lua cgltf)
