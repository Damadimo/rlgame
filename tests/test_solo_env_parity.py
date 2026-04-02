"""Parity: Python env matches itself and (optional) C parity_steps binary."""

from __future__ import annotations

import os
import subprocess
import sys

import numpy as np
import pytest

from rl.solo.catch_env import CatchGameEnv, action_to_key_bits


def test_python_deterministic():
    actions = [0, 1, 2, 0, 1, 1, 2, 0] * 30
    seed = 42

    def run():
        env = CatchGameEnv()
        env.reset(seed=seed)
        rows = []
        for a in actions:
            obs, r, term, trunc, _ = env.step(a)
            rows.append(
                (
                    env.frame_counter,
                    env._basket["x"],
                    env.score,
                    env.lives,
                    1 if env.running else 0,
                    obs.copy(),
                    r,
                )
            )
            if term:
                break
        return rows

    a = run()
    b = run()
    assert len(a) == len(b)
    for ra, rb in zip(a, b):
        assert ra[:-2] == rb[:-2]
        np.testing.assert_allclose(ra[-2], rb[-2], atol=1e-6)
        assert ra[-1] == rb[-1]


def test_action_key_bits():
    assert action_to_key_bits(0) == 0
    assert action_to_key_bits(1) == 0x8
    assert action_to_key_bits(2) == 0x4


def _find_parity_binary():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    path = os.path.join(root, "tests", "parity_steps")
    if os.path.isfile(path) and os.access(path, os.X_OK):
        return path
    return None


def _find_policy_smoke():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    path = os.path.join(root, "tests", "policy_smoke")
    if os.path.isfile(path) and os.access(path, os.X_OK):
        return path
    return None


@pytest.mark.skipif(_find_policy_smoke() is None, reason="policy_smoke not built")
def test_policy_smoke_outputs_three_floats():
    proc = subprocess.run(
        [_find_policy_smoke()],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr
    parts = proc.stdout.strip().split()
    assert len(parts) == 3
    vals = [float(x) for x in parts]
    assert all(v == v for v in vals)


@pytest.mark.skipif(_find_parity_binary() is None, reason="parity_steps not built")
def test_python_matches_c():
    rng = np.random.default_rng(0)
    actions = [int(rng.integers(0, 3)) for _ in range(200)]
    seed = 999

    env = CatchGameEnv()
    env.reset(seed=seed)
    py_rows = []
    for a in actions:
        env.step(a)
        py_rows.append(
            (env.frame_counter, env._basket["x"], env.score, env.lives, 1 if env.running else 0)
        )

    bin_path = _find_parity_binary()
    proc = subprocess.run(
        [bin_path, str(seed), str(len(actions))],
        input="\n".join(str(a) for a in actions) + "\n",
        text=True,
        capture_output=True,
        cwd=os.path.dirname(bin_path),
    )
    assert proc.returncode == 0, proc.stderr
    lines = [ln.strip() for ln in proc.stdout.strip().splitlines() if ln.strip()]
    assert len(lines) == len(py_rows)
    for py, c in zip(py_rows, lines):
        parts = [int(x) for x in c.split(",")]
        assert tuple(parts) == py


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
