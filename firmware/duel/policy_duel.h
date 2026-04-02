#ifndef POLICY_DUEL_H
#define POLICY_DUEL_H

#include <stdint.h>

#include "policy_weights_duel.h"

void policy_duel_forward_logits(const float obs[DUEL_POLICY_OBS_DIM], float logits[DUEL_POLICY_N_ACTION]);
int policy_duel_select_action(const float obs[DUEL_POLICY_OBS_DIM]);

#endif
