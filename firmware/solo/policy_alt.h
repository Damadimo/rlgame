#ifndef POLICY_ALT_H
#define POLICY_ALT_H

#include "observation.h"
#include "policy_weights_alt.h"

/** Full float forward; obs length is POLICY_ALT_OBS_DIM (see policy_weights_alt.h). */
void policy_alt_forward_logits(const float obs[POLICY_ALT_OBS_DIM], float logits[POLICY_ALT_N_ACTION]);
int policy_alt_select_action(const float obs[POLICY_ALT_OBS_DIM]);

/**
 * Uses the same 26-D vector as the default policy; alt net input order is:
 * closest-fruit x, closest-fruit y, basket x (from full_obs[2], [3], [0] — first sorted fruit slot).
 */
int policy_alt_select_action_from_game_obs(const float full_obs[GAME_OBS_DIM]);

#endif
