-- Blinky always targets Pacman's tile in chase mode
return {
    name = "aggressive_blinky",
    pacman = {},
    ghosts = {
        blinky = {
            -- always in chase mode, always chasing Pacman's tile
            update_state = function(ctx)
                log("update_state")
                return "chase"
            end,
            chase_target = function(ctx)
                log("chase_target")
                return ctx.pacman_pos
            end,
        },
        pinky = { -- don't move
            speed = function(ctx)
                log("speed")
                return 0
            end,
        },
        inky = { -- don't move
            speed = function(ctx)
                log("speed")
                return 0
            end,
        },
        clyde = { -- don't move
            speed = function(ctx)
                log("speed")
                return 0
            end,
        },
    },
}
