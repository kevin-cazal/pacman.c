-- Pacman moves every 12 ticks instead of every 8
return {
    name = "slow_pacman",
    pacman = {
        should_move = function(ctx)
            if ctx.dot_eaten or ctx.pill_eaten then
                return false
            end
            return (ctx.tick % 12) ~= 0
        end,
    },
    ghosts = {
        blinky = {},
        pinky = {},
        inky = {},
        clyde = {},
    },
}
