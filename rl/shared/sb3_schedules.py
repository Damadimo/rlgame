"""FloatSchedule / LinearSchedule compatible with Stable-Baselines3 PPO.

Newer SB3 exposes these in ``stable_baselines3.common.utils``; older installs do not.
Behavior matches upstream (master) ``common/utils.py``.
"""
from __future__ import annotations

from typing import Callable, Union

Schedule = Callable[[float], float]


class ConstantSchedule:
    def __init__(self, val: float) -> None:
        self.val = val

    def __call__(self, _: float) -> float:
        return self.val

    def __repr__(self) -> str:
        return f"ConstantSchedule(val={self.val})"


class FloatSchedule:
    """Wraps a constant or callable so PPO always gets a float (avoids numpy scalar quirks)."""

    def __init__(self, value_schedule: Union["FloatSchedule", Schedule, float, int]):
        if isinstance(value_schedule, FloatSchedule):
            self.value_schedule: Schedule = value_schedule.value_schedule
        elif isinstance(value_schedule, (float, int)):
            self.value_schedule = ConstantSchedule(float(value_schedule))
        else:
            assert callable(value_schedule), (
                f"Schedule must be float or callable, not {type(value_schedule)}"
            )
            self.value_schedule = value_schedule  # type: ignore[assignment]

    def __call__(self, progress_remaining: float) -> float:
        return float(self.value_schedule(progress_remaining))

    def __repr__(self) -> str:
        return f"FloatSchedule({self.value_schedule})"


class LinearSchedule:
    """Linear from ``start`` (progress=1) to ``end`` (by ``end_fraction`` of training)."""

    def __init__(self, start: float, end: float, end_fraction: float) -> None:
        self.start = start
        self.end = end
        self.end_fraction = end_fraction

    def __call__(self, progress_remaining: float) -> float:
        if (1 - progress_remaining) > self.end_fraction:
            return self.end
        return self.start + (1 - progress_remaining) * (self.end - self.start) / self.end_fraction

    def __repr__(self) -> str:
        return f"LinearSchedule(start={self.start}, end={self.end}, end_fraction={self.end_fraction})"
