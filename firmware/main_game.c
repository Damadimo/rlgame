// Unified DE1 binary, color menu picks mode then runs solo or duel with policies.

#include "graphics.h"
#include "game.h"
#include "duel_game.h"
#include "input.h"
#include "observation.h"
#include "observation_duel.h"
#include "policy.h"
#include "policy_alt.h"
#include "policy_duel.h"
#include "rng.h"

// Catch these at compile time so a bad export cannot link quietly.
#if GAME_OBS_DIM != POLICY_OBS_DIM
#error "GAME_OBS_DIM must match POLICY_OBS_DIM"
#endif
#if DUEL_GAME_OBS_DIM != DUEL_POLICY_OBS_DIM
#error "DUEL_GAME_OBS_DIM must match DUEL_POLICY_OBS_DIM"
#endif

// Colored bars map to KEY0 through KEY3 on the DE1.
enum play_kind {
    PLAY_AI_SOLO = 0,
    PLAY_SPLIT = 1,
    PLAY_HUMAN_SOLO = 2,
    PLAY_AI_SOLO_ALT = 3,
};

// Map policy action id to the same key bits the older mains used.
static int action_to_keys(int a)
{
    // Action one is nudge left on the shared key bitmask.
    if (a == 1) {
        return 0x8;
    }
    // Action two is nudge right.
    if (a == 2) {
        return 0x4;
    }
    // Action zero means no simulated press.
    return 0;
}

// Wait for vsync then treat the back buffer register as the new draw target.
static void flip_buffer(volatile int *pixel_ctrl_ptr)
{
    wait_for_vsync();
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);
}

// Six rectangles, one per mode plus a footer hint strip.
static void draw_mode_menu(void)
{
    clear_screen();
    // Top bar is the title band, each row below is a mode choice.
    draw_rect(40, 6, 240, 18, BLUE);
    draw_rect(20, 28, 280, 34, CYAN);
    draw_rect(20, 66, 280, 34, GREEN);
    draw_rect(20, 104, 280, 34, YELLOW);
    draw_rect(20, 142, 280, 34, MAGENTA);
    draw_rect(10, 184, 300, 28, GRAY);
}

// Require an edge so holding a key does not spam mode changes.
static int run_menu(volatile int *pixel_ctrl_ptr)
{
    int prev = read_keys();
    for (;;) {
        draw_mode_menu();
        flip_buffer(pixel_ctrl_ptr);
        int k = read_keys();
        if (k && !prev) {
            // Order matches the colored rows top to bottom on screen.
            if (k & 1) {
                return PLAY_AI_SOLO;
            }
            if (k & 2) {
                return PLAY_SPLIT;
            }
            if (k & 4) {
                return PLAY_HUMAN_SOLO;
            }
            if (k & 8) {
                return PLAY_AI_SOLO_ALT;
            }
        }
        prev = k;
    }
}

static void draw_postgame(enum play_kind mode, const GameState *g, const DuelState *d)
{
    clear_screen();
    // Big red banner reads as game over to the player.
    draw_rect(50, 20, 220, 28, RED);

    // Duel end screen uses winner color hints in the middle band.
    if (mode == PLAY_SPLIT && d != 0) {
        if (d->left.lives <= 0 && d->right.lives > 0) {
            // Right side agent won.
            draw_rect(100, 60, 120, 16, GREEN);
        } else if (d->right.lives <= 0 && d->left.lives > 0) {
            // Human on the left won.
            draw_rect(100, 60, 120, 16, CYAN);
        } else {
            // Rare tie or simultaneous zero lives.
            draw_rect(80, 60, 160, 16, YELLOW);
        }
    } else if (g != 0) {
        // Solo postgame shows score as a bar width capped for layout.
        int w = g->score * 5;
        if (w > 200) {
            w = 200;
        }
        if (w < 0) {
            w = 0;
        }
        draw_rect(60, 60, w, 14, YELLOW);
    }

    // Magenta and blue bars mean menu vs replay, same as menu legend.
    draw_rect(30, 190, 110, 28, MAGENTA);
    draw_rect(180, 190, 110, 28, BLUE);
}

// KEY0 returns 0 for main menu, KEY1 returns 1 to replay the same mode.
static int run_postgame(volatile int *pixel_ctrl_ptr, enum play_kind mode, const GameState *g, const DuelState *d)
{
    int prev = read_keys();
    for (;;) {
        draw_postgame(mode, g, d);
        flip_buffer(pixel_ctrl_ptr);
        int k = read_keys();
        if (k && !prev) {
            if (k & 1) {
                // Back to the color menu.
                return 0;
            }
            if (k & 2) {
                // Replay flag consumed in main below.
                return 1;
            }
        }
        prev = k;
    }
}

static void play_ai_solo(volatile int *pixel_ctrl_ptr, GameState *out)
{
    float obs[GAME_OBS_DIM];
    input_set_mode(INPUT_MODE_AGENT);
    init_game(out);

    while (out->running) {
        clear_screen();
        build_game_observation(out, obs);
        int a = policy_select_action(obs);
        input_set_agent_keys(action_to_keys(a));
        update_game(out);
        draw_game(out);
        // Show the finished frame then swap buffers for the next tick.
        flip_buffer(pixel_ctrl_ptr);
    }
}

static void play_ai_solo_alt(volatile int *pixel_ctrl_ptr, GameState *out)
{
    float obs[GAME_OBS_DIM];
    input_set_mode(INPUT_MODE_AGENT);
    init_game(out);

    while (out->running) {
        clear_screen();
        build_game_observation(out, obs);
        // Smaller float net, still driven from the full observation builder.
        int a = policy_alt_select_action_from_game_obs(obs);
        input_set_agent_keys(action_to_keys(a));
        update_game(out);
        draw_game(out);
        flip_buffer(pixel_ctrl_ptr);
    }
}

static void play_human_solo(volatile int *pixel_ctrl_ptr, GameState *out)
{
    input_set_mode(INPUT_MODE_HUMAN);
    init_game(out);

    while (out->running) {
        clear_screen();
        // No policy, keys go straight from GPIO inside update_game.
        update_game(out);
        draw_game(out);
        flip_buffer(pixel_ctrl_ptr);
    }
}

// Human uses GPIO on the left pane, agent keys feed the right pane from policy.
static void play_split(volatile int *pixel_ctrl_ptr, DuelState *out)
{
    float obs[DUEL_GAME_OBS_DIM];
    init_duel(out);

    while (out->running) {
        clear_screen();
        // Only the right half is visible to the duel policy header.
        build_duel_right_observation(out, obs);
        int a = policy_duel_select_action(obs);
        input_set_agent_keys(action_to_keys(a));
        update_duel(out);
        draw_duel(out);
        flip_buffer(pixel_ctrl_ptr);
    }
}

int main(void)
{
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    GameState g;
    DuelState d;
    enum play_kind mode;
    // Sticky replay skips run_menu until the player returns to the menu.
    int again = 0;

    init_vga_buffers();

    for (;;) {
        // Skip menu when user chose replay from postgame.
        if (!again) {
            mode = (enum play_kind)run_menu(pixel_ctrl_ptr);
        }
        again = 0;

        // Nudge RNG per mode so fruit patterns are not identical across entries.
        game_rng_seed(12345u + (unsigned)mode * 10007u);

        if (mode == PLAY_AI_SOLO) {
            play_ai_solo(pixel_ctrl_ptr, &g);
            again = run_postgame(pixel_ctrl_ptr, mode, &g, 0);
        } else if (mode == PLAY_AI_SOLO_ALT) {
            play_ai_solo_alt(pixel_ctrl_ptr, &g);
            again = run_postgame(pixel_ctrl_ptr, mode, &g, 0);
        } else if (mode == PLAY_SPLIT) {
            play_split(pixel_ctrl_ptr, &d);
            // Pass null GameState pointer, draw_postgame only reads DuelState here.
            again = run_postgame(pixel_ctrl_ptr, mode, 0, &d);
        } else {
            play_human_solo(pixel_ctrl_ptr, &g);
            again = run_postgame(pixel_ctrl_ptr, mode, &g, 0);
        }
    }
}
