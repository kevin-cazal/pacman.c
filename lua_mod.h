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

typedef enum {
    LUA_MOD_COLLIDE_DEFAULT = 0,
    LUA_MOD_COLLIDE_EAT_GHOST,
    LUA_MOD_COLLIDE_KILL_PACMAN,
    LUA_MOD_COLLIDE_IGNORE,
} lua_mod_collide_action_t;

typedef struct {
    lua_mod_ghosttype_t type;
    lua_mod_ghoststate_t state;
    lua_mod_i2_t pos;
    lua_mod_dir_t dir;
} lua_mod_ghost_snapshot_t;

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
    lua_mod_i2_t pixel_pos;
    lua_mod_i2_t target_pos;
    lua_mod_i2_t scatter_target;
    lua_mod_i2_t pacman_pos;
    lua_mod_dir_t pacman_dir;
    bool frightened;
    bool eaten;
    bool at_tile_center;
    uint32_t round_ticks;
    uint16_t dot_counter;
    uint16_t dot_limit;
    uint8_t global_dot_counter;
    bool global_dot_active;
    uint16_t fright_ticks;
    uint32_t fright_remaining;
    lua_mod_ghost_snapshot_t ghosts[LUA_MOD_GHOST_NUM];
} lua_mod_ghost_ctx_t;

typedef struct {
    uint32_t tick;
    uint8_t round;
    lua_mod_i2_t pos;
    uint8_t num_ghosts_eaten;
    lua_mod_ghost_snapshot_t ghosts[LUA_MOD_GHOST_NUM];
} lua_mod_game_ctx_t;

typedef struct {
    bool use_default;
    int score;
    bool update_house;
    bool force_leave_house;
} lua_mod_dot_result_t;

typedef struct {
    bool use_default;
    int score;
    bool frighten_all;
    bool frighten_none;
    bool frighten_ghost[LUA_MOD_GHOST_NUM];
    bool has_frighten_ghost[LUA_MOD_GHOST_NUM];
    bool reset_ghost_eaten_count;
    bool play_sound;
} lua_mod_pill_result_t;

typedef struct {
    bool ok;
    bool has_state;
    lua_mod_ghoststate_t state;
    bool has_dir;
    lua_mod_dir_t dir;
    bool has_speed;
    int speed;
    bool has_target;
    lua_mod_i2_t target;
    bool has_reverse;
    bool reverse;
} lua_mod_ghost_tick_result_t;

typedef struct {
    bool (*can_move_tile)(int16_t tx, int16_t ty, lua_mod_dir_t dir, bool cornering);
    bool (*is_blocking_tile)(int16_t tx, int16_t ty);
    int (*tile_distance_sq)(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
} lua_mod_maze_api_t;

void lua_mod_register_maze_api(const lua_mod_maze_api_t* api);

bool lua_mod_init(int argc, char** argv);
void lua_mod_shutdown(void);
bool lua_mod_active(void);

bool lua_mod_set_pending_source(const char* src, size_t len);
void lua_mod_clear_pending(void);
bool lua_mod_load_buffer(const char* src, size_t len);

bool lua_mod_pacman_should_move(const lua_mod_pacman_ctx_t* ctx, bool c_default, bool* out);
bool lua_mod_pacman_wanted_dir(const lua_mod_pacman_ctx_t* ctx, lua_mod_dir_t c_default, lua_mod_dir_t* out);
void lua_mod_pacman_on_tick(const lua_mod_pacman_ctx_t* ctx);

bool lua_mod_ghost_has_on_tick(lua_mod_ghosttype_t type);
bool lua_mod_ghost_on_tick(const lua_mod_ghost_ctx_t* ctx, lua_mod_ghost_tick_result_t* out);

bool lua_mod_ghost_speed(const lua_mod_ghost_ctx_t* ctx, int c_default, int* out);
bool lua_mod_ghost_target(const lua_mod_ghost_ctx_t* ctx, lua_mod_i2_t c_default, lua_mod_i2_t* out);
bool lua_mod_ghost_update_state(const lua_mod_ghost_ctx_t* ctx, lua_mod_ghoststate_t c_default, lua_mod_ghoststate_t* out);
bool lua_mod_ghost_choose_dir(const lua_mod_ghost_ctx_t* ctx, lua_mod_dir_t c_default, lua_mod_dir_t* out);

bool lua_mod_game_on_dot_eaten(const lua_mod_game_ctx_t* ctx, lua_mod_dot_result_t* out);
bool lua_mod_game_on_pill_eaten(const lua_mod_game_ctx_t* ctx, lua_mod_pill_result_t* out);
bool lua_mod_ghost_on_collide_pacman(const lua_mod_ghost_ctx_t* ctx, lua_mod_collide_action_t* out);
