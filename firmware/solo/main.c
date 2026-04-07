// Human plays full screen catch until lives run out, then a frozen score screen.

#include "graphics.h"
#include "game.h"
#include "input.h"
#include "rng.h"

int main(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    GameState game;

    // Standard bring up for this board, human input, fixed demo seed.
    init_vga_buffers();
    input_set_mode(INPUT_MODE_HUMAN);
    game_rng_seed(12345u);
    init_game(&game);

    // One VGA frame per sim step while the session is alive.
    while (game.running) {
        clear_screen();
        update_game(&game);
        draw_game(&game);

        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    }

    // Game over loop keeps the final score visible.
    for (;;) {
        clear_screen();
        draw_rect(60, 90, 200, 40, RED);
        draw_rect(90, 140, game.score * 5, 10, YELLOW);

        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);
    }
}
