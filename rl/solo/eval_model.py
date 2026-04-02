#!/usr/bin/env python3
"""Evaluate a saved PPO model: mean score and episode length over many episodes."""

from __future__ import annotations

import argparse
import os
import statistics
import sys
from pathlib import Path

from stable_baselines3 import PPO

from rl.solo.catch_env import EASY_TRAIN_PRESET, CatchGameEnv


def main() -> int:
    rl_root = Path(__file__).resolve().parent.parent
    default_model = str(rl_root / "models" / "ppo_catch")

    ap = argparse.ArgumentParser(description="Evaluate trained CatchGame PPO")
    ap.add_argument("--model", default=default_model, help="Model path (.zip optional)")
    ap.add_argument("--episodes", type=int, default=100)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--easy", action="store_true", help="Match rl.solo.train --easy env")
    ap.add_argument("--initial-lives", type=int, default=10)
    ap.add_argument("--spawn-every", type=int, default=25)
    ap.add_argument("--basket-width", type=int, default=36)
    ap.add_argument("--basket-speed", type=int, default=3)
    ap.add_argument("--fruit-dy-min", type=int, default=1)
    ap.add_argument("--fruit-dy-max", type=int, default=1)
    ap.add_argument("--fruit-move-every", type=int, default=2)
    ap.add_argument("--fruit-r-min", type=int, default=4)
    ap.add_argument("--fruit-r-max", type=int, default=6)
    args = ap.parse_args()

    model_path = args.model
    if not os.path.isfile(model_path) and os.path.isfile(model_path + ".zip"):
        model_path = model_path + ".zip"
    if not os.path.isfile(model_path):
        print(f"Model not found: {args.model}", file=sys.stderr)
        return 1

    if args.easy:
        env = CatchGameEnv(**EASY_TRAIN_PRESET)
    else:
        env = CatchGameEnv(
            initial_lives=args.initial_lives,
            spawn_every=args.spawn_every,
            basket_width=args.basket_width,
            basket_speed=args.basket_speed,
            fruit_dy_min=args.fruit_dy_min,
            fruit_dy_max=args.fruit_dy_max,
            fruit_move_every=args.fruit_move_every,
            fruit_r_min=args.fruit_r_min,
            fruit_r_max=args.fruit_r_max,
        )

    model = PPO.load(model_path, env=env)

    scores: list[int] = []
    lengths: list[int] = []
    for ep in range(args.episodes):
        obs, _ = env.reset(seed=args.seed + ep)
        done = False
        steps = 0
        while not done:
            action, _ = model.predict(obs, deterministic=True)
            obs, _r, term, trunc, _ = env.step(int(action))
            steps += 1
            done = bool(term or trunc)
        scores.append(env.score)
        lengths.append(steps)

    env.close()

    print(
        f"episodes={args.episodes}  mean_score={statistics.mean(scores):.2f}  "
        f"std_score={statistics.pstdev(scores) if len(scores) > 1 else 0:.2f}  "
        f"mean_len={statistics.mean(lengths):.1f}"
    )
    print(f"min_score={min(scores)}  max_score={max(scores)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
