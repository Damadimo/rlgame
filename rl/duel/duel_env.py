"""Split-screen duel env: scripted left pane vs RL agent right pane (160px-wide fields).

Logic and RNG order mirror duel_game.c / observation_duel.c (see rl/DUEL.md)."""

from __future__ import annotations

import gymnasium as gym
import numpy as np
from gymnasium import spaces

from rl.solo.catch_env import (
    FRUIT_PALETTE,
    MAX_FRUITS,
    _game_rng_next,
    action_to_key_bits,
    build_observation,
    fruit_hits_basket,
)

# Must match duel_game.h
DUEL_PANE_W = 160
DUEL_PANE_H = 240
DUEL_FRUIT_DY = 1
DUEL_FRUIT_MOVE_EVERY = 2
DUEL_BASKET_W = 18
DUEL_BASKET_H = 10
DUEL_BASKET_SPEED = 2
DUEL_SPAWN_EVERY = 22
DUEL_INITIAL_LIVES = 8

OBS_DIM = 1 + MAX_FRUITS * 5


def _clamp(v: int, lo: int, hi: int) -> int:
    return lo if v < lo else hi if v > hi else v


def _greedy_opponent_keys(basket_x: int, basket_w: int, fruits: list[dict]) -> int:
    target = None
    best_y = -10**9
    for f in fruits:
        if f["active"] and f["y"] > best_y:
            best_y = f["y"]
            target = f
    if target is None:
        return 0
    cx = basket_x + basket_w // 2
    if target["x"] < cx - 1:
        return 0x8
    if target["x"] > cx + 1:
        return 0x4
    return 0


def _random_opponent_keys(r: int) -> int:
    m = r % 100
    if m < 50:
        return 0
    if m < 75:
        return 0x8
    return 0x4


class DuelCatchRightEnv(gym.Env):
    """Train the right (AI) player; left uses a scripted opponent."""

    metadata = {"render_modes": []}

    def __init__(
        self,
        *,
        opponent: str = "greedy",
        initial_lives: int = DUEL_INITIAL_LIVES,
        spawn_every: int = DUEL_SPAWN_EVERY,
        basket_width: int = DUEL_BASKET_W,
        basket_height: int = DUEL_BASKET_H,
        basket_speed: int = DUEL_BASKET_SPEED,
        fruit_dy: int = DUEL_FRUIT_DY,
        fruit_move_every: int = DUEL_FRUIT_MOVE_EVERY,
        fruit_r_min: int = 2,
        fruit_r_max: int = 4,
    ):
        super().__init__()
        if opponent not in ("greedy", "random"):
            raise ValueError("opponent must be 'greedy' or 'random'")
        if fruit_r_min > fruit_r_max:
            raise ValueError("fruit_r_min must be <= fruit_r_max")
        self._opponent = opponent
        self._initial_lives = int(initial_lives)
        self._spawn_every = int(spawn_every)
        self._basket_w = int(basket_width)
        self._basket_h = int(basket_height)
        self._basket_speed = int(basket_speed)
        self._fruit_dy = int(fruit_dy)
        self._fruit_move_every = int(fruit_move_every)
        self._fruit_r_min = int(fruit_r_min)
        self._fruit_r_max = int(fruit_r_max)
        self._obs_dy_div = float(max(3, self._fruit_dy))
        self._obs_r_div = float(max(8, self._fruit_r_max))

        self.action_space = spaces.Discrete(3)
        self.observation_space = spaces.Box(low=0.0, high=1.0, shape=(OBS_DIM,), dtype=np.float32)

        self._rng_state = 1
        self._left_basket: dict = {}
        self._right_basket: dict = {}
        self._left_fruits: list[dict] = []
        self._right_fruits: list[dict] = []
        self.frame_counter = 0
        self._left_lives = DUEL_INITIAL_LIVES
        self._right_lives = DUEL_INITIAL_LIVES
        self._left_score = 0
        self._right_score = 0
        self.running = True

    def _rand(self) -> int:
        self._rng_state, r = _game_rng_next(self._rng_state)
        return r

    def _empty_fruit(self) -> dict:
        return {
            "x": 0,
            "y": 0,
            "r": 0,
            "dy": 0,
            "active": False,
        }

    def reset(self, *, seed: int | None = None, options: dict | None = None):
        super().reset(seed=seed)
        if seed is None:
            seed = 1
        self._rng_state = int(seed) & 0xFFFFFFFF

        yb = DUEL_PANE_H - 18
        self._left_basket = {
            "x": (DUEL_PANE_W - self._basket_w) // 2,
            "y": yb,
            "w": self._basket_w,
            "h": self._basket_h,
        }
        self._right_basket = {
            "x": (DUEL_PANE_W - self._basket_w) // 2,
            "y": yb,
            "w": self._basket_w,
            "h": self._basket_h,
        }
        self._left_fruits = [self._empty_fruit() for _ in range(MAX_FRUITS)]
        self._right_fruits = [self._empty_fruit() for _ in range(MAX_FRUITS)]
        self.frame_counter = 0
        self._left_lives = self._initial_lives
        self._right_lives = self._initial_lives
        self._left_score = 0
        self._right_score = 0
        self.running = True
        return self._get_obs_right(), {}

    def _spawn_fruit(self, fruits: list[dict]) -> None:
        span = self._fruit_r_max - self._fruit_r_min + 1
        for i in range(MAX_FRUITS):
            if not fruits[i]["active"]:
                fruits[i]["active"] = True
                fruits[i]["r"] = self._fruit_r_min + (self._rand() % span)
                fruits[i]["x"] = 6 + self._rand() % (DUEL_PANE_W - 12)
                fruits[i]["y"] = -5
                fruits[i]["dy"] = self._fruit_dy
                fruits[i]["color"] = FRUIT_PALETTE[self._rand() % len(FRUIT_PALETTE)]
                return

    def _update_basket(self, basket: dict, keys: int) -> None:
        if keys & 0x8:
            basket["x"] -= self._basket_speed
        if keys & 0x4:
            basket["x"] += self._basket_speed
        max_x = DUEL_PANE_W - basket["w"]
        basket["x"] = _clamp(basket["x"], 0, max_x)

    def _update_fruits_side(
        self, fruits: list[dict], basket: dict, *, is_right: bool
    ) -> tuple[int, int]:
        catches = 0
        misses = 0
        for i in range(MAX_FRUITS):
            f = fruits[i]
            if not f["active"]:
                continue
            if self.frame_counter % self._fruit_move_every == 0:
                f["y"] += f["dy"]
            if fruit_hits_basket(f, basket):
                f["active"] = False
                catches += 1
            elif f["y"] - f["r"] > DUEL_PANE_H:
                f["active"] = False
                misses += 1
        if is_right:
            self._right_score += catches
            self._right_lives -= misses
        else:
            self._left_score += catches
            self._left_lives -= misses
        return catches, misses

    def _get_obs_right(self) -> np.ndarray:
        return build_observation(
            self._right_basket["x"],
            self._right_fruits,
            screen_w=DUEL_PANE_W,
            screen_h=DUEL_PANE_H,
            basket_w=self._right_basket["w"],
            dy_norm_div=self._obs_dy_div,
            r_norm_div=self._obs_r_div,
        )

    def step(self, action: int):
        self.frame_counter += 1

        if self._opponent == "greedy":
            lk = _greedy_opponent_keys(
                self._left_basket["x"], self._left_basket["w"], self._left_fruits
            )
        else:
            lk = _random_opponent_keys(self._rand())

        self._update_basket(self._left_basket, lk)
        self._update_basket(self._right_basket, action_to_key_bits(int(action)))

        if self.frame_counter % self._spawn_every == 0:
            self._spawn_fruit(self._left_fruits)
            self._spawn_fruit(self._right_fruits)

        self._update_fruits_side(self._left_fruits, self._left_basket, is_right=False)
        rc, rm = self._update_fruits_side(
            self._right_fruits, self._right_basket, is_right=True
        )

        reward = float(rc) - float(rm)
        terminated = self._left_lives <= 0 or self._right_lives <= 0
        self.running = not terminated
        return self._get_obs_right(), reward, terminated, False, {}
