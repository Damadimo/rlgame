#include "graphics.h"
#include "game.h"
#include "ai.h"

int main(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    GameState game;

    init_vga_buffers();
    init_game(&game);

    while (game.running) {
        clear_screen();

// looks at closest fruit falling, makes decision, executes it
#if AI_MODE
        // default settings
        int closest_x = SCREEN_WIDTH / 2;   
        int closest_y = 0;                    
        int found = 0;

        // finds the closest activatge fruit 
        for (int i = 0; i < MAX_FRUITS; i++) {
            if (game.fruits[i].active) {
                if (!found || game.fruits[i].y > closest_y) {
                    closest_x = game.fruits[i].x;
                    closest_y = game.fruits[i].y;
                    found = 1;
                }
            }
        }

        // gets AI decision
        int action = ai_get_action(closest_x, closest_y, game.basket.x);

        // executes the action on the basket (1 stay) 
        if (action == 0) {
            game.basket.x -= BASKET_SPEED;  // move left
        } else if (action == 2) {
            game.basket.x += BASKET_SPEED;  // move right
        }
#endif
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