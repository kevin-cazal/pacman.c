# Pacman.c Lua mods

Mods are Lua files that return a table with optional `pacman` and `ghosts` hooks. If a hook is missing or returns `nil`, the game uses the built-in C behavior.

## Run with a mod

```bash
./build/pacman --mod mods/aggressive_blinky.lua
# or
PACMAN_MOD=mods/slow_pacman.lua ./build/pacman
```

Without `--mod`, the embedded `mods/default.lua` is used (vanilla behavior).

## WASM mod lab (browser)

The Emscripten build uses `web/shell_mod.html`, `web/mod_loader.js`, `web/mod_editor.js`, self-hosted Monaco (`web/monaco/vs/`), and `web/mod_lab.html` (copied next to `pacman.html`). The mod pane is a Monaco editor with Lua syntax highlighting. Click the **Lua mod** pane to edit; click the **game** pane to play. **Load mod** applies Lua and restarts play.

```bash
./build.sh wasm   # runs scripts/vendor-monaco.sh if web/monaco/ is missing
python3 -m http.server -d build-wasm/
```

Open `mod_lab.html` or `pacman.html`. To vendor Monaco only: `./scripts/vendor-monaco.sh`.

## Pacman hooks

| Hook | Return | Description |
|------|--------|-------------|
| `should_move` | boolean | Whether Pacman moves this tick |
| `wanted_dir` | `"up"` / `"down"` / `"left"` / `"right"` | Desired direction before cornering |
| `on_tick` | nothing | Called each tick while Pacman would move |

Context fields: `tick`, `round`, `freeze`, `dir`, `pos` `{x,y}` (tile), `input` `{up,down,left,right}`, `dot_eaten`, `pill_eaten`.

## Ghost hooks (per ghost: `blinky`, `pinky`, `inky`, `clyde`)

| Hook | Return | Description |
|------|--------|-------------|
| `scatter_target` | `{x,y}` | Tile target in scatter mode |
| `chase_target` | `{x,y}` | Tile target in chase mode |
| `frightened_target` | `{x,y}` | Tile target when frightened |
| `speed` | integer | Pixels to move this tick |
| `update_state` | state name | Override proposed state transition |
| `choose_dir` | direction string | Override `next_dir` at intersections |

Ghost context: `type`, `state`, `new_state`, `dir`, `next_dir`, `pos`, `target_pos`, `pacman_pos`, `frightened`, `eaten`, plus common tick/round/freeze.

State names: `chase`, `scatter`, `frightened`, `eyes`, `house`, `leavehouse`, `enterhouse`, `none`.

Use `log("message")` for console output.

## Example mods

- `default.lua` — empty hooks (vanilla)
- `slow_pacman.lua` — slower movement cadence
- `aggressive_blinky.lua` — Blinky chases Pacman's tile directly
- `random_clyde.lua` — Clyde uses a custom frightened wander pattern
