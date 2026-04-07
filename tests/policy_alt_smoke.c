// Like policy_smoke but exercises the float alt network headers.
// Uses POLICY_ALT_OBS_DIM and the F32 tables from policy_weights_alt.h.
// Build with make policy_alt_smoke from repo root.

#include <stdio.h>

#include "policy_alt.h"

int main(void)
{
    float obs[POLICY_ALT_OBS_DIM];
    for (int i = 0; i < POLICY_ALT_OBS_DIM; i++) {
        obs[i] = 0.25f;
    }
    float logits[POLICY_ALT_N_ACTION];
    policy_alt_forward_logits(obs, logits);
    // Space separated list, length follows POLICY_ALT_N_ACTION.
    for (int i = 0; i < POLICY_ALT_N_ACTION; i++) {
        printf("%s%f", i ? " " : "", logits[i]);
    }
    printf("\n");
    return 0;
}
