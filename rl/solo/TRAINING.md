# Training guide: strong CatchGame agent (solo / full screen)

Run commands from the **repository root** so `python -m rl.solo.*` resolves.

## 1. Match hardware

Train with the **same** rules as `firmware/solo/game.h` / `game.c` (see `CONTRACT.md`): lives, `FRUIT_DY`, `FRUIT_MOVE_EVERY`, spawn interval, basket size. After changing C, update Python defaults or pass the same CLI flags, then **retrain** and **re-export** `weights/policy_weights.h`.

## 2. Measure the right thing

- Use **`python -m rl.solo.eval_model --episodes 200 --seed 0`** (or more episodes). It varies the reset seed per episode so you see **mean ± spread** of **score** (catches), not a single lucky trajectory.
- **`EvalCallback` mean_reward** is the **sum of step rewards** (catches − misses), not the same number as **score**. Use it for rough progress only.
- **`episode_reward=… +/- 0.00` in EvalCallback** usually means eval is **almost the same trajectory every time** (deterministic policy + fixed eval reset). It is **not** proof the policy is stable. Trust **`eval_model`** mean and std over many seeds for “when to stop.”
- If `export_policy` reports a **large** `max|logit_fp - logit_q|`, int8 may change `argmax` on some frames; retrain or accept slight on-device drift.

## 2b. Continue training (`--resume`)

Same flags as before for env rules (`--initial-lives`, `--fruit-move-every`, `--n-envs`, `--n-steps`, `--batch-size` should match the run that produced the checkpoint):

```bash
python -m rl.solo.train --resume rl/models/ppo_catch.zip --timesteps 1000000
```

To keep fine-tuning from the **best** eval snapshot:

```bash
python -m rl.solo.train --resume rl/models/best_ppo_catch/best_model.zip --timesteps 1000000
```

On Windows PowerShell, paths like `rl\models\ppo_catch.zip` work. If you omit `.zip`, `train.py` will try adding `.zip` when that file exists.

## 2c. When to stop

1. Run **`eval_model`** every few hundred thousand steps (or after each `--resume` block). **Stop** when **mean score** over 200–500 episodes **flattens or drifts down** for several evaluations in a row.
2. If **training** `ep_rew_mean` keeps rising but **eval** `mean_reward` or **eval_model score** does not, you are likely **overfitting** the training env’s noise; stop and use the **best_model.zip** from eval, or reduce LR / train shorter.
3. Set a **target** you care about (e.g. mean catches per episode, or worst-case over N episodes); stop when you consistently beat it.
4. Diminishing returns: if another **1M steps** barely moves **eval_model mean**, more of the same rarely helps without changing the env, obs, or algorithm.

## 3. Recommended training recipe

**A. Baseline (current defaults)**  
`python -m rl.solo.train`  
Defaults: 1M steps, 4 parallel envs, `[128,128]` MLP, eval + best checkpoint, LR decay. Checkpoints under **`rl/models/`**; export **`weights/policy_weights.h`**.

**B. If learning is slow or flat**  
- Increase **`--timesteps`** (e.g. `2_000_000`).  
- Try **`--ent-coef 0.02`** for more exploration, or **`--learning-rate 1e-4`** if updates look unstable.  
- Temporarily **`--n-envs 8`** if CPU has cores (faster rollouts).

**C. Curriculum (optional)**  
1. `python -m rl.solo.train --easy --timesteps 500_000 --out-model rl/models/ppo_easy`  
2. Continue on the **real** game: `python -m rl.solo.train --resume rl/models/ppo_easy.zip --timesteps 1500000` (no `--easy`).

**D. Slightly easier game (already in firmware)**  
More lives and **`FRUIT_MOVE_EVERY`** &gt; 1 slow falling without changing `FRUIT_DY` in the observation. To go slower still, increase **`--fruit-move-every`** in Python **and** `#define FRUIT_MOVE_EVERY` in `game.h` together.

## 4. After training

1. `python -m rl.solo.eval_model --episodes 500` — set your own bar for mean score / min score.  
2. `python -m rl.solo.watch_agent` — visual sanity check.  
3. Flash firmware with **`weights/policy_weights.h`**.

## 5. Common pitfalls

- **`watch_agent` / `eval_model` flags must match training** (lives, `fruit-move-every`, etc.).  
- Old **`.zip` checkpoints** trained on old dynamics will look bad on the new game — retrain.  
- **Parity tests** (`tests/parity_steps`) must be rebuilt after C changes.

## Split-screen duel

For human vs AI on a **160+160** split field, see **`rl/duel/DUEL.md`** and **`python -m rl.duel.train_duel`**.
