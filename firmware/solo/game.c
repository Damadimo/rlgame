// Full screen catch sim, spawn rules, drawing, and basket fruit collision.

#include <stdbool.h>

#include "game.h"
#include "graphics.h"
#include "input.h"
#include "rng.h"

// File local helpers only used inside this translation unit.
static void draw_basket(Basket *b);
static void draw_fruit(Fruit *f);
static void spawn_fruit(GameState *game);
static void update_basket(GameState *game);
static void update_fruits(GameState *game);
static bool fruit_hits_basket(Fruit *f, Basket *b);
static int clamp(int v, int lo, int hi);
static short int random_fruit_color(void);

void init_game(GameState *game)
{
    // Park the basket near the bottom center.
    game->basket.w = BASKET_WIDTH;
    game->basket.h = BASKET_HEIGHT;
    game->basket.x = (SCREEN_WIDTH - BASKET_WIDTH) / 2;
    game->basket.y = SCREEN_HEIGHT - 18;

    // Fruit pool starts empty, spawn_fruit will reuse slots.
    for (int i = 0; i < MAX_FRUITS; i++) {
        game->fruits[i].active = false;
    }

    game->score = 0;
    game->lives = INITIAL_LIVES;
    game->frame_counter = 0;
    game->running = true;
}

void update_game(GameState *game)
{
    game->frame_counter++;

    // Movement and collisions both keyed off the same frame id.
    update_basket(game);

    // Steady drip of new fruit if any slot is idle.
    if (game->frame_counter % 25 == 0) {
        spawn_fruit(game);
    }

    update_fruits(game);

    // Stop the outer main loop when the player is out of lives.
    if (game->lives <= 0) {
        game->running = false;
    }
}

void draw_game(GameState *game)
{
    // Floor line across the full width.
    draw_rect(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, GRAY);

    draw_basket(&game->basket);

    // Inactive fruit draw nothing, see draw_fruit.
    for (int i = 0; i < MAX_FRUITS; i++) {
        draw_fruit(&game->fruits[i]);
    }

    // HUD bars grow with score and remaining lives.
    draw_rect(5, 5, game->score * 6, 6, GREEN);
    draw_rect(5, 15, game->lives * 20, 6, RED);
}

static void draw_basket(Basket *b)
{
    // Main wood colored body.
    draw_rect(b->x, b->y, b->w, b->h, BROWN);
    // Thin rim strip above the brown body.
    draw_rect(b->x - 2, b->y - 2, b->w + 4, 2, YELLOW);
}

static void draw_fruit(Fruit *f)
{
    if (!f->active) {
        return;
    }

    draw_circle(f->x, f->y, f->r, f->color);
    // Two pixels as a tiny stem.
    plot_pixel(f->x, f->y - f->r - 1, GREEN);
    plot_pixel(f->x, f->y - f->r - 2, GREEN);
}

static void update_basket(GameState *game)
{
    int keys = read_movement_keys();

    // Bit masks match the DE1 pushbutton wiring used in lab demos.
    if (keys & 0x8) {
        game->basket.x -= BASKET_SPEED;
    }
    if (keys & 0x4) {
        game->basket.x += BASKET_SPEED;
    }

    // Keep the whole basket inside the visible width.
    game->basket.x = clamp(game->basket.x, 0, SCREEN_WIDTH - game->basket.w);
}

static void spawn_fruit(GameState *game)
{
    for (int i = 0; i < MAX_FRUITS; i++) {
        if (!game->fruits[i].active) {
            game->fruits[i].active = true;
            // Radius picks one of three sizes for a bit of variety.
            game->fruits[i].r = 4 + (game_rand() % 3);
            game->fruits[i].x = 10 + game_rand() % (SCREEN_WIDTH - 20);
            // Start slightly above the top edge so it enters smoothly.
            game->fruits[i].y = -5;
            game->fruits[i].dy = FRUIT_DY;
            game->fruits[i].color = random_fruit_color();
            return;
        }
    }
}

static void update_fruits(GameState *game)
{
    for (int i = 0; i < MAX_FRUITS; i++) {
        Fruit *f = &game->fruits[i];

        if (!f->active) {
            continue;
        }

        // Subsample vertical motion so speeds feel tunable with FRUIT_MOVE_EVERY.
        if (game->frame_counter % FRUIT_MOVE_EVERY == 0) {
            f->y += f->dy;
        }

        if (fruit_hits_basket(f, &game->basket)) {
            f->active = false;
            game->score++;
        } else if (f->y - f->r > SCREEN_HEIGHT) {
            // Missed catch once the fruit center clears the bottom margin.
            f->active = false;
            game->lives--;
        }
    }
}

// Axis aligned boxes for circle versus rectangle overlap, good enough here.
static bool fruit_hits_basket(Fruit *f, Basket *b)
{
    // Bounding box of the fruit disc.
    int fruit_left = f->x - f->r;
    int fruit_right = f->x + f->r;
    int fruit_top = f->y - f->r;
    int fruit_bottom = f->y + f->r;

    int basket_left = b->x;
    int basket_right = b->x + b->w;
    int basket_top = b->y;
    int basket_bottom = b->y + b->h;

    // Any separation on an axis means no overlap.
    if (fruit_right < basket_left) {
        return false;
    }
    if (fruit_left > basket_right) {
        return false;
    }
    if (fruit_bottom < basket_top) {
        return false;
    }
    if (fruit_top > basket_bottom) {
        return false;
    }

    return true;
}

// Tiny helper so basket x never underflows past zero or past the right edge.
static int clamp(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static short int random_fruit_color(void)
{
    short int colors[6] = {RED, GREEN, YELLOW, MAGENTA, CYAN, ORANGE};
    // Palette matches the VGA color constants in graphics.h.
    return colors[game_rand() % 6];
}
