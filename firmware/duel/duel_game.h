#ifndef DUEL_GAME_H
#define DUEL_GAME_H

#include <stdbool.h>

#include "game.h"

/* Full VGA remains 320x240; each player gets a 160px-wide vertical pane. */
#define DUEL_MAX_FRUITS 5
#define DUEL_PANE_W 160
#define DUEL_PANE_H 240
#define DUEL_FRUIT_DY 1
#define DUEL_FRUIT_MOVE_EVERY 2
#define DUEL_BASKET_W 18
#define DUEL_BASKET_H 10
#define DUEL_BASKET_SPEED 2
#define DUEL_SPAWN_EVERY 22
#define DUEL_INITIAL_LIVES 8
#define DUEL_LEFT_ORIGIN_X 0
#define DUEL_RIGHT_ORIGIN_X 160

typedef struct {
    Basket basket;
    Fruit fruits[DUEL_MAX_FRUITS];
    int score;
    int lives;
} DuelPane;

typedef struct {
    DuelPane left;
    DuelPane right;
    int frame_counter;
    bool running;
} DuelState;

void init_duel(DuelState *d);
void update_duel(DuelState *d);
void draw_duel(DuelState *d);

#endif
