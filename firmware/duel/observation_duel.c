// Float obs for duel policy, right pane only, layout matches solo packing.

#include "observation_duel.h"

#include <stdlib.h>

#include "duel_game.h"

// Sort scratch, src_i keeps qsort stable when y and x tie.
typedef struct {
    int y;
    int x;
    int src_i;
} ActiveFruit;

// Identical sort key idea as build_game_observation in solo code.
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
    // Deterministic tie break if y and x match.
    return a->src_i - b->src_i;
}

void build_duel_right_observation(const DuelState *d, float obs[DUEL_GAME_OBS_DIM])
{
    const DuelPane *p = &d->right;
    int max_x = DUEL_PANE_W - DUEL_BASKET_W;
    // Basket x normalized over its lane inside the right pane.
    obs[0] = max_x > 0 ? (float)p->basket.x / (float)max_x : 0.f;

    ActiveFruit act[DUEL_MAX_FRUITS];
    int n = 0;
    for (int i = 0; i < DUEL_MAX_FRUITS; i++) {
        const Fruit *f = &p->fruits[i];
        if (f->active) {
            act[n].y = f->y;
            act[n].x = f->x;
            act[n].src_i = i;
            n++;
        }
    }
    if (n > 1) {
        // Deepest fruit first so slot zero is what the policy should track.
        qsort(act, (size_t)n, sizeof act[0], cmp_active_fruit);
    }

    // Five floats per fruit slot, same layout as the full screen trainer.
    for (int slot = 0; slot < DUEL_MAX_FRUITS; slot++) {
        int o = 1 + slot * 5;
        if (slot < n) {
            const Fruit *f = &p->fruits[act[slot].src_i];
            obs[o + 0] = 1.f;
            obs[o + 1] = (float)f->x / (float)DUEL_PANE_W;
            float yn = ((float)f->y + 32.f) / ((float)DUEL_PANE_H + 64.f);
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
            // Padding for unused fruit slots in the fixed width vector.
            obs[o + 0] = 0.f;
            obs[o + 1] = 0.f;
            obs[o + 2] = 0.f;
            obs[o + 3] = 0.f;
            obs[o + 4] = 0.f;
        }
    }
}
