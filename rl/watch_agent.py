#!/usr/bin/env python3
"""Load a trained PPO model and watch it play in a pygame window."""

from __future__ import annotations

import argparse
import os
import sys

import pygame
from stable_baselines3 import PPO

from catch_env import EASY_TRAIN_PRESET, CatchGameEnv


def main() -> int:
    root = os.path.dirname(os.path.abspath(__file__))
    default_model = os.path.join(root, "models", "ppo_catch")

    ap = argparse.ArgumentParser(description="Watch trained agent play CatchGame")
    ap.add_argument(
        "--model",
        default=default_model,
        help="Path to saved PPO model (without .zip if SB3 adds it)",
    )
    ap.add_argument("--episodes", type=int, default=10, help="Episodes before exit")
    ap.add_argument("--fps", type=int, default=60, help="Target frame rate")
    ap.add_argument(
        "--scale",
        type=int,
        default=2,
        help="Window scale (2 => 640x480 for 320x240 game)",
    )
    ap.add_argument("--seed", type=int, default=None, help="Base RNG seed for reset")
    ap.add_argument(
        "--stochastic",
        action="store_true",
        help="Sample from policy instead of greedy (argmax)",
    )
    ap.add_argument(
        "--easy",
        action="store_true",
        help="Use same EASY_TRAIN_PRESET as rl/train.py --easy (must match how the model was trained)",
    )
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
        print("Train first: python rl/train.py", file=sys.stderr)
        return 1

    if args.easy:
        env = CatchGameEnv(render_mode="human", render_scale=args.scale, **EASY_TRAIN_PRESET)
    else:
        env = CatchGameEnv(
            render_mode="human",
            render_scale=args.scale,
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
    model = PPO.load(model_path)

    clock = pygame.time.Clock()
    try:
        for ep in range(args.episodes):
            seed = None if args.seed is None else (args.seed + ep)
            obs, _ = env.reset(seed=seed)
            env.render()
            done = False
            while not done:
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        return 0
                    if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                        return 0

                action, _ = model.predict(obs, deterministic=not args.stochastic)
                obs, _reward, terminated, truncated, _ = env.step(int(action))
                env.render()
                clock.tick(args.fps)
                done = bool(terminated or truncated)
    finally:
        env.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
