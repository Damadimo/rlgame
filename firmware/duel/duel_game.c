// Split screen catch, two independent panes plus shared frame counter.

#include <stdbool.h>

#include "duel_game.h"
#include "graphics.h"
#include "input.h"
#include "rng.h"

// Integer clamp for basket x inside each pane width.
static int clamp_i(int v, int lo, int hi)
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
    // Same palette idea as solo game.c.
    return colors[game_rand() % 6];
}

// Same AABB test as solo mode, duplicated so duel stays self contained.
static bool fruit_hits_basket(const Fruit *f, const Basket *b)
{
    // Axis aligned overlap test on fruit bounds versus basket rect.
    int fruit_left = f->x - f->r;
    int fruit_right = f->x + f->r;
    int fruit_top = f->y - f->r;
    int fruit_bottom = f->y + f->r;

    int basket_left = b->x;
    int basket_right = b->x + b->w;
    int basket_top = b->y;
    int basket_bottom = b->y + b->h;

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

// One half of the screen, fruits and basket in pane local coordinates.
static void init_pane(DuelPane *p)
{
    p->basket.w = DUEL_BASKET_W;
    p->basket.h = DUEL_BASKET_H;
    p->basket.x = (DUEL_PANE_W - DUEL_BASKET_W) / 2;
    p->basket.y = DUEL_PANE_H - 18;
    for (int i = 0; i < DUEL_MAX_FRUITS; i++) {
        p->fruits[i].active = false;
    }
    p->score = 0;
    p->lives = DUEL_INITIAL_LIVES;
}

void init_duel(DuelState *d)
{
    // Symmetric start for human left and agent right.
    init_pane(&d->left);
    init_pane(&d->right);
    d->frame_counter = 0;
    d->running = true;
}

static void update_basket_pane(DuelPane *p, int key_bits)
{
    // 0x8 and 0x4 line up with solo firmware key wiring.
    if (key_bits & 0x8) {
        p->basket.x -= DUEL_BASKET_SPEED;
    }
    if (key_bits & 0x4) {
        p->basket.x += DUEL_BASKET_SPEED;
    }
    int max_x = DUEL_PANE_W - p->basket.w;
    p->basket.x = clamp_i(p->basket.x, 0, max_x);
}

static void spawn_fruit_pane(DuelPane *p)
{
    for (int i = 0; i < DUEL_MAX_FRUITS; i++) {
        if (!p->fruits[i].active) {
            // Smaller radii than solo, fits the narrow pane.
            p->fruits[i].active = true;
            p->fruits[i].r = 2 + (game_rand() % 3);
            p->fruits[i].x = 6 + game_rand() % (DUEL_PANE_W - 12);
            p->fruits[i].y = -5;
            p->fruits[i].dy = DUEL_FRUIT_DY;
            p->fruits[i].color = random_fruit_color();
            return;
        }
    }
}

static void update_fruits_pane(DuelPane *p, int frame_counter)
{
    for (int i = 0; i < DUEL_MAX_FRUITS; i++) {
        Fruit *f = &p->fruits[i];
        if (!f->active) {
            continue;
        }
        if (frame_counter % DUEL_FRUIT_MOVE_EVERY == 0) {
            f->y += f->dy;
        }
        if (fruit_hits_basket(f, &p->basket)) {
            f->active = false;
            p->score++;
        } else if (f->y - f->r > DUEL_PANE_H) {
            // Fruit left the bottom of this pane without a catch.
            f->active = false;
            p->lives--;
        }
    }
}

void update_duel(DuelState *d)
{
    d->frame_counter++;

    // Left follows the physical keys, right follows injected agent bits.
    update_basket_pane(&d->left, read_human_movement_keys());
    update_basket_pane(&d->right, input_get_agent_keys());

    if (d->frame_counter % DUEL_SPAWN_EVERY == 0) {
        spawn_fruit_pane(&d->left);
        spawn_fruit_pane(&d->right);
    }

    update_fruits_pane(&d->left, d->frame_counter);
    update_fruits_pane(&d->right, d->frame_counter);

    if (d->left.lives <= 0 || d->right.lives <= 0) {
        d->running = false;
    }
}

// origin_x shifts drawing into the left or right half of the wide framebuffer.
static void draw_basket_at(const Basket *b, int origin_x)
{
    draw_rect(origin_x + b->x, b->y, b->w, b->h, BROWN);
    draw_rect(origin_x + b->x - 2, b->y - 2, b->w + 4, 2, YELLOW);
}

static void draw_fruit_at(const Fruit *f, int origin_x)
{
    if (!f->active) {
        return;
    }
    // Circle plus stem pixels, same motif as solo draw_fruit.
    draw_circle(origin_x + f->x, f->y, f->r, f->color);
    plot_pixel(origin_x + f->x, f->y - f->r - 1, GREEN);
    plot_pixel(origin_x + f->x, f->y - f->r - 2, GREEN);
}

void draw_duel(DuelState *d)
{
    // Full width floor strip under both panes.
    draw_rect(0, DUEL_PANE_H - 2, SCREEN_WIDTH, 2, GRAY);
    // Vertical divider between the two 160 pixel wide fields.
    draw_rect(158, 0, 4, DUEL_PANE_H, GRAY);

    draw_basket_at(&d->left.basket, DUEL_LEFT_ORIGIN_X);
    draw_basket_at(&d->right.basket, DUEL_RIGHT_ORIGIN_X);
    for (int i = 0; i < DUEL_MAX_FRUITS; i++) {
        draw_fruit_at(&d->left.fruits[i], DUEL_LEFT_ORIGIN_X);
        draw_fruit_at(&d->right.fruits[i], DUEL_RIGHT_ORIGIN_X);
    }

    // Compact HUD for each side.
    draw_rect(5, 5, d->left.score * 3, 5, GREEN);
    draw_rect(5, 13, d->left.lives * 8, 5, RED);
    draw_rect(165, 5, d->right.score * 3, 5, GREEN);
    draw_rect(165, 13, d->right.lives * 8, 5, RED);
}
