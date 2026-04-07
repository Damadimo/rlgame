// Compile and run duel policy on the host with a toy observation vector.
// Exit nonzero if the chosen action is outside the three way discrete set.
// Build with make policy_duel_smoke from repo root.

#include <stdio.h>

#include "policy_duel.h"

int main(void)
{
    float obs[DUEL_POLICY_OBS_DIM];
    // Mild variation so the matmul is not all zeros, still deterministic.
    for (int i = 0; i < DUEL_POLICY_OBS_DIM; i++) {
        obs[i] = 0.1f * (float)(i % 7);
    }
    int a = policy_duel_select_action(obs);
    printf("policy_duel_select_action -> %d\n", a);
    // Actions are 0 stay, 1 left, 2 right for this project.
    return a < 0 || a > 2 ? 1 : 0;
}
