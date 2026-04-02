#ifndef OBSERVATION_DUEL_H
#define OBSERVATION_DUEL_H

#include "duel_game.h"

#define DUEL_GAME_OBS_DIM 26

void build_duel_right_observation(const DuelState *d, float obs[DUEL_GAME_OBS_DIM]);

#endif
