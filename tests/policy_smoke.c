// Print three logits for a constant obs vector to compare with Python export.
// Handy after regenerating weights/policy_weights.h to spot accidental drift.
// Build with make targets at repo root, see Makefile.

#include <stdio.h>

#include "policy.h"

int main(void)
{
    float obs[POLICY_OBS_DIM];
    // Flat input makes hand checking and diffing easy.
    for (int i = 0; i < POLICY_OBS_DIM; i++) {
        obs[i] = 0.25f;
    }
    float logits[POLICY_N_ACTION];
    policy_forward_logits(obs, logits);
    // One line per run so shell scripts can parse or diff stdout.
    printf("%f %f %f\n", logits[0], logits[1], logits[2]);
    return 0;
}
