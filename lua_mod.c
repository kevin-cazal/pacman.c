#include "lua_mod.h"
#include "sokol_log.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define LUA_MOD_PENDING_MAX (256 * 1024)

static char pending_src[LUA_MOD_PENDING_MAX];
static size_t pending_len;

extern const char pacman_mod_default_lua[];
extern const size_t pacman_mod_default_lua_len;

typedef struct {
    int should_move;
    int wanted_dir;
    int on_tick;
} pacman_hooks_t;

typedef struct {
    int update_state;
    int scatter_target;
    int chase_target;
    int frightened_target;
    int speed;
    int choose_dir;
} ghost_hooks_t;

static struct {
    lua_State* L;
    bool loaded;
    pacman_hooks_t pacman;
    ghost_hooks_t ghosts[LUA_MOD_GHOST_NUM];
} mod;

#define HOOK_NONE LUA_NOREF

static void unref_hook(int* ref) {
    if (mod.L && *ref != HOOK_NONE) {
        luaL_unref(mod.L, LUA_REGISTRYINDEX, *ref);
    }
    *ref = HOOK_NONE;
}

static void log_msg(uint32_t level, const char* msg) {
    slog_func("lua_mod", level, 0, msg, 0, NULL, NULL);
}

static void log_lua_error(const char* where) {
    const char* msg = mod.L ? lua_tostring(mod.L, -1) : "unknown";
    char buf[512];
    snprintf(buf, sizeof(buf), "%s: %s", where, msg ? msg : "lua error");
    log_msg(1, buf);
    if (mod.L) {
        lua_pop(mod.L, 1);
    }
}

static int l_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    log_msg(3, msg);
    return 0;
}

static const char* dir_name(lua_mod_dir_t d) {
    switch (d) {
        case LUA_MOD_DIR_RIGHT: return "right";
        case LUA_MOD_DIR_DOWN:  return "down";
        case LUA_MOD_DIR_LEFT:  return "left";
        case LUA_MOD_DIR_UP:    return "up";
        default: return "right";
    }
}

static bool dir_from_lua(lua_State* L, int idx, lua_mod_dir_t* out) {
    if (!lua_isstring(L, idx)) {
        return false;
    }
    const char* s = lua_tostring(L, idx);
    if (strcmp(s, "right") == 0) { *out = LUA_MOD_DIR_RIGHT; return true; }
    if (strcmp(s, "down") == 0)  { *out = LUA_MOD_DIR_DOWN;  return true; }
    if (strcmp(s, "left") == 0)  { *out = LUA_MOD_DIR_LEFT;  return true; }
    if (strcmp(s, "up") == 0)    { *out = LUA_MOD_DIR_UP;    return true; }
    return false;
}

static const char* ghoststate_name(lua_mod_ghoststate_t s) {
    switch (s) {
        case LUA_MOD_GHOSTSTATE_CHASE:       return "chase";
        case LUA_MOD_GHOSTSTATE_SCATTER:     return "scatter";
        case LUA_MOD_GHOSTSTATE_FRIGHTENED:  return "frightened";
        case LUA_MOD_GHOSTSTATE_EYES:        return "eyes";
        case LUA_MOD_GHOSTSTATE_HOUSE:       return "house";
        case LUA_MOD_GHOSTSTATE_LEAVEHOUSE:  return "leavehouse";
        case LUA_MOD_GHOSTSTATE_ENTERHOUSE:  return "enterhouse";
        default: return "none";
    }
}

static bool ghoststate_from_lua(lua_State* L, int idx, lua_mod_ghoststate_t* out) {
    if (!lua_isstring(L, idx)) {
        return false;
    }
    const char* s = lua_tostring(L, idx);
    if (strcmp(s, "chase") == 0)       { *out = LUA_MOD_GHOSTSTATE_CHASE; return true; }
    if (strcmp(s, "scatter") == 0)     { *out = LUA_MOD_GHOSTSTATE_SCATTER; return true; }
    if (strcmp(s, "frightened") == 0)  { *out = LUA_MOD_GHOSTSTATE_FRIGHTENED; return true; }
    if (strcmp(s, "eyes") == 0)        { *out = LUA_MOD_GHOSTSTATE_EYES; return true; }
    if (strcmp(s, "house") == 0)       { *out = LUA_MOD_GHOSTSTATE_HOUSE; return true; }
    if (strcmp(s, "leavehouse") == 0)  { *out = LUA_MOD_GHOSTSTATE_LEAVEHOUSE; return true; }
    if (strcmp(s, "enterhouse") == 0)  { *out = LUA_MOD_GHOSTSTATE_ENTERHOUSE; return true; }
    if (strcmp(s, "none") == 0)        { *out = LUA_MOD_GHOSTSTATE_NONE; return true; }
    return false;
}

static void push_i2(lua_State* L, lua_mod_i2_t v) {
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, v.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, v.y);
    lua_setfield(L, -2, "y");
}

static bool read_i2(lua_State* L, int idx, lua_mod_i2_t* out) {
    idx = lua_absindex(L, idx);
    if (!lua_istable(L, idx)) {
        return false;
    }
    lua_getfield(L, idx, "x");
    lua_getfield(L, idx, "y");
    if (!lua_isinteger(L, -2) && !lua_isnumber(L, -2)) {
        lua_pop(L, 2);
        return false;
    }
    if (!lua_isinteger(L, -1) && !lua_isnumber(L, -1)) {
        lua_pop(L, 2);
        return false;
    }
    out->x = (int16_t)lua_tointeger(L, -2);
    out->y = (int16_t)lua_tointeger(L, -1);
    lua_pop(L, 2);
    return true;
}

static void push_input(lua_State* L, const lua_mod_pacman_ctx_t* ctx) {
    lua_createtable(L, 0, 4);
    lua_pushboolean(L, ctx->input_up);
    lua_setfield(L, -2, "up");
    lua_pushboolean(L, ctx->input_down);
    lua_setfield(L, -2, "down");
    lua_pushboolean(L, ctx->input_left);
    lua_setfield(L, -2, "left");
    lua_pushboolean(L, ctx->input_right);
    lua_setfield(L, -2, "right");
}

static void push_pacman_ctx(lua_State* L, const lua_mod_pacman_ctx_t* ctx) {
    lua_createtable(L, 0, 10);
    lua_pushinteger(L, (lua_Integer)ctx->tick);
    lua_setfield(L, -2, "tick");
    lua_pushinteger(L, ctx->round);
    lua_setfield(L, -2, "round");
    lua_pushinteger(L, ctx->freeze);
    lua_setfield(L, -2, "freeze");
    lua_pushstring(L, dir_name(ctx->dir));
    lua_setfield(L, -2, "dir");
    push_i2(L, ctx->pos);
    lua_setfield(L, -2, "pos");
    push_input(L, ctx);
    lua_setfield(L, -2, "input");
    lua_pushboolean(L, ctx->dot_eaten);
    lua_setfield(L, -2, "dot_eaten");
    lua_pushboolean(L, ctx->pill_eaten);
    lua_setfield(L, -2, "pill_eaten");
}

static const char* ghost_type_name(lua_mod_ghosttype_t t) {
    switch (t) {
        case LUA_MOD_GHOST_BLINKY: return "blinky";
        case LUA_MOD_GHOST_PINKY:  return "pinky";
        case LUA_MOD_GHOST_INKY:   return "inky";
        case LUA_MOD_GHOST_CLYDE:  return "clyde";
        default: return "blinky";
    }
}

static void push_ghost_ctx(lua_State* L, const lua_mod_ghost_ctx_t* ctx) {
    lua_createtable(L, 0, 14);
    lua_pushinteger(L, (lua_Integer)ctx->tick);
    lua_setfield(L, -2, "tick");
    lua_pushinteger(L, ctx->round);
    lua_setfield(L, -2, "round");
    lua_pushinteger(L, ctx->freeze);
    lua_setfield(L, -2, "freeze");
    lua_pushstring(L, ghost_type_name(ctx->type));
    lua_setfield(L, -2, "type");
    lua_pushstring(L, ghoststate_name(ctx->state));
    lua_setfield(L, -2, "state");
    lua_pushstring(L, ghoststate_name(ctx->new_state));
    lua_setfield(L, -2, "new_state");
    lua_pushstring(L, dir_name(ctx->dir));
    lua_setfield(L, -2, "dir");
    lua_pushstring(L, dir_name(ctx->next_dir));
    lua_setfield(L, -2, "next_dir");
    push_i2(L, ctx->pos);
    lua_setfield(L, -2, "pos");
    push_i2(L, ctx->target_pos);
    lua_setfield(L, -2, "target_pos");
    push_i2(L, ctx->pacman_pos);
    lua_setfield(L, -2, "pacman_pos");
    lua_pushboolean(L, ctx->frightened);
    lua_setfield(L, -2, "frightened");
    lua_pushboolean(L, ctx->eaten);
    lua_setfield(L, -2, "eaten");
}

static int store_hook(lua_State* L, int fn_index) {
    lua_pushvalue(L, fn_index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

static void load_ghost_hooks(lua_State* L, int ghosts_table) {
    static const char* names[LUA_MOD_GHOST_NUM] = { "blinky", "pinky", "inky", "clyde" };
    for (int i = 0; i < LUA_MOD_GHOST_NUM; i++) {
        ghost_hooks_t* h = &mod.ghosts[i];
        h->update_state = h->scatter_target = h->chase_target = HOOK_NONE;
        h->frightened_target = h->speed = h->choose_dir = HOOK_NONE;

        lua_getfield(L, ghosts_table, names[i]);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            continue;
        }
        int gt = lua_gettop(L);

        lua_getfield(L, gt, "update_state");
        if (lua_isfunction(L, -1)) h->update_state = store_hook(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, gt, "scatter_target");
        if (lua_isfunction(L, -1)) h->scatter_target = store_hook(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, gt, "chase_target");
        if (lua_isfunction(L, -1)) h->chase_target = store_hook(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, gt, "frightened_target");
        if (lua_isfunction(L, -1)) h->frightened_target = store_hook(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, gt, "speed");
        if (lua_isfunction(L, -1)) h->speed = store_hook(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, gt, "choose_dir");
        if (lua_isfunction(L, -1)) h->choose_dir = store_hook(L, -1);
        lua_pop(L, 1);

        lua_pop(L, 1);
    }
}

static bool run_hook_int(int ref, void (*push_ctx)(lua_State*, const void*), const void* ctx, int* out) {
    if (!mod.L || ref == HOOK_NONE) {
        return false;
    }
    lua_rawgeti(mod.L, LUA_REGISTRYINDEX, ref);
    push_ctx(mod.L, ctx);
    if (lua_pcall(mod.L, 1, 1, 0) != LUA_OK) {
        log_lua_error("lua mod hook");
        return false;
    }
    if (lua_isnil(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    if (!lua_isinteger(mod.L, -1) && !lua_isnumber(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    *out = (int)lua_tointeger(mod.L, -1);
    lua_pop(mod.L, 1);
    return true;
}

static bool run_hook_bool(int ref, void (*push_ctx)(lua_State*, const void*), const void* ctx, bool* out) {
    if (!mod.L || ref == HOOK_NONE) {
        return false;
    }
    lua_rawgeti(mod.L, LUA_REGISTRYINDEX, ref);
    push_ctx(mod.L, ctx);
    if (lua_pcall(mod.L, 1, 1, 0) != LUA_OK) {
        log_lua_error("lua mod hook");
        return false;
    }
    if (lua_isnil(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    if (!lua_isboolean(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    *out = lua_toboolean(mod.L, -1);
    lua_pop(mod.L, 1);
    return true;
}

static bool run_hook_dir(int ref, void (*push_ctx)(lua_State*, const void*), const void* ctx, lua_mod_dir_t* out) {
    if (!mod.L || ref == HOOK_NONE) {
        return false;
    }
    lua_rawgeti(mod.L, LUA_REGISTRYINDEX, ref);
    push_ctx(mod.L, ctx);
    if (lua_pcall(mod.L, 1, 1, 0) != LUA_OK) {
        log_lua_error("lua mod hook");
        return false;
    }
    if (lua_isnil(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    if (!dir_from_lua(mod.L, -1, out)) {
        lua_pop(mod.L, 1);
        return false;
    }
    lua_pop(mod.L, 1);
    return true;
}

static bool run_hook_ghoststate(int ref, void (*push_ctx)(lua_State*, const void*), const void* ctx, lua_mod_ghoststate_t* out) {
    if (!mod.L || ref == HOOK_NONE) {
        return false;
    }
    lua_rawgeti(mod.L, LUA_REGISTRYINDEX, ref);
    push_ctx(mod.L, ctx);
    if (lua_pcall(mod.L, 1, 1, 0) != LUA_OK) {
        log_lua_error("lua mod hook");
        return false;
    }
    if (lua_isnil(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    if (!ghoststate_from_lua(mod.L, -1, out)) {
        lua_pop(mod.L, 1);
        return false;
    }
    lua_pop(mod.L, 1);
    return true;
}

static bool run_hook_i2(int ref, void (*push_ctx)(lua_State*, const void*), const void* ctx, lua_mod_i2_t* out) {
    if (!mod.L || ref == HOOK_NONE) {
        return false;
    }
    lua_rawgeti(mod.L, LUA_REGISTRYINDEX, ref);
    push_ctx(mod.L, ctx);
    if (lua_pcall(mod.L, 1, 1, 0) != LUA_OK) {
        log_lua_error("lua mod hook");
        return false;
    }
    if (lua_isnil(mod.L, -1)) {
        lua_pop(mod.L, 1);
        return false;
    }
    if (!read_i2(mod.L, -1, out)) {
        lua_pop(mod.L, 1);
        return false;
    }
    lua_pop(mod.L, 1);
    return true;
}

static void clear_hooks(void) {
    unref_hook(&mod.pacman.should_move);
    unref_hook(&mod.pacman.wanted_dir);
    unref_hook(&mod.pacman.on_tick);
    for (int i = 0; i < LUA_MOD_GHOST_NUM; i++) {
        unref_hook(&mod.ghosts[i].update_state);
        unref_hook(&mod.ghosts[i].scatter_target);
        unref_hook(&mod.ghosts[i].chase_target);
        unref_hook(&mod.ghosts[i].frightened_target);
        unref_hook(&mod.ghosts[i].speed);
        unref_hook(&mod.ghosts[i].choose_dir);
    }
}

static const char* mod_path_from_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mod") == 0 && i + 1 < argc) {
            return argv[i + 1];
        }
    }
    const char* env = getenv("PACMAN_MOD");
    return env && env[0] ? env : NULL;
}

static bool parse_mod_table(lua_State* L, int idx) {
    if (!lua_istable(L, idx)) {
        log_msg(1, "lua mod must return a table");
        return false;
    }

    lua_getfield(L, idx, "name");
    if (lua_isstring(L, -1)) {
        log_msg(3, lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    clear_hooks();

    lua_getfield(L, idx, "pacman");
    if (lua_istable(L, -1)) {
        int pt = lua_gettop(L);
        lua_getfield(L, pt, "should_move");
        if (lua_isfunction(L, -1)) mod.pacman.should_move = store_hook(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, pt, "wanted_dir");
        if (lua_isfunction(L, -1)) mod.pacman.wanted_dir = store_hook(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, pt, "on_tick");
        if (lua_isfunction(L, -1)) mod.pacman.on_tick = store_hook(L, -1);
        lua_pop(L, 1);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1);
    }

    lua_getfield(L, idx, "ghosts");
    if (lua_istable(L, -1)) {
        load_ghost_hooks(L, lua_gettop(L));
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1);
    }

    mod.loaded = true;
    return true;
}

static bool load_mod_source(const char* path, const char* src, size_t src_len) {
    if (mod.L) {
        clear_hooks();
        lua_close(mod.L);
        mod.L = NULL;
        mod.loaded = false;
    }

    mod.L = luaL_newstate();
    if (!mod.L) {
        return false;
    }
    luaL_openlibs(mod.L);
    lua_pushcfunction(mod.L, l_log);
    lua_setglobal(mod.L, "log");

    int status;
    if (path) {
        status = luaL_dofile(mod.L, path);
    } else {
        status = luaL_loadbuffer(mod.L, src, src_len, "mod");
        if (status == LUA_OK) {
            status = lua_pcall(mod.L, 0, 1, 0);
        }
    }
    if (status != LUA_OK) {
        log_lua_error("lua mod load");
        lua_close(mod.L);
        mod.L = NULL;
        return false;
    }
    return parse_mod_table(mod.L, -1);
}

bool lua_mod_set_pending_source(const char* src, size_t len) {
    if (!src || len == 0) {
        pending_len = 0;
        return true;
    }
    if (len >= LUA_MOD_PENDING_MAX) {
        log_msg(1, "lua mod source too large");
        return false;
    }
    memcpy(pending_src, src, len);
    pending_len = len;
    return true;
}

void lua_mod_clear_pending(void) {
    pending_len = 0;
}

bool lua_mod_load_buffer(const char* src, size_t len) {
    if (!src || len == 0) {
        log_msg(1, "lua mod source empty");
        return false;
    }
    if (len >= LUA_MOD_PENDING_MAX) {
        log_msg(1, "lua mod source too large");
        return false;
    }
    if (!load_mod_source(NULL, src, len)) {
        return false;
    }
    lua_pop(mod.L, 1);
    return true;
}

bool lua_mod_init(int argc, char** argv) {
    memset(&mod, 0, sizeof(mod));
    mod.pacman.should_move = mod.pacman.wanted_dir = mod.pacman.on_tick = HOOK_NONE;
    for (int i = 0; i < LUA_MOD_GHOST_NUM; i++) {
        mod.ghosts[i].update_state = HOOK_NONE;
    }

    const char* path = mod_path_from_args(argc, argv);
    bool ok;
    if (path) {
        log_msg(3, path);
        ok = load_mod_source(path, NULL, 0);
    } else if (pending_len > 0) {
        ok = load_mod_source(NULL, pending_src, pending_len);
        if (ok) {
            lua_mod_clear_pending();
        }
    } else {
        ok = load_mod_source(NULL, pacman_mod_default_lua, pacman_mod_default_lua_len);
    }
    if (!ok) {
        return false;
    }
    lua_pop(mod.L, 1);
    return true;
}

void lua_mod_shutdown(void) {
    clear_hooks();
    if (mod.L) {
        lua_close(mod.L);
        mod.L = NULL;
    }
    mod.loaded = false;
}

bool lua_mod_active(void) {
    return mod.loaded && mod.L != NULL;
}

bool lua_mod_pacman_should_move(const lua_mod_pacman_ctx_t* ctx, bool c_default, bool* out) {
    (void)c_default;
    return run_hook_bool(mod.pacman.should_move,
        (void (*)(lua_State*, const void*))push_pacman_ctx, ctx, out);
}

bool lua_mod_pacman_wanted_dir(const lua_mod_pacman_ctx_t* ctx, lua_mod_dir_t c_default, lua_mod_dir_t* out) {
    (void)c_default;
    return run_hook_dir(mod.pacman.wanted_dir,
        (void (*)(lua_State*, const void*))push_pacman_ctx, ctx, out);
}

void lua_mod_pacman_on_tick(const lua_mod_pacman_ctx_t* ctx) {
    if (!mod.L || mod.pacman.on_tick == HOOK_NONE) {
        return;
    }
    lua_rawgeti(mod.L, LUA_REGISTRYINDEX, mod.pacman.on_tick);
    push_pacman_ctx(mod.L, ctx);
    if (lua_pcall(mod.L, 1, 0, 0) != LUA_OK) {
        log_lua_error("pacman.on_tick");
    }
}

static ghost_hooks_t* ghost_hooks_for(lua_mod_ghosttype_t type) {
    if (type < 0 || type >= LUA_MOD_GHOST_NUM) {
        return NULL;
    }
    return &mod.ghosts[type];
}

bool lua_mod_ghost_speed(const lua_mod_ghost_ctx_t* ctx, int c_default, int* out) {
    (void)c_default;
    ghost_hooks_t* h = ghost_hooks_for(ctx->type);
    if (!h) {
        return false;
    }
    return run_hook_int(h->speed,
        (void (*)(lua_State*, const void*))push_ghost_ctx, ctx, out);
}

bool lua_mod_ghost_target(const lua_mod_ghost_ctx_t* ctx, lua_mod_i2_t c_default, lua_mod_i2_t* out) {
    (void)c_default;
    ghost_hooks_t* h = ghost_hooks_for(ctx->type);
    if (!h) {
        return false;
    }
    int ref = HOOK_NONE;
    switch (ctx->state) {
        case LUA_MOD_GHOSTSTATE_SCATTER:    ref = h->scatter_target; break;
        case LUA_MOD_GHOSTSTATE_CHASE:      ref = h->chase_target; break;
        case LUA_MOD_GHOSTSTATE_FRIGHTENED: ref = h->frightened_target; break;
        default: return false;
    }
    return run_hook_i2(ref,
        (void (*)(lua_State*, const void*))push_ghost_ctx, ctx, out);
}

bool lua_mod_ghost_update_state(const lua_mod_ghost_ctx_t* ctx, lua_mod_ghoststate_t c_default, lua_mod_ghoststate_t* out) {
    (void)c_default;
    ghost_hooks_t* h = ghost_hooks_for(ctx->type);
    if (!h) {
        return false;
    }
    return run_hook_ghoststate(h->update_state,
        (void (*)(lua_State*, const void*))push_ghost_ctx, ctx, out);
}

bool lua_mod_ghost_choose_dir(const lua_mod_ghost_ctx_t* ctx, lua_mod_dir_t c_default, lua_mod_dir_t* out) {
    (void)c_default;
    ghost_hooks_t* h = ghost_hooks_for(ctx->type);
    if (!h) {
        return false;
    }
    return run_hook_dir(h->choose_dir,
        (void (*)(lua_State*, const void*))push_ghost_ctx, ctx, out);
}

#ifdef __EMSCRIPTEN__
extern void pacman_restart_game(void);

EMSCRIPTEN_KEEPALIVE int wasm_set_pending_mod(const char* src) {
    if (!src) {
        return 0;
    }
    return lua_mod_set_pending_source(src, strlen(src)) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int wasm_lua_mod_load(const char* src) {
    if (!src) {
        return 0;
    }
    if (!lua_mod_load_buffer(src, strlen(src))) {
        return 0;
    }
    pacman_restart_game();
    return 1;
}
#endif
