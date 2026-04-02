/* Print alt-policy logits for a fixed observation (host validation). */
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
    for (int i = 0; i < POLICY_ALT_N_ACTION; i++) {
        printf("%s%f", i ? " " : "", logits[i]);
    }
    printf("\n");
    return 0;
}
