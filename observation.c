#include "observation.h"

#include "game.h"
#include "graphics.h"

void build_game_observation(const GameState *game, float obs[POLICY_OBS_DIM])
{
    int max_x = SCREEN_WIDTH - BASKET_WIDTH;
    obs[0] = max_x > 0 ? (float)game->basket.x / (float)max_x : 0.f;

    for (int i = 0; i < MAX_FRUITS; i++) {
        int o = 1 + i * 5;
        const Fruit *f = &game->fruits[i];
        if (f->active) {
            obs[o + 0] = 1.f;
            obs[o + 1] = (float)f->x / (float)SCREEN_WIDTH;
            float yn = ((float)f->y + 32.f) / ((float)SCREEN_HEIGHT + 64.f);
            if (yn < 0.f) {
                yn = 0.f;
            }
            if (yn > 1.f) {
                yn = 1.f;
            }
            obs[o + 2] = yn;
            obs[o + 3] = (float)f->dy / 3.f;
            obs[o + 4] = (float)f->r / 8.f;
        } else {
            obs[o + 0] = 0.f;
            obs[o + 1] = 0.f;
            obs[o + 2] = 0.f;
            obs[o + 3] = 0.f;
            obs[o + 4] = 0.f;
        }
    }
}
