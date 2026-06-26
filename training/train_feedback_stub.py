#!/usr/bin/env python3
"""Training entry point for the OpenDeFeedback tiny model (STUB / MVP 3).

Placeholder that documents the intended training interface. It does NOT train
anything yet and deliberately avoids importing torch so it runs anywhere.

Planned model: a tiny GRU or MLP (~a few thousand params) mapping a per-frame
feature vector (src/ml/FeatureExtractor.h layout) to 4 confidence masks
(feedback / noise / room / direct). Trained offline in PyTorch, then quantised
and exported as embedded C weights (see export_model_stub.py).
"""

from __future__ import annotations

import argparse


NUM_FEATURE_BANDS = 8
INPUT_SIZE = NUM_FEATURE_BANDS + 4   # bands + rms, centroid, flux, crest
NUM_MASKS = 4


def build_model(hidden_size: int):
    """TODO: return a tiny torch.nn.Module (GRU/MLP). Stub for now."""
    raise NotImplementedError("Model construction lands in MVP 3 (requires torch).")


def train(args):
    print("[train_feedback_stub] STUB — no training performed.")
    print(f"  input size  : {INPUT_SIZE}")
    print(f"  hidden size : {args.hidden}")
    print(f"  outputs     : {NUM_MASKS} confidence masks")
    print(f"  epochs      : {args.epochs}")
    print(f"  data dir    : {args.data}")
    print("  Targets are soft masks, NOT clean waveforms (see training/README.md).")


def main():
    ap = argparse.ArgumentParser(description="Train OpenDeFeedback tiny model (STUB).")
    ap.add_argument("--data", type=str, default="data")
    ap.add_argument("--epochs", type=int, default=50)
    ap.add_argument("--hidden", type=int, default=16)
    ap.add_argument("--out", type=str, default="checkpoints")
    train(ap.parse_args())


if __name__ == "__main__":
    main()
