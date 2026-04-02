#include "observation_duel.h"

#include <stdlib.h>

#include "duel_game.h"

typedef struct {
    int y;
    int x;
    int src_i;
} ActiveFruit;

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
    return a->src_i - b->src_i;
}

void build_duel_right_observation(const DuelState *d, float obs[DUEL_GAME_OBS_DIM])
{
    const DuelPane *p = &d->right;
    int max_x = DUEL_PANE_W - DUEL_BASKET_W;
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
        qsort(act, (size_t)n, sizeof act[0], cmp_active_fruit);
    }

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
            obs[o + 0] = 0.f;
            obs[o + 1] = 0.f;
            obs[o + 2] = 0.f;
            obs[o + 3] = 0.f;
            obs[o + 4] = 0.f;
        }
    }
}
