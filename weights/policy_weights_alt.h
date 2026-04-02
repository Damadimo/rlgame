/* Float MLP for alternate solo policy: 3 inputs -> 8 ReLU -> 3 logits (no ReLU on output). */
#ifndef POLICY_WEIGHTS_ALT_H
#define POLICY_WEIGHTS_ALT_H

#define POLICY_ALT_OBS_DIM 3
#define POLICY_ALT_H1 8
#define POLICY_ALT_N_ACTION 3

/* Row-major: W1[j*3+i] is weight input i -> hidden j. */
static const float POLICY_ALT_F32_W1[24] = {
    -0.447534f, 0.005736f, 0.537721f, -0.544856f, -0.569693f, -0.303467f,
    -0.211172f, -0.173164f, 0.135065f, 0.128103f, -0.606072f, 0.023503f,
    -0.340672f, -0.238715f, 0.129875f, -0.436114f, 0.394019f, 1.038354f,
    1.025600f, 0.034123f, -0.447495f, 1.394727f, 0.541149f, -0.660892f
};

static const float POLICY_ALT_F32_B1[8] = {
    -0.536002f, -0.059374f, 0.709799f, 0.102546f, -0.158742f, 0.805140f,
    -0.033526f, -0.377667f
};

/* Row-major: W2[k*8+j] is weight hidden j -> logit k. */
static const float POLICY_ALT_F32_W2[24] = {
    0.296222f, -0.216376f, 0.426011f, -0.340751f, -0.172694f, 0.770041f,
    -1.170538f, -1.222931f, 0.308591f, -0.054581f, 0.072057f, 0.169502f,
    -0.251762f, -0.010840f, -0.129970f, -0.180465f, 0.226623f, -0.175656f,
    0.125003f, 0.261819f, 0.022158f, -0.378560f, 0.865844f, 1.331719f
};

static const float POLICY_ALT_F32_B2[3] = {
    0.043234f, -0.481264f, 0.151517f
};

#endif
