#ifndef OBSERVATION_H
#define OBSERVATION_H

#include "game.h"

#define GAME_OBS_DIM 51

void build_game_observation(const GameState *game, float obs[GAME_OBS_DIM]);

#endif
