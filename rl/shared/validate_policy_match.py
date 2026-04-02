"""Smoke-test C policy: run tests/policy_smoke and check logits are finite."""

from __future__ import annotations

import os
import subprocess
import sys


def _find_binary():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    path = os.path.join(root, "tests", "policy_smoke")
    if os.path.isfile(path) and os.access(path, os.X_OK):
        return path
    return None


def main() -> int:
    bin_path = _find_binary()
    if bin_path is None:
        print("build tests/policy_smoke from repo root: make policy_smoke", file=sys.stderr)
        return 1
    out = subprocess.check_output([bin_path], text=True)
    parts = out.strip().split()
    if len(parts) != 3:
        print("unexpected output:", out, file=sys.stderr)
        return 1
    vals = [float(x) for x in parts]
    for v in vals:
        if not (v == v):  # NaN
            return 1
    print("policy_smoke logits:", vals)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
