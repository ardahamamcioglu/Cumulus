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

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/icon.icns")
    if(EXISTS ${APP_ICON_MACOS})
        set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        set(PLATFORM_RESOURCES ${APP_ICON_MACOS})
    endif()
    add_executable(Cumulus MACOSX_BUNDLE main.c ${PLATFORM_RESOURCES})
elseif(WIN32)
    set(APP_ICON_WINDOWS "${CMAKE_SOURCE_DIR}/resources.rc")
    if(EXISTS ${APP_ICON_WINDOWS})
        set(PLATFORM_RESOURCES ${APP_ICON_WINDOWS})
    endif()
    add_executable(Cumulus WIN32 main.c ${PLATFORM_RESOURCES})
else()
    # Linux - no embedded icon, uses .desktop file
    add_executable(Cumulus main.c)
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS Cumulus DESTINATION bin)
    install(FILES "${CMAKE_SOURCE_DIR}/cumulus.desktop" DESTINATION share/applications OPTIONAL)
    install(FILES "${CMAKE_SOURCE_DIR}/icon.png" DESTINATION share/icons/hicolor/256x256/apps RENAME cumulus.png OPTIONAL)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Nuklear lua cgltf)