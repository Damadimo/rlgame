# RL Catch Game (DE1-SoC + Python PPO)

Two **firmware + training** tracks share VGA / input / RNG code but use different game logic and weight headers.

## Layout

| Path | Purpose |
|------|---------|
| **`firmware/shared/`** | `graphics`, `input`, `rng` (both modes) |
| **`firmware/solo/`** | Full-screen catch: `game`, `observation`, `policy`, `main.c`, `main_agent.c` |
| **`firmware/duel/`** | Split-screen 160+160: `duel_game`, `observation_duel`, `policy_duel`, `main_duel.c` |
| **`weights/`** | `policy_weights.h` / `policy_weights_duel.h` (int8 export); `policy_weights_alt.h` (float 3→8→3 alt net, hand-edited or replaced) |
| **`rl/shared/`** | `export_policy.py`, `validate_policy_match.py` |
| **`rl/solo/`** | Full-screen env + `train.py`, `eval_model.py`, `watch_agent.py`, `CONTRACT.md`, `TRAINING.md` |
| **`rl/duel/`** | Split-screen env + `train_duel.py`, `DUEL.md` |
| **`tests/`** | Host `parity_steps`, `policy_smoke`, `policy_duel_smoke`, pytest |

## Solo (AI-only or human full screen)

```bash
# Train (repo root) → weights/policy_weights.h
python -m rl.solo.train

python -m rl.solo.eval_model --episodes 200
python -m rl.solo.watch_agent
```

Unified menu **`firmware/main_game.c`**: **KEY3** runs full-screen AI using **`weights/policy_weights_alt.h`**: a **float** MLP (`POLICY_ALT_F32_W1` / `B1` / `W2` / `B2`). The game still builds the usual 26-D vector; **`policy_alt_select_action_from_game_obs`** feeds **three** values in training order: **closest fruit x**, **closest fruit y**, **basket x** (`obs[2]`, `obs[3]`, `obs[0]` — first sorted fruit slot). Adjust packing in **`firmware/solo/policy_alt.c`** if needed. The default solo policy remains the int8 export in **`policy_weights.h`**.

Docs: **`rl/solo/CONTRACT.md`**, **`rl/solo/TRAINING.md`**.  
DE1: **`Makefile.de1`** default `SRCS` target solo AI (`main_agent.c`).

## Duel (human left vs AI right)

```bash
python -m rl.duel.train_duel
```

Doc: **`rl/duel/DUEL.md`**.  
DE1: switch `MAIN` / `SRCS` / `HDRS` / `INCLUDES` per comments in **`Makefile.de1`**.

## Host C tests (needs `gcc`)

```bash
make
pytest
```

## Python deps

```bash
pip install -r requirements.txt
```
