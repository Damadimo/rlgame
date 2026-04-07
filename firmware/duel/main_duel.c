// Duel only entry point without the unified menu binary.

#include "graphics.h"
#include "duel_game.h"
#include "input.h"
#include "observation_duel.h"
#include "policy_duel.h"
#include "rng.h"

// Obs vector length must match the duel weight header or link will be wrong.
#if DUEL_GAME_OBS_DIM != DUEL_POLICY_OBS_DIM
#error "DUEL_GAME_OBS_DIM must match DUEL_POLICY_OBS_DIM (see rl/duel/DUEL.md)"
#endif

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

int main(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    DuelState duel;
    float obs[DUEL_GAME_OBS_DIM];

    init_vga_buffers();
    game_rng_seed(12345u);
    init_duel(&duel);

    // Policy steers right pane keys, human keys read inside update_duel.
    while (duel.running) {
        clear_screen();
        build_duel_right_observation(&duel, obs);
        int a = policy_duel_select_action(obs);
        input_set_agent_keys(action_to_keys(a));
        update_duel(&duel);
        draw_duel(&duel);

        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    }

    // Simple winner tint bars like the unified postgame sketch.
    for (;;) {
        clear_screen();
        draw_rect(40, 80, 240, 50, RED);
        if (duel.left.lives <= 0 && duel.right.lives > 0) {
            draw_rect(100, 140, 120, 10, GREEN);
        } else if (duel.right.lives <= 0 && duel.left.lives > 0) {
            draw_rect(100, 140, 120, 10, CYAN);
        } else {
            draw_rect(80, 140, 160, 10, YELLOW);
        }
        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    }
}
