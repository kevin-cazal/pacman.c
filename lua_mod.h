#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct { int16_t x; int16_t y; } lua_mod_i2_t;

typedef enum {
    LUA_MOD_DIR_RIGHT = 0,
    LUA_MOD_DIR_DOWN,
    LUA_MOD_DIR_LEFT,
    LUA_MOD_DIR_UP,
    LUA_MOD_DIR_NUM
} lua_mod_dir_t;

typedef enum {
    LUA_MOD_GHOST_BLINKY = 0,
    LUA_MOD_GHOST_PINKY,
    LUA_MOD_GHOST_INKY,
    LUA_MOD_GHOST_CLYDE,
    LUA_MOD_GHOST_NUM
} lua_mod_ghosttype_t;

typedef enum {
    LUA_MOD_GHOSTSTATE_NONE = 0,
    LUA_MOD_GHOSTSTATE_CHASE,
    LUA_MOD_GHOSTSTATE_SCATTER,
    LUA_MOD_GHOSTSTATE_FRIGHTENED,
    LUA_MOD_GHOSTSTATE_EYES,
    LUA_MOD_GHOSTSTATE_HOUSE,
    LUA_MOD_GHOSTSTATE_LEAVEHOUSE,
    LUA_MOD_GHOSTSTATE_ENTERHOUSE,
} lua_mod_ghoststate_t;

typedef struct {
    uint32_t tick;
    uint8_t round;
    uint8_t freeze;
    lua_mod_dir_t dir;
    lua_mod_i2_t pos;
    bool input_up;
    bool input_down;
    bool input_left;
    bool input_right;
    bool dot_eaten;
    bool pill_eaten;
} lua_mod_pacman_ctx_t;

typedef struct {
    uint32_t tick;
    uint8_t round;
    uint8_t freeze;
    lua_mod_ghosttype_t type;
    lua_mod_ghoststate_t state;
    lua_mod_ghoststate_t new_state;
    lua_mod_dir_t dir;
    lua_mod_dir_t next_dir;
    lua_mod_i2_t pos;
    lua_mod_i2_t target_pos;
    lua_mod_i2_t pacman_pos;
    bool frightened;
    bool eaten;
} lua_mod_ghost_ctx_t;

bool lua_mod_init(int argc, char** argv);
void lua_mod_shutdown(void);
bool lua_mod_active(void);

bool lua_mod_set_pending_source(const char* src, size_t len);
void lua_mod_clear_pending(void);
bool lua_mod_load_buffer(const char* src, size_t len);

bool lua_mod_pacman_should_move(const lua_mod_pacman_ctx_t* ctx, bool c_default, bool* out);
bool lua_mod_pacman_wanted_dir(const lua_mod_pacman_ctx_t* ctx, lua_mod_dir_t c_default, lua_mod_dir_t* out);
void lua_mod_pacman_on_tick(const lua_mod_pacman_ctx_t* ctx);

bool lua_mod_ghost_speed(const lua_mod_ghost_ctx_t* ctx, int c_default, int* out);
bool lua_mod_ghost_target(const lua_mod_ghost_ctx_t* ctx, lua_mod_i2_t c_default, lua_mod_i2_t* out);
bool lua_mod_ghost_update_state(const lua_mod_ghost_ctx_t* ctx, lua_mod_ghoststate_t c_default, lua_mod_ghoststate_t* out);
bool lua_mod_ghost_choose_dir(const lua_mod_ghost_ctx_t* ctx, lua_mod_dir_t c_default, lua_mod_dir_t* out);
