# Pacman.c Lua mods

Mods are Lua files that return a table with optional `game`, `pacman`, and `ghosts` hooks. If a hook is missing or returns `nil`, the game uses the built-in C behavior.

## Run with a mod

```bash
./build/pacman --mod mods/aggressive_blinky.lua
# or
PACMAN_MOD=mods/slow_pacman.lua ./build/pacman
```

Without `--mod`, the embedded `mods/default.lua` is used (vanilla behavior).

## WASM mod lab (browser)

The Emscripten build uses `web/shell_mod.html`, `web/mod_loader.js`, `web/mod_editor.js`, self-hosted Monaco (`web/monaco/vs/`), and `web/mod_lab.html` (copied next to `index.html`). The mod pane is a Monaco editor with Lua syntax highlighting. Click the **Lua mod** pane to edit; click the **game** pane to play. **Load mod** applies Lua and restarts play.

```bash
./build.sh wasm   # runs scripts/vendor-monaco.sh if web/monaco/ is missing
python3 -m http.server -d build-wasm/
```

Open `index.html` or `mod_lab.html`. GitHub Pages deploys the staged `site/` output via `.github/workflows/pages.yml`. To vendor Monaco only: `./scripts/vendor-monaco.sh`.

## Maze helpers (globals)

| Function | Description |
|----------|-------------|
| `can_move(pos, cornering?)` | `pos` is `{x,y,dir}` tile coords, or `can_move(tx, ty, dir, cornering?)` |
| `tile_blocked(pos)` | `pos` is `{x,y}` or `tile_blocked(tx, ty)` |
| `dist_sq(a, b)` | Squared tile distance between two `{x,y}` positions |
| `log("message")` | Console log |

## Pacman hooks

| Hook | Return | Description |
|------|--------|-------------|
| `should_move` | boolean | Whether Pacman moves this tick |
| `wanted_dir` | `"up"` / `"down"` / `"left"` / `"right"` | Desired direction before cornering |
| `on_tick` | nothing | Called each tick while Pacman would move |

Context fields: `tick`, `round`, `freeze`, `dir`, `pos` `{x,y}` (tile), `input` `{up,down,left,right}`, `dot_eaten`, `pill_eaten`.

## Ghost hooks (per ghost: `blinky`, `pinky`, `inky`, `clyde`)

### Full AI takeover: `on_tick`

When `on_tick` is defined for a ghost, **all other ghost hooks for that ghost are ignored**, and the built-in C state machine, target selection, and direction logic are skipped. Return a table each tick:

| Field | Required | Description |
|-------|----------|-------------|
| `dir` | yes | Movement direction this tick |
| `speed` | yes | Pixels to move (0 = stay still) |
| `state` | no | New ghost state name (see below) |
| `target` | no | Tile target `{x,y}` (debug overlay) |
| `reverse` | no | Reverse direction on scatter/chase change (default: auto) |

### Partial overrides (only when `on_tick` is absent)

| Hook | Return | Description |
|------|--------|-------------|
| `scatter_target` | `{x,y}` | Tile target in scatter mode |
| `chase_target` | `{x,y}` | Tile target in chase mode |
| `frightened_target` | `{x,y}` | Tile target when frightened |
| `speed` | integer | Pixels to move this tick |
| `update_state` | state name | Override proposed state transition |
| `choose_dir` | direction string | Override `next_dir` at intersections |
| `on_collide_pacman` | `"eat_ghost"` / `"kill_pacman"` / `"ignore"` | Override collision outcome |

### Ghost context (`on_tick` and partial hooks)

`type`, `state`, `new_state`, `dir`, `next_dir`, `pos` (tile), `pixel_pos`, `target_pos`, `scatter_target`, `pacman_pos`, `pacman_dir`, `frightened`, `eaten`, `at_tile_center`, `round_ticks`, `dot_counter`, `dot_limit`, `global_dot_counter`, `global_dot_active`, `fright_ticks`, `fright_remaining`, `ghosts` (table of all four ghosts with `type`, `state`, `pos`, `dir`).

State names: `chase`, `scatter`, `frightened`, `eyes`, `house`, `leavehouse`, `enterhouse`, `none`.

## Game hooks (`game` table)

| Hook | Return | Description |
|------|--------|-------------|
| `on_dot_eaten` | table or `nil` | Override dot scoring and house counters. `nil` = vanilla. Table: `{ score = N, house = "default" \| "none" }` |
| `on_pill_eaten` | table or `nil` | Override pill scoring and fright. `nil` = vanilla. Table: `{ score = N, frighten = "all" \| "none" \| { blinky = true, ... }, sound = bool, reset_ghost_eaten_count = bool }` |

Context: `tick`, `round`, `pos`, `num_ghosts_eaten`, `ghosts` (snapshot of all four).

## Example mods

- `default.lua` — empty hooks (vanilla)
- `slow_pacman.lua` — slower movement cadence
- `aggressive_blinky.lua` — Blinky chases Pacman's tile directly (partial hooks)
- `random_clyde.lua` — Clyde uses a custom frightened wander pattern
- `full_lua_ghosts.lua` — Blinky full `on_tick` AI with maze helpers; other ghosts frozen
