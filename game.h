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
    short int color;               // RGB color randomized 
    bool active;                   // true if fruit is currently on screen
} Fruit;

typedef struct {
    Basket basket;
    Fruit fruits[MAX_FRUITS];       // arrays of fruits (can or cannot be active)
    int score;                      // number of fruits caught
    int lives;                      // decrements when fruit is missed 
    int frame_counter;              // total frames elapsed used for spawning fruits
    bool running;
} GameState;

void init_game(GameState *game);    // sets all variables/parameters to starting value
void update_game(GameState *game);  // game logic updated 1/60th of a second
void draw_game(GameState *game);    // render the current frame/state into back buffer 

#endif