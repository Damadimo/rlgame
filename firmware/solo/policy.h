#ifndef POLICY_H
#define POLICY_H

#include <stdint.h>

#include "policy_weights.h"

void policy_forward_logits(const float obs[POLICY_OBS_DIM], float logits[POLICY_N_ACTION]);
int policy_select_action(const float obs[POLICY_OBS_DIM]);

#endif
