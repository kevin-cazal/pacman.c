-- Full Lua ghost AI: Blinky chases Pacman via on_tick; others stay in house.
-- Demonstrates game rule hooks (optional pill/dot overrides).
return {
    name = "full_lua_ghosts",
    game = {},
    pacman = {},
    ghosts = {
        blinky = {
            on_tick = function(ctx)
                local target = ctx.pacman_pos
                local best_dir = ctx.dir
                local best_dist = 999999
                local dirs = { "up", "left", "down", "right" }
                for _, d in ipairs(dirs) do
                    local pos = { x = ctx.pos.x, y = ctx.pos.y, dir = d }
                    if can_move(pos, false) then
                        local dx = target.x - ctx.pos.x
                        local dy = target.y - ctx.pos.y
                        local dist = dx * dx + dy * dy
                        if dist < best_dist then
                            best_dist = dist
                            best_dir = d
                        end
                    end
                end
                local speed = 1
                if ctx.state == "frightened" or ctx.state == "eyes" then
                    speed = (ctx.tick % 2 == 0) and 1 or 2
                elseif ctx.tick % 7 ~= 0 then
                    speed = 1
                else
                    speed = 0
                end
                return {
                    state = "chase",
                    dir = best_dir,
                    speed = speed,
                    target = target,
                }
            end,
        },
        pinky = {
            on_tick = function(ctx)
                return { dir = ctx.dir, speed = 0, target = ctx.pos }
            end,
        },
        inky = {
            on_tick = function(ctx)
                return { dir = ctx.dir, speed = 0, target = ctx.pos }
            end,
        },
        clyde = {
            on_tick = function(ctx)
                return { dir = ctx.dir, speed = 0, target = ctx.pos }
            end,
        },
    },
}
