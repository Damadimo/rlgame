// Quantized duel policy MLP, same math as policy.c, duel sized tables.

#include "policy_duel.h"

#include <math.h>
#include <stdint.h>

#ifndef DUEL_POLICY_OBS_DIM
#error "DUEL_POLICY_OBS_DIM must be defined by policy_weights_duel.h"
#endif

// Nonlinearity used after each hidden affine map.
static float relu(float x)
{
    return x > 0.f ? x : 0.f;
}

void policy_duel_forward_logits(const float obs[DUEL_POLICY_OBS_DIM], float logits[DUEL_POLICY_N_ACTION])
{
    // Same layout as policy.c but dimensions come from policy_weights_duel.h.
    int32_t xq[DUEL_POLICY_OBS_DIM];
    // Scale each observation channel into sixteen bit range for weight multiply.
    for (int i = 0; i < DUEL_POLICY_OBS_DIM; i++) {
        float v = obs[i] * (float)DUEL_POLICY_IN_Q;
        if (v > 32767.f) {
            v = 32767.f;
        }
        if (v < -32768.f) {
            v = -32768.f;
        }
        xq[i] = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
    }

    float hidden1[DUEL_POLICY_H1];
    // Layer one, rows of W1 times xq, bias and scale from the header macros.
    for (int j = 0; j < DUEL_POLICY_H1; j++) {
        int64_t acc = 0;
        for (int k = 0; k < DUEL_POLICY_OBS_DIM; k++) {
            acc += (int64_t)DUEL_POLICY_W1[j * DUEL_POLICY_OBS_DIM + k] * (int64_t)xq[k];
        }
        float z = (float)acc * DUEL_POLICY_SCALE1 * DUEL_POLICY_INV_IN_Q + DUEL_POLICY_B1[j];
        hidden1[j] = relu(z);
    }

    int32_t y1q[DUEL_POLICY_H1];
    // Quantize relu outputs so layer two also uses integer weights.
    for (int j = 0; j < DUEL_POLICY_H1; j++) {
        float v = hidden1[j] * (float)DUEL_POLICY_IN_Q;
        if (v > 32767.f) {
            v = 32767.f;
        }
        if (v < -32768.f) {
            v = -32768.f;
        }
        y1q[j] = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
    }

    float hidden2[DUEL_POLICY_H2];
    // Second hidden, DUEL_POLICY_W2 and DUEL_POLICY_B2.
    for (int j = 0; j < DUEL_POLICY_H2; j++) {
        int64_t acc = 0;
        for (int k = 0; k < DUEL_POLICY_H1; k++) {
            acc += (int64_t)DUEL_POLICY_W2[j * DUEL_POLICY_H1 + k] * (int64_t)y1q[k];
        }
        float z = (float)acc * DUEL_POLICY_SCALE2 * DUEL_POLICY_INV_IN_Q + DUEL_POLICY_B2[j];
        hidden2[j] = relu(z);
    }

    int32_t y2q[DUEL_POLICY_H2];
    // Same quantize step as after hidden1.
    for (int j = 0; j < DUEL_POLICY_H2; j++) {
        float v = hidden2[j] * (float)DUEL_POLICY_IN_Q;
        if (v > 32767.f) {
            v = 32767.f;
        }
        if (v < -32768.f) {
            v = -32768.f;
        }
        y2q[j] = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
    }

    // Final logits stay float, no relu, one value per action id.
    for (int j = 0; j < DUEL_POLICY_N_ACTION; j++) {
        int64_t acc = 0;
        for (int k = 0; k < DUEL_POLICY_H2; k++) {
            acc += (int64_t)DUEL_POLICY_WA[j * DUEL_POLICY_H2 + k] * (int64_t)y2q[k];
        }
        logits[j] = (float)acc * DUEL_POLICY_SCALE_OUT * DUEL_POLICY_INV_IN_Q + DUEL_POLICY_BA[j];
    }
}

// Same argmax contract as policy_select_action on the solo side.
int policy_duel_select_action(const float obs[DUEL_POLICY_OBS_DIM])
{
    float logits[DUEL_POLICY_N_ACTION];
    policy_duel_forward_logits(obs, logits);
    int best = 0;
    // Deterministic pick for deployment, matches greedy eval when exporting.
    for (int i = 1; i < DUEL_POLICY_N_ACTION; i++) {
        if (logits[i] > logits[best]) {
            best = i;
        }
    }
    return best;
}
