/*
calls ai_get_action once per frame in the game loop, passes the closest fruit's position
and the basket's x position

returns either:
    - 0 left
    - 1 stay
    - 2 right

function does two matrix multiplications using the trained weights in weights.h
*/

#ifndef AI_H
#define AI_H

int ai_get_action(int fruit_x, int fruit_y, int basket_x);

#endif