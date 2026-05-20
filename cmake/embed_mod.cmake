# Generate mods_embed.c from mods/default.lua for WASM / no-filesystem builds
set(PACMAN_MOD_LUA_FILE "${CMAKE_SOURCE_DIR}/mods/default.lua")
if(EXISTS "${PACMAN_MOD_LUA_FILE}")
    file(READ "${PACMAN_MOD_LUA_FILE}" PACMAN_MOD_LUA_CONTENT)
    string(REPLACE "\\" "\\\\" PACMAN_MOD_LUA_CONTENT "${PACMAN_MOD_LUA_CONTENT}")
    string(REPLACE "\"" "\\\"" PACMAN_MOD_LUA_CONTENT "${PACMAN_MOD_LUA_CONTENT}")
    string(REPLACE "\n" "\\n\"\n\"" PACMAN_MOD_LUA_CONTENT "${PACMAN_MOD_LUA_CONTENT}")
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/mods_embed.c.in"
        "${CMAKE_BINARY_DIR}/mods_embed.c"
        @ONLY
    )
endif()
