/* Print policy logits for a fixed observation (host validation). */
#include <stdio.h>

#include "policy.h"

int main(void)
{
    float obs[POLICY_OBS_DIM];
    for (int i = 0; i < POLICY_OBS_DIM; i++) {
        obs[i] = 0.25f;
    }
    float logits[POLICY_N_ACTION];
    policy_forward_logits(obs, logits);
    printf("%f %f %f\n", logits[0], logits[1], logits[2]);
    return 0;
}
