// Packs GameState into the float vector exported to policy_weights.h training.

#include "observation.h"

#include <stdlib.h>

#include "game.h"
#include "graphics.h"

// Scratch tuple for qsort, src_i breaks ties so order stays stable.
typedef struct {
    int y;
    int x;
    int src_i;
} ActiveFruit;

// Sort lower y first so the nearest threat to the basket fills slot zero.
static int cmp_active_fruit(const void *aa, const void *bb)
{
    const ActiveFruit *a = (const ActiveFruit *)aa;
    const ActiveFruit *b = (const ActiveFruit *)bb;
    if (a->y != b->y) {
        return b->y - a->y;
    }
    if (a->x != b->x) {
        return a->x - b->x;
    }
    // Stable ordering when two fruits share the same cell.
    return a->src_i - b->src_i;
}

void build_game_observation(const GameState *game, float obs[GAME_OBS_DIM])
{
    int max_x = SCREEN_WIDTH - BASKET_WIDTH;
    // Normalized basket position along its travel range.
    obs[0] = max_x > 0 ? (float)game->basket.x / (float)max_x : 0.f;

    ActiveFruit act[MAX_FRUITS];
    int n = 0;
    for (int i = 0; i < MAX_FRUITS; i++) {
        const Fruit *f = &game->fruits[i];
        if (f->active) {
            act[n].y = f->y;
            act[n].x = f->x;
            act[n].src_i = i;
            n++;
        }
    }
    if (n > 1) {
        // Highest y first so slot zero is the most urgent fruit.
        qsort(act, (size_t)n, sizeof act[0], cmp_active_fruit);
    }

    // Pack up to MAX_FRUITS blocks of five floats after the basket slot.
    for (int slot = 0; slot < MAX_FRUITS; slot++) {
        int o = 1 + slot * 5;
        if (slot < n) {
            const Fruit *f = &game->fruits[act[slot].src_i];
            obs[o + 0] = 1.f;
            obs[o + 1] = (float)f->x / (float)SCREEN_WIDTH;
            // Soft vertical normalization with small margin, clamp to unit range.
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
            // Empty slots are explicit zeros so the net sees padding.
            obs[o + 0] = 0.f;
            obs[o + 1] = 0.f;
            obs[o + 2] = 0.f;
            obs[o + 3] = 0.f;
            obs[o + 4] = 0.f;
        }
    }
}
