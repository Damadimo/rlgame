#include "graphics.h"
#include "game.h"
#include "input.h"
#include "observation.h"
#include "policy.h"
#include "rng.h"

#if GAME_OBS_DIM != POLICY_OBS_DIM
#error "GAME_OBS_DIM must match POLICY_OBS_DIM (see CONTRACT.md)"
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
    GameState game;
    float obs[GAME_OBS_DIM];

    init_vga_buffers();
    input_set_mode(INPUT_MODE_AGENT);
    game_rng_seed(12345u);
    init_game(&game);

    while (game.running) {
        clear_screen();
        build_game_observation(&game, obs);
        int a = policy_select_action(obs);
        input_set_agent_keys(action_to_keys(a));
        update_game(&game);
        draw_game(&game);

        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    }

    while (1) {
        clear_screen();
        draw_rect(60, 90, 200, 40, RED);
        draw_rect(90, 140, game.score * 5, 10, YELLOW);

        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    }

    return 0;
}
