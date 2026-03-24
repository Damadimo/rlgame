# RL contract (training and RV32 deployment)

This document locks the interface between the Gymnasium environment, training, and embedded inference so behavior matches [game.c](../game.c).

## Constants (from [game.h](../game.h) and [graphics.h](../graphics.h))

| Symbol | Value |
|--------|-------|
| `SCREEN_WIDTH` | 320 |
| `SCREEN_HEIGHT` | 240 |
| `MAX_FRUITS` | 10 |
| `BASKET_WIDTH` | 36 |
| `BASKET_HEIGHT` | 10 |
| `BASKET_SPEED` | 3 |
| Spawn interval | every 25 **incremented** frames (`frame_counter % 25 == 0` after `frame_counter++`) |

## Action space

Discrete **3** actions (Gymnasium `Discrete(3)`):

| Action ID | Meaning | Key bitmask (same as [game.c](game.c) `update_basket`) |
|-----------|---------|-------------------------|
| 0 | No move | `0` |
| 1 | Left | `0x8` |
| 2 | Right | `0x4` |

`update_game` order per step: `frame_counter++` → basket movement from keys → spawn if `frame_counter % 25 == 0` → move fruits and resolve collisions.

## Random number generator

Spawning uses the same generator as [rng.c](../rng.c): 32-bit state, `next = next * 1103515245 + 12345`, return `(next >> 16) & 0x7fff` (classic `rand()`-style). **Before** each episode, call `game_rng_seed(seed)` (C) or set Python state to match.

## Observation vector

Length **51**, `float32`, order:

1. **Basket** (1): `basket.x / (SCREEN_WIDTH - BASKET_WIDTH)` in \([0, 1]\).
2. For each fruit slot `i = 0..9` (5 each):
   - `active`: `1.0` if `fruits[i].active` else `0.0`
   - `x_norm`: `fruits[i].x / SCREEN_WIDTH` (0 if inactive)
   - `y_norm`: `(fruits[i].y + 32) / (SCREEN_HEIGHT + 64)` clipped to \([0, 1]\) (inactive → `0`)
   - `dy_norm`: `fruits[i].dy / 3.0` (inactive → `0`; max dy is 3)
   - `r_norm`: `fruits[i].r / 8.0` (inactive → `0`; typical r is 4–6)

Normalization matches [policy.c](../policy.c) / training preprocessing.

## Rewards

- **+1.0** when a fruit is caught (`score` increases).
- **-1.0** when a fruit is missed (`lives` decreases).
- **0.0** otherwise.

Episode terminates when `lives <= 0` (`running == false`).

## Policy output on device

Inference produces logits or Q-values for 3 actions; take `argmax`. Map to key bits for `input_set_agent_keys()` / `read_movement_keys()` in agent mode: `0`, `0x8`, or `0x4`.

## Fixed-point inference

Weights are exported as **int8** with per-layer scale; activations use **int32** accumulators and **ReLU** after hidden layers. See [policy.c](../policy.c). `GAME_OBS_DIM` in [observation.h](../observation.h) must equal `POLICY_OBS_DIM` from generated [policy_weights.h](../policy_weights.h) (both are 51 with the current layout).

## Firmware entry points

- Human play: [main.c](../main.c) with `INPUT_MODE_HUMAN` and `read_movement_keys()` reading keys.
- Agent on device: [main_agent.c](../main_agent.c) — `INPUT_MODE_AGENT`, `build_game_observation` → `policy_select_action` → `input_set_agent_keys` before `update_game`.
