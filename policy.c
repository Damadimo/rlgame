#include "policy.h"

#include <math.h>
#include <stdint.h>

#ifndef POLICY_OBS_DIM
#error "POLICY_OBS_DIM must be defined by policy_weights.h"
#endif

static float relu(float x)
{
    return x > 0.f ? x : 0.f;
}

void policy_forward_logits(const float obs[POLICY_OBS_DIM], float logits[POLICY_N_ACTION])
{
    int32_t xq[POLICY_OBS_DIM];
    for (int i = 0; i < POLICY_OBS_DIM; i++) {
        float v = obs[i] * (float)POLICY_IN_Q;
        if (v > 32767.f) {
            v = 32767.f;
        }
        if (v < -32768.f) {
            v = -32768.f;
        }
        xq[i] = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
    }

    float hidden1[POLICY_H1];
    for (int j = 0; j < POLICY_H1; j++) {
        int64_t acc = 0;
        for (int k = 0; k < POLICY_OBS_DIM; k++) {
            acc += (int64_t)POLICY_W1[j * POLICY_OBS_DIM + k] * (int64_t)xq[k];
        }
        float z = (float)acc * POLICY_SCALE1 * POLICY_INV_IN_Q + POLICY_B1[j];
        hidden1[j] = relu(z);
    }

    int32_t y1q[POLICY_H1];
    for (int j = 0; j < POLICY_H1; j++) {
        float v = hidden1[j] * (float)POLICY_IN_Q;
        if (v > 32767.f) {
            v = 32767.f;
        }
        if (v < -32768.f) {
            v = -32768.f;
        }
        y1q[j] = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
    }

    float hidden2[POLICY_H2];
    for (int j = 0; j < POLICY_H2; j++) {
        int64_t acc = 0;
        for (int k = 0; k < POLICY_H1; k++) {
            acc += (int64_t)POLICY_W2[j * POLICY_H1 + k] * (int64_t)y1q[k];
        }
        float z = (float)acc * POLICY_SCALE2 * POLICY_INV_IN_Q + POLICY_B2[j];
        hidden2[j] = relu(z);
    }

    int32_t y2q[POLICY_H2];
    for (int j = 0; j < POLICY_H2; j++) {
        float v = hidden2[j] * (float)POLICY_IN_Q;
        if (v > 32767.f) {
            v = 32767.f;
        }
        if (v < -32768.f) {
            v = -32768.f;
        }
        y2q[j] = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
    }

    for (int j = 0; j < POLICY_N_ACTION; j++) {
        int64_t acc = 0;
        for (int k = 0; k < POLICY_H2; k++) {
            acc += (int64_t)POLICY_WA[j * POLICY_H2 + k] * (int64_t)y2q[k];
        }
        logits[j] = (float)acc * POLICY_SCALE_OUT * POLICY_INV_IN_Q + POLICY_BA[j];
    }
}

int policy_select_action(const float obs[POLICY_OBS_DIM])
{
    float logits[POLICY_N_ACTION];
    policy_forward_logits(obs, logits);
    int best = 0;
    for (int i = 1; i < POLICY_N_ACTION; i++) {
        if (logits[i] > logits[best]) {
            best = i;
        }
    }
    return best;
}
