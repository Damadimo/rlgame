#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void game_rng_seed(uint32_t s);
int game_rand(void);

#endif
