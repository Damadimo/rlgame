# RL contract (training and RV32 deployment)

This document locks the interface between the Gymnasium environment, training, and embedded inference so behavior matches [game.c](../../firmware/solo/game.c).

## Constants (from [game.h](../../firmware/solo/game.h) and [graphics.h](../../firmware/shared/graphics.h))

| Symbol | Value |
|--------|-------|
| `SCREEN_WIDTH` | 320 |
| `SCREEN_HEIGHT` | 240 |
| `MAX_FRUITS` | 5 |
| `FRUIT_DY` | 1 (nominal step added to `y` when a fruit moves) |
| `FRUIT_MOVE_EVERY` | 2 — apply `y += dy` only when `frame_counter % N == 0` (slower effective fall) |
| `INITIAL_LIVES` | 10 |
| `BASKET_WIDTH` | 36 |
| `BASKET_HEIGHT` | 10 |
| `BASKET_SPEED` | 3 |
| Spawn interval | every 25 **incremented** frames (`frame_counter % 25 == 0` after `frame_counter++`) |

## Action space

Discrete **3** actions (Gymnasium `Discrete(3)`):

| Action ID | Meaning | Key bitmask (same as [game.c](../../firmware/solo/game.c) `update_basket`) |
|-----------|---------|-------------------------|
| 0 | No move | `0` |
| 1 | Left | `0x8` |
| 2 | Right | `0x4` |

`update_game` order per step: `frame_counter++` → basket movement from keys → spawn if `frame_counter % 25 == 0` → for each active fruit, if `frame_counter % FRUIT_MOVE_EVERY == 0` then `y += dy` → collision / miss resolution.

## Random number generator

Spawning uses the same generator as [rng.c](../../firmware/shared/rng.c): 32-bit state, `next = next * 1103515245 + 12345`, return `(next >> 16) & 0x7fff` (classic `rand()`-style). **Before** each episode, call `game_rng_seed(seed)` (C) or set Python state to match.

## Observation vector

Length **26** (`1 + 5 * 5`), `float32`, order:

1. **Basket** (1): `basket.x / (SCREEN_WIDTH - BASKET_WIDTH)` in \([0, 1]\).
2. **Fruit slots `0..4` are sorted by urgency**, not by internal array index:
   - Take all active fruits from `game->fruits[]`.
   - Sort by **descending** `y` (larger `y` = lower on screen = more urgent), then ascending `x`, then ascending original slot index (deterministic tie-break).
   - Fill up to **5** slots with that order; remaining slots are all zeros (inactive).
   - For each filled slot, the 5 floats are:
     - `active`: `1.0`
     - `x_norm`: `x / SCREEN_WIDTH`
     - `y_norm`: `(y + 32) / (SCREEN_HEIGHT + 64)` clipped to \([0, 1]\)
     - `dy_norm`: `dy / 3.0` (`dy` is always `FRUIT_DY` = 1)
     - `r_norm`: `r / 8.0` (typical r is 4–6)

Normalization matches [policy.c](../../firmware/solo/policy.c) / training preprocessing. [observation.c](../../firmware/solo/observation.c) and [catch_env.py](catch_env.py) must use the **same** sort rule.

## Rewards

- **+1.0** when a fruit is caught (`score` increases).
- **-1.0** when a fruit is missed (`lives` decreases).
- **0.0** otherwise.

Episode terminates when `lives <= 0` (`running == false`). Starting lives = `INITIAL_LIVES` (10).

## Policy output on device

Inference produces logits or Q-values for 3 actions; take `argmax`. Map to key bits for `input_set_agent_keys()` / `read_movement_keys()` in agent mode: `0`, `0x8`, or `0x4`.

## Fixed-point inference

Weights are exported as **int8** with per-layer scale; activations use **int32** accumulators and **ReLU** after hidden layers. See [policy.c](../../firmware/solo/policy.c). `GAME_OBS_DIM` in [observation.h](../../firmware/solo/observation.h) must equal `POLICY_OBS_DIM` from generated [policy_weights.h](../../weights/policy_weights.h) (both are **26** with the current layout).

## Firmware entry points

- Human play: [main.c](../../firmware/solo/main.c) with `INPUT_MODE_HUMAN` and `read_movement_keys()` reading keys.
- Agent on device: [main_agent.c](../../firmware/solo/main_agent.c) — `INPUT_MODE_AGENT`, `build_game_observation` → `policy_select_action` → `input_set_agent_keys` before `update_game`.
