/* Host smoke: link policy_duel with placeholder policy_weights_duel.h */
#include <stdio.h>
#include <stdlib.h>

#include "policy_duel.h"

int main(void)
{
    float obs[DUEL_POLICY_OBS_DIM];
    for (int i = 0; i < DUEL_POLICY_OBS_DIM; i++) {
        obs[i] = 0.1f * (float)(i % 7);
    }
    int a = policy_duel_select_action(obs);
    printf("policy_duel_select_action -> %d\n", a);
    return a < 0 || a > 2 ? 1 : 0;
}
