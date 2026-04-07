// Step the solo sim from stdin actions and print CSV rows for Python parity checks.
// Feed one integer action per line, same encoding the firmware uses for agent keys.
// Build with make parity from repo root, used next to Python parity scripts.

#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "input.h"
#include "rng.h"

// Must match firmware mains mapping from policy action id to key bitmask.
static int action_to_keys(int a)
{
    if (a == 1) {
        return 0x8;
    }
    if (a == 2) {
        return 0x4;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <seed_uint> <num_steps>\n", argv[0]);
        return 1;
    }
    unsigned seed = (unsigned)strtoul(argv[1], NULL, 10);
    int n = atoi(argv[2]);

    // Lock RNG so a given seed and action script reproduces every time.
    game_rng_seed(seed);
    input_set_mode(INPUT_MODE_AGENT);
    GameState g;
    init_game(&g);

    for (int step = 0; step < n; step++) {
        int a;
        if (scanf("%d", &a) != 1) {
            // stdin ended early or bad token.
            return 1;
        }
        input_set_agent_keys(action_to_keys(a));
        update_game(&g);
        // Columns mirror what the Python parity script expects to parse.
        printf(
            "%d,%d,%d,%d,%d\n",
            g.frame_counter,
            g.basket.x,
            g.score,
            g.lives,
            g.running ? 1 : 0
        );
    }
    return 0;
}
