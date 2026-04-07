// Float MLP alt policy from policy_weights_alt.h, fewer params than default net.

#include "policy_alt.h"

#ifndef POLICY_ALT_OBS_DIM
#error "POLICY_ALT_OBS_DIM must be defined by policy_weights_alt.h"
#endif

// Matches hidden layer nonlinearity in the exported torch graph.
static float relu(float x)
{
    return x > 0.f ? x : 0.f;
}

void policy_alt_forward_logits(const float obs[POLICY_ALT_OBS_DIM], float logits[POLICY_ALT_N_ACTION])
{
    float h[POLICY_ALT_H1];
    // First layer is a plain float dot product plus bias.
    for (int j = 0; j < POLICY_ALT_H1; j++) {
        float z = POLICY_ALT_F32_B1[j];
        for (int i = 0; i < POLICY_ALT_OBS_DIM; i++) {
            z += POLICY_ALT_F32_W1[j * POLICY_ALT_OBS_DIM + i] * obs[i];
        }
        h[j] = relu(z);
    }

    // Linear readout to one logit per discrete action.
    for (int k = 0; k < POLICY_ALT_N_ACTION; k++) {
        float z = POLICY_ALT_F32_B2[k];
        for (int j = 0; j < POLICY_ALT_H1; j++) {
            z += POLICY_ALT_F32_W2[k * POLICY_ALT_H1 + j] * h[j];
        }
        logits[k] = z;
    }
}

int policy_alt_select_action(const float obs[POLICY_ALT_OBS_DIM])
{
    float logits[POLICY_ALT_N_ACTION];
    policy_alt_forward_logits(obs, logits);
    int best = 0;
    // Greedy policy for deployment.
    for (int i = 1; i < POLICY_ALT_N_ACTION; i++) {
        if (logits[i] > logits[best]) {
            best = i;
        }
    }
    return best;
}

// Alt training used three inputs in a different order than the full vector packs them.
int policy_alt_select_action_from_game_obs(const float full_obs[GAME_OBS_DIM])
{
    float alt[POLICY_ALT_OBS_DIM];
    // Closest fruit x and y then basket x, see policy_alt.h notes.
    alt[0] = full_obs[2];
    alt[1] = full_obs[3];
    alt[2] = full_obs[0];
    return policy_alt_select_action(alt);
}
