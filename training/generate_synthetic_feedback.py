#!/usr/bin/env python3
"""Synthetic training-data generator for OpenDeFeedback (STUB / MVP 3).

This is a placeholder that documents the intended interface. It does NOT yet
produce real datasets. See training/README.md for the full plan.

Planned output per example:
  * a mixed mono signal (clean + noise + room + optional feedback)
  * per-frame ground-truth confidence masks: feedback / noise / room / direct
  * per-frame feature vectors matching src/ml/FeatureExtractor.h

Intentionally dependency-light so it can be run as-is to show the interface.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from typing import List


# Keep in sync with src/ml/FeatureExtractor.h (FeatureFrame layout).
NUM_FEATURE_BANDS = 8
NUM_MASKS = 4  # feedback, noise, room, direct


@dataclass
class SynthConfig:
    sample_rate: int = 48000
    duration_s: float = 4.0
    feedback_prob: float = 0.5          # chance an example contains feedback
    feedback_freq_hz: tuple = (200.0, 6000.0)
    feedback_q: tuple = (10.0, 40.0)
    noise_levels_db: tuple = (-60.0, -30.0)
    room_rt60_s: tuple = (0.2, 1.2)
    seed: int = 0
    out_dir: str = "data"


@dataclass
class Example:
    audio: List[float] = field(default_factory=list)
    masks: List[List[float]] = field(default_factory=list)     # [frames][NUM_MASKS]
    features: List[List[float]] = field(default_factory=list)  # [frames][feat]


def synthesize_feedback_tone(cfg: SynthConfig):
    """TODO: generate an exponentially-growing resonant tone (howl-round)."""
    raise NotImplementedError("Real feedback synthesis lands in MVP 3.")


def generate_example(cfg: SynthConfig) -> Example:
    """TODO: mix clean + noise + room (+feedback) and emit aligned labels."""
    raise NotImplementedError("Real data generation lands in MVP 3.")


def main():
    ap = argparse.ArgumentParser(description="Generate synthetic OpenDeFeedback data (STUB).")
    ap.add_argument("--num", type=int, default=1000, help="number of examples")
    ap.add_argument("--out", type=str, default="data", help="output directory")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    cfg = SynthConfig(out_dir=args.out, seed=args.seed)
    print("[generate_synthetic_feedback] STUB — no data written.")
    print(f"  would generate : {args.num} examples")
    print(f"  sample rate    : {cfg.sample_rate} Hz")
    print(f"  feature bands  : {NUM_FEATURE_BANDS}")
    print(f"  masks per frame: {NUM_MASKS} (feedback, noise, room, direct)")
    print(f"  output dir     : {cfg.out_dir}")
    print("  See training/README.md for the implementation plan.")


if __name__ == "__main__":
    main()
