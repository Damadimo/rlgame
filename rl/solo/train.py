#!/usr/bin/env python3
"""Train PPO on CatchGameEnv; export int8 policy header.

Defaults favor strong agents: sorted observations, wider MLP, long training,
periodic eval + best-checkpoint export."""

from __future__ import annotations

import argparse
import os
from pathlib import Path

import torch.nn as nn
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import EvalCallback
from stable_baselines3.common.utils import FloatSchedule, LinearSchedule
from stable_baselines3.common.vec_env import DummyVecEnv, VecEnv

from rl.shared.export_policy import export_policy_int8
from rl.solo.catch_env import EASY_TRAIN_PRESET, CatchGameEnv


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _rl_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _make_env(args) -> CatchGameEnv:
    """Build env; --easy overrides individual difficulty flags."""
    if args.easy:
        return CatchGameEnv(**EASY_TRAIN_PRESET)
    return CatchGameEnv(
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--timesteps",
        type=int,
        default=1_000_000,
        help="Total env steps (with --n-envs>1, each rollout collects n_envs * n_steps steps)",
    )
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument(
        "--ent-coef",
        type=float,
        default=0.01,
        help="Entropy bonus (try 0.02 early if exploration looks poor)",
    )
    ap.add_argument(
        "--learning-rate",
        type=float,
        default=3e-4,
        help="Initial LR; decays linearly to --lr-final if set",
    )
    ap.add_argument(
        "--lr-final",
        type=float,
        default=1e-5,
        help="Linear LR schedule end value (set to same as --learning-rate to disable decay)",
    )
    ap.add_argument(
        "--clip-range",
        type=float,
        default=0.2,
        help="PPO clip epsilon",
    )
    ap.add_argument(
        "--out-model",
        default=str(_rl_root() / "models" / "ppo_catch"),
    )
    ap.add_argument(
        "--out-header",
        default=str(_repo_root() / "weights" / "policy_weights.h"),
    )
    ap.add_argument(
        "--easy",
        action="store_true",
        help="Curriculum preset (wider basket, etc.); train without --easy before hardware export",
    )
    ap.add_argument("--initial-lives", type=int, default=10)
    ap.add_argument("--spawn-every", type=int, default=25)
    ap.add_argument("--basket-width", type=int, default=36)
    ap.add_argument("--basket-speed", type=int, default=3)
    ap.add_argument("--fruit-dy-min", type=int, default=1, help="Must equal --fruit-dy-max")
    ap.add_argument("--fruit-dy-max", type=int, default=1)
    ap.add_argument(
        "--fruit-move-every",
        type=int,
        default=2,
        help="Apply vertical dy only on frames where frame_counter %% N == 0 (game.h FRUIT_MOVE_EVERY); larger = slower fall",
    )
    ap.add_argument("--fruit-r-min", type=int, default=4)
    ap.add_argument("--fruit-r-max", type=int, default=6)
    ap.add_argument(
        "--net-width",
        type=int,
        nargs="+",
        default=[128, 128],
        help="Hidden layer sizes for MLP policy/value (e.g. 128 128)",
    )
    ap.add_argument(
        "--n-envs",
        type=int,
        default=4,
        help="Parallel DummyVecEnv instances (faster data collection; use 1 to debug)",
    )
    ap.add_argument("--n-steps", type=int, default=2048, help="Steps per env per PPO rollout")
    ap.add_argument("--batch-size", type=int, default=512)
    ap.add_argument("--gamma", type=float, default=0.99)
    ap.add_argument(
        "--no-eval",
        action="store_true",
        help="Skip EvalCallback (faster; exports last checkpoint only)",
    )
    ap.add_argument(
        "--eval-freq",
        type=int,
        default=0,
        help="Approx. total env timesteps between evaluations (0 = auto ~1%% of --timesteps); "
        "internally scaled for --n-envs per SB3 EvalCallback semantics",
    )
    ap.add_argument("--n-eval-episodes", type=int, default=20)
    ap.add_argument(
        "--resume",
        metavar="PATH",
        default=None,
        help="Continue from an SB3 .zip (e.g. models/ppo_catch.zip or best_ppo_catch/best_model.zip). "
        "Uses --learning-rate / --lr-final / --ent-coef / --clip-range from CLI for this run; "
        "rollout shape (n_steps, batch_size, n_envs) should match how the checkpoint was trained.",
    )
    args = ap.parse_args()

    if args.resume is None and len(args.net_width) != 2:
        ap.error("--net-width must list exactly two hidden sizes (export_policy.py / policy.c expect two ReLU layers)")
    if args.timesteps < 1:
        ap.error("--timesteps must be >= 1")
    if args.n_steps < 1 or args.batch_size < 1:
        ap.error("--n-steps and --batch-size must be >= 1")
    rollout_size = max(1, args.n_envs) * args.n_steps
    if rollout_size < args.batch_size:
        ap.error(
            f"n_envs * n_steps ({rollout_size}) must be >= batch_size ({args.batch_size}) for PPO"
        )
    if args.fruit_move_every < 1:
        ap.error("--fruit-move-every must be >= 1")

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

    if args.easy:
        print("train: --easy preset (not C parity):", EASY_TRAIN_PRESET)

    # SB3 LinearSchedule: interpolate from start (pr=1) to end (pr=0) while training progress
    # (1 - progress_remaining) runs 0 → 1; end_fraction=1.0 spans the full run.
    if args.learning_rate == args.lr_final:
        learning_rate = float(args.learning_rate)
    else:
        # PPO wraps with FloatSchedule internally; LinearSchedule is the modern API (replaces get_linear_fn).
        learning_rate = LinearSchedule(float(args.learning_rate), float(args.lr_final), 1.0)

    def _one_env():
        return _make_env(args)

    env: CatchGameEnv | VecEnv
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
            # Fresh LR schedule over *this* learn() segment (works with reset_num_timesteps=False).
            if args.learning_rate == args.lr_final:
                model.learning_rate = float(args.learning_rate)
            else:
                model.learning_rate = LinearSchedule(
                    float(args.learning_rate), float(args.lr_final), 1.0
                )
            model.ent_coef = args.ent_coef
            # PPO.train() calls self.clip_range(progress); raw float breaks after load.
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
            # EvalCallback counts _on_step calls; each VecEnv step advances num_timesteps by n_envs.
            target_ts = (
                args.eval_freq
                if args.eval_freq > 0
                else max(8192, args.timesteps // 100)
            )
            n_envs_eff = max(1, args.n_envs)
            eval_freq_calls = max(target_ts // n_envs_eff, 1)
            best_dir = os.path.join(os.path.dirname(args.out_model) or ".", "best_ppo_catch")
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

        # Only use EvalCallback's best checkpoint when eval ran this session; otherwise a stale
        # best_model.zip from an older run would overwrite the freshly trained weights on export.
        if not args.no_eval:
            best_zip = os.path.join(
                os.path.dirname(args.out_model) or ".", "best_ppo_catch", "best_model.zip"
            )
            if os.path.isfile(best_zip):
                print("loading best eval checkpoint for export:", best_zip)
                train_env = model.get_env()
                model = PPO.load(best_zip, env=train_env)

        export_policy_int8(model, args.out_header)
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
