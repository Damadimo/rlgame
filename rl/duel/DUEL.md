# Split-screen duel (human vs AI)

## VGA layout

The full framebuffer stays **320×240**. The screen is split into two **160×240** playfields:

- **Left (x = 0…159):** human (`read_human_movement_keys()`).
- **Right (x = 160…319):** AI (`input_set_agent_keys` / `input_get_agent_keys()`).
- A **4-pixel** gray divider at **x = 158**.

## C sources

| Role | Path |
|------|------|
| Duel logic | `firmware/duel/duel_game.c`, `duel_game.h` |
| Right-pane observation | `firmware/duel/observation_duel.c`, `observation_duel.h` |
| Policy inference | `firmware/duel/policy_duel.c`, `policy_duel.h` |
| Entry point | `firmware/duel/main_duel.c` (duel-only) or unified menu `firmware/main_game.c` |
| Shared (VGA, keys, RNG) | `firmware/shared/*` |
| Types (`Basket`, `Fruit`) | `firmware/solo/game.h` (included by duel) |

## C constants (`duel_game.h`)

Match these in Python (`rl/duel/duel_env.py`) if you change firmware:

| Symbol | Value |
|--------|--------|
| `DUEL_PANE_W` / `DUEL_PANE_H` | 160 / 240 |
| `DUEL_BASKET_W` | 18 |
| `DUEL_BASKET_H` | 10 |
| `DUEL_BASKET_SPEED` | 2 |
| `DUEL_SPAWN_EVERY` | 22 |
| `DUEL_FRUIT_DY` | 1 |
| `DUEL_FRUIT_MOVE_EVERY` | 2 |
| `DUEL_INITIAL_LIVES` | 8 |
| Fruit radius | `2 + rand % 3` |

Episode ends when **either** side has `lives <= 0`.

## Observation (right AI only)

`observation_duel.c` builds a **26-D** vector for the **right** pane only (same layout as solo `observation.c`, but normalized with `DUEL_PANE_W` / `DUEL_PANE_H`).

## Policy weights

- **`weights/policy_weights_duel.h`** — duel AI (separate from solo **`weights/policy_weights.h`**).
- The generated header uses the **`DUEL_POLICY_*`** symbol prefix (e.g. `DUEL_POLICY_W1`, `DUEL_POLICY_OBS_DIM`) so it can be linked in the same binary as solo **`POLICY_*`** weights (`firmware/main_game.c` unified build).

## Training (Python)

From the **repository root**:

```bash
python -m rl.duel.train_duel
```

This writes **`rl/models/ppo_duel_catch.zip`**, uses **`rl/models/best_ppo_duel/best_model.zip`** for export when eval is enabled, and generates **`weights/policy_weights_duel.h`**.

Defaults vs solo `rl.solo.train`:

- **`--ent-coef 0.005`** (greedier than solo default `0.01`).
- Opponent on the left: **`--opponent greedy`** (chase lowest fruit) or **`random`**.

Resume:

```bash
python -m rl.duel.train_duel --resume rl/models/best_ppo_duel/best_model.zip --timesteps 500000
```

## DE1-SoC build

See **`Makefile.de1`** comments for a duel `SRCS` / `HDRS` / `INCLUDES` (`-Ifirmware/duel`) example. You need **`firmware/solo/game.h`** on the include path for `Basket` / `Fruit` types.
