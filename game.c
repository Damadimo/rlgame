#include <stdlib.h>
#include <stdbool.h>

#include "game.h"
#include "graphics.h"
#include "input.h"

// declaration of all helper functions
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
    game->basket.w = BASKET_WIDTH;
    game->basket.h = BASKET_HEIGHT;
    game->basket.x = (SCREEN_WIDTH - BASKET_WIDTH) / 2;     // (320-26)/2 = 142
    game->basket.y = SCREEN_HEIGHT - 18;                    // y = 222 (18 pixels above from bottom)

    // reset all active fruits
    for (int i = 0; i < MAX_FRUITS; i++) {
        game->fruits[i].active = false;
    }

    // reset all GameState variables
    game->score = 0;
    game->lives = 5;
    game->frame_counter = 0;
    game->running = true;
}

void update_game(GameState *game)
{
    // increment to next frame
    game->frame_counter++;

    update_basket(game);

    // every 25 frames drops a fruit
    if (game->frame_counter % 25 == 0) {
        spawn_fruit(game);
    }

    update_fruits(game);

    if (game->lives <= 0) {
        game->running = false;
    }
}

void draw_game(GameState *game)
{
    // gray line right above the bottom by 2px
    draw_rect(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, GRAY);

    draw_basket(&game->basket);

    // draws each fruit (inactive ones get skipped)
    for (int i = 0; i < MAX_FRUITS; i++) {
        draw_fruit(&game->fruits[i]);
    }

    // score bar at top left of the screen
    draw_rect(5, 5, game->score * 6, 6, GREEN);

    // lives bar at the below the score
    draw_rect(5, 15, game->lives * 20, 6, RED);
}

static void draw_basket(Basket *b)
{
    draw_rect(b->x, b->y, b->w, b->h, BROWN);
    draw_rect(b->x - 2, b->y - 2, b->w + 4, 2, YELLOW);
}

static void draw_fruit(Fruit *f)
{
    if (!f->active) return;

    draw_circle(f->x, f->y, f->r, f->color);
    plot_pixel(f->x, f->y - f->r - 1, GREEN);
    plot_pixel(f->x, f->y - f->r - 2, GREEN);
}

static void update_basket(GameState *game)
{
    int keys = read_keys();

    /* Change to active-low if needed */
    if (keys & 0x8) {
        game->basket.x -= BASKET_SPEED;
    }
    if (keys & 0x4) {
        game->basket.x += BASKET_SPEED;
    }

    game->basket.x = clamp(game->basket.x, 0, SCREEN_WIDTH - game->basket.w);
}

static void spawn_fruit(GameState *game)
{
    for (int i = 0; i < MAX_FRUITS; i++) {
        if (!game->fruits[i].active) {
            game->fruits[i].active = true;
            game->fruits[i].r = 4 + (rand() % 3);
            game->fruits[i].x = 10 + rand() % (SCREEN_WIDTH - 20);
            game->fruits[i].y = -5;
            game->fruits[i].dy = 1 + (rand() % 3);
            game->fruits[i].color = random_fruit_color();
            return;
        }
    }
}

static void update_fruits(GameState *game)
{
    for (int i = 0; i < MAX_FRUITS; i++) {
        Fruit *f = &game->fruits[i];

        if (!f->active) continue;

        f->y += f->dy;

        if (fruit_hits_basket(f, &game->basket)) {
            f->active = false;
            game->score++;
        }
        else if (f->y - f->r > SCREEN_HEIGHT) {
            f->active = false;
            game->lives--;
        }
    }
}

static bool fruit_hits_basket(Fruit *f, Basket *b)
{
    int fruit_left   = f->x - f->r;
    int fruit_right  = f->x + f->r;
    int fruit_top    = f->y - f->r;
    int fruit_bottom = f->y + f->r;

    int basket_left   = b->x;
    int basket_right  = b->x + b->w;
    int basket_top    = b->y;
    int basket_bottom = b->y + b->h;

    if (fruit_right < basket_left) return false;
    if (fruit_left > basket_right) return false;
    if (fruit_bottom < basket_top) return false;
    if (fruit_top > basket_bottom) return false;

    return true;
}

static int clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static short int random_fruit_color(void)
{
    short int colors[6] = {RED, GREEN, YELLOW, MAGENTA, CYAN, ORANGE};
    return colors[rand() % 6];
}