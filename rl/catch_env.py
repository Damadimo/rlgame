"""Gymnasium environment matching game.c logic (see CONTRACT.md)."""

from __future__ import annotations

import gymnasium as gym
import numpy as np
from gymnasium import spaces

# Mirrors game.h / graphics.h
SCREEN_WIDTH = 320
SCREEN_HEIGHT = 240
MAX_FRUITS = 5
FRUIT_DY = 1
FRUIT_MOVE_EVERY = 2
INITIAL_LIVES = 10
BASKET_WIDTH = 36
BASKET_HEIGHT = 10
BASKET_SPEED = 3
SPAWN_EVERY = 25

OBS_DIM = 1 + MAX_FRUITS * 5

# Optional curriculum preset for Python-only training (does not match game.c / CONTRACT.md).
EASY_TRAIN_PRESET = {
    "initial_lives": 20,
    "spawn_every": 40,
    "basket_width": 44,
    "basket_speed": BASKET_SPEED,
    "fruit_dy_min": FRUIT_DY,
    "fruit_dy_max": FRUIT_DY,
    "fruit_move_every": FRUIT_MOVE_EVERY,
    "fruit_r_min": 5,
    "fruit_r_max": 7,
}


def action_to_key_bits(action: int) -> int:
    if action == 1:
        return 0x8
    if action == 2:
        return 0x4
    return 0


def _clamp(v: int, lo: int, hi: int) -> int:
    return lo if v < lo else hi if v > hi else v


def _game_rng_next(state: int) -> tuple[int, int]:
    state = (state * 1103515245 + 12345) & 0xFFFFFFFF
    r = (state >> 16) & 0x7FFF
    return state, r


def build_observation(
    basket_x: int,
    fruits: list[dict],
    *,
    screen_w: int = SCREEN_WIDTH,
    screen_h: int = SCREEN_HEIGHT,
    basket_w: int = BASKET_WIDTH,
    dy_norm_div: float = 3.0,
    r_norm_div: float = 8.0,
) -> np.ndarray:
    """Fruit slots are sorted by urgency: larger ``y`` first (lower on screen), then ``x``, then slot index."""
    obs = np.zeros(OBS_DIM, dtype=np.float32)
    max_x = screen_w - basket_w
    obs[0] = basket_x / float(max_x) if max_x > 0 else 0.0
    base = 1
    dy_d = float(dy_norm_div) if dy_norm_div > 0 else 1.0
    r_d = float(r_norm_div) if r_norm_div > 0 else 1.0

    actives: list[tuple[int, int, int, dict]] = []
    for i, f in enumerate(fruits):
        if f["active"]:
            actives.append((f["y"], f["x"], i, f))
    actives.sort(key=lambda t: (-t[0], t[1], t[2]))

    for slot, (_y, _x, _i, f) in enumerate(actives[:MAX_FRUITS]):
        off = base + slot * 5
        obs[off + 0] = 1.0
        obs[off + 1] = f["x"] / float(screen_w)
        yn = (f["y"] + 32) / float(screen_h + 64)
        obs[off + 2] = float(np.clip(yn, 0.0, 1.0))
        obs[off + 3] = f["dy"] / dy_d
        obs[off + 4] = f["r"] / r_d

    return obs


def fruit_hits_basket(f: dict, basket: dict) -> bool:
    fruit_left = f["x"] - f["r"]
    fruit_right = f["x"] + f["r"]
    fruit_top = f["y"] - f["r"]
    fruit_bottom = f["y"] + f["r"]
    basket_left = basket["x"]
    basket_right = basket["x"] + basket["w"]
    basket_top = basket["y"]
    basket_bottom = basket["y"] + basket["h"]
    if fruit_right < basket_left:
        return False
    if fruit_left > basket_right:
        return False
    if fruit_bottom < basket_top:
        return False
    if fruit_top > basket_bottom:
        return False
    return True


FRUIT_PALETTE = (
    (248, 0, 0),
    (0, 252, 0),
    (255, 252, 0),
    (248, 28, 248),
    (0, 252, 252),
    (252, 128, 0),
)


class CatchGameEnv(gym.Env):
    metadata = {"render_modes": ["human", "rgb_array"]}

    def __init__(
        self,
        render_mode: str | None = None,
        render_scale: int = 2,
        *,
        initial_lives: int = INITIAL_LIVES,
        spawn_every: int = SPAWN_EVERY,
        basket_width: int = BASKET_WIDTH,
        basket_height: int = BASKET_HEIGHT,
        basket_speed: int = BASKET_SPEED,
        fruit_dy_min: int = FRUIT_DY,
        fruit_dy_max: int = FRUIT_DY,
        fruit_move_every: int = FRUIT_MOVE_EVERY,
        fruit_r_min: int = 4,
        fruit_r_max: int = 6,
    ):
        super().__init__()
        if not (1 <= fruit_dy_min <= fruit_dy_max):
            raise ValueError("fruit_dy_min and fruit_dy_max must satisfy 1 <= min <= max")
        if fruit_dy_min != fruit_dy_max:
            raise ValueError("fruit_dy_min must equal fruit_dy_max (constant fall speed; matches game.c FRUIT_DY)")
        if not (1 <= fruit_r_min <= fruit_r_max):
            raise ValueError("fruit_r_min and fruit_r_max must satisfy 1 <= min <= max")
        if not (1 <= basket_width < SCREEN_WIDTH):
            raise ValueError("basket_width must be in [1, SCREEN_WIDTH)")
        if initial_lives < 1:
            raise ValueError("initial_lives must be >= 1")
        if spawn_every < 1:
            raise ValueError("spawn_every must be >= 1")
        if fruit_move_every < 1:
            raise ValueError("fruit_move_every must be >= 1")

        self.render_mode = render_mode
        self.render_scale = max(1, int(render_scale))
        self.action_space = spaces.Discrete(3)
        self.observation_space = spaces.Box(
            low=0.0, high=1.0, shape=(OBS_DIM,), dtype=np.float32
        )
        self._initial_lives = int(initial_lives)
        self._spawn_every = int(spawn_every)
        self._basket_width = int(basket_width)
        self._basket_height = int(basket_height)
        self._basket_speed = int(basket_speed)
        self._fruit_dy_min = int(fruit_dy_min)
        self._fruit_dy_max = int(fruit_dy_max)
        self._fruit_r_min = int(fruit_r_min)
        self._fruit_r_max = int(fruit_r_max)
        self._fruit_move_every = int(fruit_move_every)
        # Keep dy/r scales at least as large as the C contract so easy mode stays in [0, 1].
        self._obs_dy_div = float(max(3, self._fruit_dy_max))
        self._obs_r_div = float(max(8, self._fruit_r_max))

        self._rng_state = 1
        self._basket: dict = {}
        self._fruits: list[dict] = []
        self.frame_counter = 0
        self.score = 0
        self.lives = self._initial_lives
        self.running = True
        # Lazy pygame (only if rendering)
        self._pg_screen = None
        self._pg_clock = None
        self._pg_inited = False

    def _rand(self) -> int:
        self._rng_state, r = _game_rng_next(self._rng_state)
        return r

    def reset(
        self,
        *,
        seed: int | None = None,
        options: dict | None = None,
    ):
        super().reset(seed=seed)
        if seed is None:
            seed = 1
        self._rng_state = int(seed) & 0xFFFFFFFF
        self._basket = {
            "x": (SCREEN_WIDTH - self._basket_width) // 2,
            "y": SCREEN_HEIGHT - 18,
            "w": self._basket_width,
            "h": self._basket_height,
        }
        self._fruits = [
            {
                "x": 0,
                "y": 0,
                "r": 0,
                "dy": 0,
                "active": False,
            }
            for _ in range(MAX_FRUITS)
        ]
        self.frame_counter = 0
        self.score = 0
        self.lives = self._initial_lives
        self.running = True
        obs = self._get_obs()
        return obs, {}

    def _spawn_fruit(self) -> None:
        r_span = self._fruit_r_max - self._fruit_r_min + 1
        for i in range(MAX_FRUITS):
            if not self._fruits[i]["active"]:
                self._fruits[i]["active"] = True
                self._fruits[i]["r"] = self._fruit_r_min + (self._rand() % r_span)
                self._fruits[i]["x"] = 10 + self._rand() % (SCREEN_WIDTH - 20)
                self._fruits[i]["y"] = -5
                self._fruits[i]["dy"] = self._fruit_dy_min
                self._fruits[i]["color"] = FRUIT_PALETTE[self._rand() % len(FRUIT_PALETTE)]
                return

    def _update_basket(self, keys: int) -> None:
        if keys & 0x8:
            self._basket["x"] -= self._basket_speed
        if keys & 0x4:
            self._basket["x"] += self._basket_speed
        max_x = SCREEN_WIDTH - self._basket["w"]
        self._basket["x"] = _clamp(self._basket["x"], 0, max_x)

    def _update_fruits(self) -> tuple[int, int]:
        """Returns (catches, misses) this step."""
        catches = 0
        misses = 0
        for i in range(MAX_FRUITS):
            f = self._fruits[i]
            if not f["active"]:
                continue
            if self.frame_counter % self._fruit_move_every == 0:
                f["y"] += f["dy"]
            if fruit_hits_basket(f, self._basket):
                f["active"] = False
                self.score += 1
                catches += 1
            elif f["y"] - f["r"] > SCREEN_HEIGHT:
                f["active"] = False
                self.lives -= 1
                misses += 1
        return catches, misses

    def _get_obs(self) -> np.ndarray:
        return build_observation(
            self._basket["x"],
            self._fruits,
            basket_w=self._basket["w"],
            dy_norm_div=self._obs_dy_div,
            r_norm_div=self._obs_r_div,
        )

    def step(self, action: int):
        keys = action_to_key_bits(int(action))
        self.frame_counter += 1
        self._update_basket(keys)
        if self.frame_counter % self._spawn_every == 0:
            self._spawn_fruit()
        catches, misses = self._update_fruits()
        reward = float(catches) - float(misses)
        if self.lives <= 0:
            self.running = False
        terminated = not self.running
        truncated = False
        obs = self._get_obs()
        return obs, reward, terminated, truncated, {}

    def _ensure_pygame(self) -> None:
        if self._pg_inited:
            return
        import pygame

        pygame.init()
        pygame.font.init()
        self._pg_inited = True

    def _draw_frame(self, surf) -> None:
        import pygame

        sc = self.render_scale
        surf.fill((0, 0, 0))
        gy = SCREEN_HEIGHT - 2
        pygame.draw.rect(
            surf,
            (132, 132, 132),
            (0, gy * sc, SCREEN_WIDTH * sc, 2 * sc),
        )
        bx, by, bw, bh = (
            self._basket["x"],
            self._basket["y"],
            self._basket["w"],
            self._basket["h"],
        )
        pygame.draw.rect(
            surf,
            (160, 80, 40),
            (bx * sc, by * sc, bw * sc, bh * sc),
        )
        pygame.draw.rect(
            surf,
            (255, 220, 0),
            ((bx - 2) * sc, (by - 2) * sc, (bw + 4) * sc, 2 * sc),
        )
        for f in self._fruits:
            if not f["active"]:
                continue
            color = f.get("color", (255, 200, 80))
            cx, cy, r = f["x"] * sc, f["y"] * sc, max(1, f["r"] * sc)
            pygame.draw.circle(surf, color, (cx, cy), r)

        try:
            font = pygame.font.SysFont("monaco", 14 * max(1, sc // 2))
        except Exception:
            font = pygame.font.Font(None, 16 * sc)
        hud = font.render(
            f"score {self.score}   lives {self.lives}   frame {self.frame_counter}",
            True,
            (220, 220, 220),
        )
        surf.blit(hud, (6 * sc, 4 * sc))

    def render(self):
        """Window for ``human``; HxWx3 uint8 array for ``rgb_array`` (no window)."""
        if self.render_mode not in ("human", "rgb_array"):
            return None

        import pygame

        self._ensure_pygame()
        sc = self.render_scale
        w, h = SCREEN_WIDTH * sc, SCREEN_HEIGHT * sc

        if self.render_mode == "human":
            if self._pg_screen is None:
                self._pg_screen = pygame.display.set_mode((w, h))
                pygame.display.set_caption("Catch Game — trained agent")
                self._pg_clock = pygame.time.Clock()
            surf = self._pg_screen
        else:
            surf = pygame.Surface((w, h))

        self._draw_frame(surf)

        if self.render_mode == "human":
            pygame.event.pump()
            pygame.display.flip()
            return None

        rgb = pygame.surfarray.array3d(surf)
        return np.transpose(rgb, (1, 0, 2)).astype(np.uint8)

    def close(self) -> None:
        if self._pg_inited:
            import pygame

            pygame.quit()
            self._pg_inited = False
            self._pg_screen = None
            self._pg_clock = None
