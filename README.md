# RL Catch Game (DE1-SoC + Python PPO)

Two **firmware + training** tracks share VGA / input / RNG code but use different game logic and weight headers.

## Layout

| Path | Purpose |
|------|---------|
| **`firmware/shared/`** | `graphics`, `input`, `rng` (both modes) |
| **`firmware/solo/`** | Full-screen catch: `game`, `observation`, `policy`, `main.c`, `main_agent.c` |
| **`firmware/duel/`** | Split-screen 160+160: `duel_game`, `observation_duel`, `policy_duel`, `main_duel.c` |
| **`weights/`** | Exported int8 headers: `policy_weights.h` (solo), `policy_weights_duel.h` (duel) |
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
