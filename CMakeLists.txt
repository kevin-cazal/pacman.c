cmake_minimum_required(VERSION 3.20)
project(pacman C)
set(CMAKE_C_STANDARD 11)

include(${CMAKE_SOURCE_DIR}/cmake/embed_mod.cmake)

# POSIX thread handling (covers Linux, GNU/Hurd, BSD)
if (UNIX AND NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

#=== LIBRARY: lua
add_subdirectory(third_party/lua)

#=== LIBRARY: sokol
set(SOKOL_HEADERS
    sokol/sokol_gfx.h
    sokol/sokol_app.h
    sokol/sokol_audio.h
    sokol/sokol_glue.h)
add_library(sokol STATIC sokol/sokol.c ${SOKOL_HEADERS})
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    target_compile_options(sokol PRIVATE -x objective-c)
    target_link_libraries(sokol
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework Metal"
        "-framework OpenGL"
        "-framework AudioToolbox")
elseif(CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    # browser build: no X11/ALSA
else()
    if (UNIX AND NOT APPLE)
        target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL asound dl m)
        target_link_libraries(sokol PUBLIC Threads::Threads)
    endif()
endif()
target_include_directories(sokol INTERFACE sokol)

#=== LIBRARY: lua_mod
add_library(lua_mod STATIC lua_mod.c lua_mod.h ${CMAKE_BINARY_DIR}/mods_embed.c)
target_include_directories(lua_mod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lua_mod PRIVATE third_party/lua/src sokol)
target_link_libraries(lua_mod PUBLIC lua_static)
#=== EXECUTABLE: pacman
set(PACMAN_SOURCES pacman.c)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_executable(pacman WIN32 ${PACMAN_SOURCES})
else()
    add_executable(pacman ${PACMAN_SOURCES})
endif()
target_link_libraries(pacman sokol lua_mod)
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_link_options(pacman PUBLIC --shell-file ${CMAKE_SOURCE_DIR}/web/shell_mod.html)
    target_link_options(pacman PUBLIC -sUSE_WEBGL2=1 -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sMALLOC=emmalloc --closure=1)
    target_link_options(pacman PUBLIC "-sEXPORTED_RUNTIME_METHODS=['cwrap','ccall']")
    add_custom_command(TARGET pacman POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/web/mod_loader.js
            $<TARGET_FILE_DIR:pacman>/mod_loader.js
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/web/mod_editor.js
            $<TARGET_FILE_DIR:pacman>/mod_editor.js
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/web/mod_lab.html
            $<TARGET_FILE_DIR:pacman>/mod_lab.html
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/web/monaco
            $<TARGET_FILE_DIR:pacman>/monaco
        COMMENT "Copy mod lab HTML/JS/Monaco beside pacman.html")
endif()
if (MSVC)
    target_compile_options(pacman PUBLIC /W3)
else()
    target_compile_options(pacman PUBLIC -Wall -Wextra -Wsign-compare)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin"
    AND CMAKE_C_COMPILER_ID MATCHES "Clang"
    AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_options(pacman PRIVATE LINKER:-dead_strip)
endif()
