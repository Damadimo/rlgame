#include "graphics.h"
#include "game.h"
#include "input.h"
#include "rng.h"

int main(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    GameState game;

    init_vga_buffers();
    input_set_mode(INPUT_MODE_HUMAN);
    game_rng_seed(12345u);
    init_game(&game);

    while (game.running) {
        clear_screen();
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