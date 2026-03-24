#!/usr/bin/env python3
"""Train PPO on CatchGame-v0 and export quantized policy header."""

from __future__ import annotations

import argparse
import os
import sys

import torch.nn as nn
from stable_baselines3 import PPO

from catch_env import CatchGameEnv
from export_policy import export_policy_int8


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--timesteps", type=int, default=100_000)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument(
        "--ent-coef",
        type=float,
        default=0.01,
        help="Entropy bonus (higher => more exploration; try 0.005–0.03 if stuck repeating)",
    )
    ap.add_argument(
        "--learning-rate",
        type=float,
        default=3e-4,
        help="Lower (e.g. 1e-4) if learning is unstable or collapses quickly",
    )
    ap.add_argument(
        "--clip-range",
        type=float,
        default=0.2,
        help="PPO clip epsilon; slightly larger (e.g. 0.25) can help escape bad plateaus",
    )
    ap.add_argument(
        "--out-model",
        default=os.path.join(os.path.dirname(__file__), "models", "ppo_catch"),
    )
    ap.add_argument(
        "--out-header",
        default=os.path.join(os.path.dirname(__file__), "..", "policy_weights.h"),
    )
    args = ap.parse_args()

    os.makedirs(os.path.dirname(args.out_model), exist_ok=True)

    env = CatchGameEnv()
    model = PPO(
        "MlpPolicy",
        env,
        verbose=1,
        seed=args.seed,
        policy_kwargs=dict(net_arch=[64, 64], activation_fn=nn.ReLU),
        learning_rate=args.learning_rate,
        n_steps=2048,
        batch_size=256,
        gamma=0.99,
        ent_coef=args.ent_coef,
        clip_range=args.clip_range,
    )
    model.learn(total_timesteps=args.timesteps, progress_bar=False)
    model.save(args.out_model)
    print("saved", args.out_model)

    export_policy_int8(model, args.out_header)
    print("exported", args.out_header)


if __name__ == "__main__":
    main()
