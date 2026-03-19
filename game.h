#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAX_FRUITS 10
#define BASKET_WIDTH  36
#define BASKET_HEIGHT 10
#define BASKET_SPEED  3

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Basket;

typedef struct {
    int x;
    int y;
    int r;
    int dy;
    short int color;
    bool active;
} Fruit;

typedef struct {
    Basket basket;
    Fruit fruits[MAX_FRUITS];
    int score;
    int lives;
    int frame_counter;
    bool running;
} GameState;

void init_game(GameState *game);
void update_game(GameState *game);
void draw_game(GameState *game);

#endif