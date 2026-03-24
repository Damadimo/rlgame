/* Read actions from stdin (one integer 0-2 per line), print state CSV per step. */
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "input.h"
#include "rng.h"

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

    game_rng_seed(seed);
    input_set_mode(INPUT_MODE_AGENT);
    GameState g;
    init_game(&g);

    for (int step = 0; step < n; step++) {
        int a;
        if (scanf("%d", &a) != 1) {
            return 1;
        }
        input_set_agent_keys(action_to_keys(a));
        update_game(&g);
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
