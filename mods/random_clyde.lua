-- Clyde picks wider random targets while frightened
return {
    name = "random_clyde",
    pacman = {},
    ghosts = {
        blinky = {},
        pinky = {},
        inky = {},
        clyde = {
            frightened_target = function(ctx)
                local t = ctx.tick
                return {
                    x = (t * 7 + 3) % 28,
                    y = (t * 11 + 5) % 36,
                }
            end,
        },
    },
}
