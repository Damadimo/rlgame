#!/usr/bin/env python3
"""Train PPO for split-screen duel (right-hand AI). Exports policy_weights_duel.h.

Lower default --ent-coef than train.py for a greedier (more deterministic) policy."""

from __future__ import annotations

import argparse
import os
from pathlib import Path

import torch.nn as nn
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import EvalCallback
from stable_baselines3.common.utils import FloatSchedule, LinearSchedule
from stable_baselines3.common.vec_env import DummyVecEnv, VecEnv

from rl.duel.duel_env import DUEL_INITIAL_LIVES, DUEL_SPAWN_EVERY, DuelCatchRightEnv
from rl.shared.export_policy import export_policy_int8


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _rl_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _make_env(args) -> DuelCatchRightEnv:
    return DuelCatchRightEnv(
        opponent=args.opponent,
        initial_lives=args.initial_lives,
        spawn_every=args.spawn_every,
        basket_width=args.basket_width,
        basket_speed=args.basket_speed,
        fruit_dy=args.fruit_dy,
        fruit_move_every=args.fruit_move_every,
        fruit_r_min=args.fruit_r_min,
        fruit_r_max=args.fruit_r_max,
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--timesteps", type=int, default=1_000_000)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument(
        "--ent-coef",
        type=float,
        default=0.005,
        help="Entropy bonus (lower = greedier policy; duel default below train.py)",
    )
    ap.add_argument("--learning-rate", type=float, default=3e-4)
    ap.add_argument("--lr-final", type=float, default=1e-5)
    ap.add_argument("--clip-range", type=float, default=0.2)
    ap.add_argument(
        "--out-model",
        default=str(_rl_root() / "models" / "ppo_duel_catch"),
    )
    ap.add_argument(
        "--out-header",
        default=str(_repo_root() / "weights" / "policy_weights_duel.h"),
    )
    ap.add_argument(
        "--opponent",
        choices=("greedy", "random"),
        default="greedy",
        help="Left-pane scripted opponent for training",
    )
    ap.add_argument("--initial-lives", type=int, default=DUEL_INITIAL_LIVES)
    ap.add_argument("--spawn-every", type=int, default=DUEL_SPAWN_EVERY)
    ap.add_argument("--basket-width", type=int, default=18)
    ap.add_argument("--basket-speed", type=int, default=2)
    ap.add_argument("--fruit-dy", type=int, default=1)
    ap.add_argument("--fruit-move-every", type=int, default=2)
    ap.add_argument("--fruit-r-min", type=int, default=2)
    ap.add_argument("--fruit-r-max", type=int, default=4)
    ap.add_argument("--net-width", type=int, nargs="+", default=[128, 128])
    ap.add_argument("--n-envs", type=int, default=4)
    ap.add_argument("--n-steps", type=int, default=2048)
    ap.add_argument("--batch-size", type=int, default=512)
    ap.add_argument("--gamma", type=float, default=0.99)
    ap.add_argument("--no-eval", action="store_true")
    ap.add_argument("--eval-freq", type=int, default=0)
    ap.add_argument("--n-eval-episodes", type=int, default=20)
    ap.add_argument("--resume", metavar="PATH", default=None)
    args = ap.parse_args()

    if args.resume is None and len(args.net_width) != 2:
        ap.error("--net-width must list exactly two hidden sizes")
    if args.timesteps < 1:
        ap.error("--timesteps must be >= 1")
    rollout_size = max(1, args.n_envs) * args.n_steps
    if rollout_size < args.batch_size:
        ap.error("n_envs * n_steps must be >= batch_size")
    if args.fruit_move_every < 1:
        ap.error("--fruit-move-every must be >= 1")
    if args.fruit_r_min > args.fruit_r_max:
        ap.error("fruit_r_min must be <= fruit_r_max")

    resume_path: str | None = None
    if args.resume:
        resume_path = args.resume
        if not resume_path.endswith(".zip"):
            alt = resume_path + ".zip"
            if os.path.isfile(alt):
                resume_path = alt
        if not os.path.isfile(resume_path):
            ap.error(f"--resume not found: {resume_path}")

    os.makedirs(os.path.dirname(args.out_model) or ".", exist_ok=True)

    if args.learning_rate == args.lr_final:
        learning_rate = float(args.learning_rate)
    else:
        learning_rate = LinearSchedule(float(args.learning_rate), float(args.lr_final), 1.0)

    def _one_env():
        return _make_env(args)

    env: DuelCatchRightEnv | VecEnv
    if args.n_envs <= 1:
        env = _one_env()
    else:
        env = DummyVecEnv([_one_env for _ in range(args.n_envs)])

    eval_env: VecEnv | None = None
    policy_kwargs = dict(net_arch=list(args.net_width), activation_fn=nn.ReLU)

    model: PPO | None = None
    try:
        if resume_path is not None:
            print("resume:", resume_path)
            model = PPO.load(resume_path, env=env)
            if args.learning_rate == args.lr_final:
                model.learning_rate = float(args.learning_rate)
            else:
                model.learning_rate = LinearSchedule(
                    float(args.learning_rate), float(args.lr_final), 1.0
                )
            model.ent_coef = args.ent_coef
            model.clip_range = FloatSchedule(args.clip_range)
        else:
            model = PPO(
                "MlpPolicy",
                env,
                verbose=1,
                seed=args.seed,
                policy_kwargs=policy_kwargs,
                learning_rate=learning_rate,
                n_steps=args.n_steps,
                batch_size=args.batch_size,
                gamma=args.gamma,
                ent_coef=args.ent_coef,
                clip_range=args.clip_range,
                gae_lambda=0.95,
                max_grad_norm=0.5,
                vf_coef=0.5,
                n_epochs=10,
            )

        callbacks = []
        if not args.no_eval:
            eval_env = DummyVecEnv([_one_env])
            target_ts = (
                args.eval_freq if args.eval_freq > 0 else max(8192, args.timesteps // 100)
            )
            n_envs_eff = max(1, args.n_envs)
            eval_freq_calls = max(target_ts // n_envs_eff, 1)
            best_dir = os.path.join(os.path.dirname(args.out_model) or ".", "best_ppo_duel")
            os.makedirs(best_dir, exist_ok=True)
            callbacks.append(
                EvalCallback(
                    eval_env,
                    best_model_save_path=best_dir,
                    log_path=os.path.join(best_dir, "logs"),
                    eval_freq=eval_freq_calls,
                    deterministic=True,
                    n_eval_episodes=args.n_eval_episodes,
                    verbose=1,
                )
            )

        model.learn(
            total_timesteps=args.timesteps,
            progress_bar=False,
            callback=callbacks if callbacks else None,
            reset_num_timesteps=resume_path is None,
        )
        model.save(args.out_model)
        print("saved", args.out_model)

        if not args.no_eval:
            best_zip = os.path.join(
                os.path.dirname(args.out_model) or ".", "best_ppo_duel", "best_model.zip"
            )
            if os.path.isfile(best_zip):
                print("loading best eval checkpoint for export:", best_zip)
                train_env = model.get_env()
                model = PPO.load(best_zip, env=train_env)

        export_policy_int8(
            model,
            args.out_header,
            include_guard="POLICY_WEIGHTS_DUEL_H",
            symbol_prefix="DUEL_POLICY",
        )
        print("exported", args.out_header)
    except BaseException:
        if model is None:
            try:
                env.close()
            except Exception:
                pass
        raise
    finally:
        if eval_env is not None:
            eval_env.close()
        if model is not None:
            ge = model.get_env()
            if ge is not None:
                ge.close()


if __name__ == "__main__":
    main()
