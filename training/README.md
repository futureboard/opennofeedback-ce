# OpenDeFeedback — Training (future / MVP 3)

This folder describes the **planned** machine-learning pipeline. It is **not**
used by the plugin yet — the plugin ships a neutral `TinyModelStub` and runs
entirely on hand-tuned DSP. Nothing here runs inside the realtime audio path.

## Goals

Train a **tiny** model (a small GRU or MLP, a few thousand parameters) that, per
short analysis frame, predicts soft **confidence masks** rather than a clean
waveform:

* `feedback`  — probability the frame contains acoustic feedback,
* `noise`     — probability of steady background noise,
* `room`      — probability of reverberant tail energy,
* `direct`    — probability of wanted direct signal.

These confidences will *scale* the existing DSP suppression stages. We predict
masks (not audio) so the model stays tiny, interpretable, and cheap enough to
run with custom C++ inference using embedded weights — **no ONNX Runtime, no
Python, no file I/O at audio time.**

## Why masks, not waveforms

* Orders of magnitude fewer parameters than a generative denoiser.
* Deterministic, bounded CPU; easy to keep realtime-safe.
* Degrades gracefully — a wrong mask just nudges a DSP amount; it can't inject
  artefacts the way a waveform-predicting net can.

## Data generation (`generate_synthetic_feedback.py`)

Synthesise labelled training data by mixing known components, so the masks are
known exactly:

1. **Clean speech / vocals / instruments** (dry sources).
2. **Noise** — hiss, hum, HVAC, crowd, etc.
3. **Room** — convolve dry sources with measured/synthetic room impulse
   responses to create reverberant tails.
4. **Feedback** — synthetic exponentially-growing resonant tones at random
   frequencies/Q (and optionally a simple closed-loop room+mic+gain model) to
   emulate howl-round onset.

The mixer records, per frame, the ground-truth presence/level of each component
to form the target masks. Feature extraction must **match the C++
`FeatureExtractor` layout** (`src/ml/FeatureExtractor.h`).

## Training (`train_feedback_stub.py`)

* PyTorch, offline, GPU optional.
* Inputs: per-frame feature vectors (same layout as the C++ extractor).
* Targets: the 4 confidence masks.
* Loss: per-mask BCE / MSE; class-balance feedback (it's rare but important).
* Keep the network tiny and quantisation-friendly.

## Export (`export_model_stub.py`)

* Quantise to int8 (or small float) weights.
* Emit a C header replacing `src/ml/ModelWeightsStub.h` (weights as a `const`
  array + scales/zero-points + a bumped `kWeightsVersion`).
* The runtime `TinyModelStub` (to be implemented) loads these embedded weights;
  no external runtime is required.

## Status

All three scripts here are **stubs** that document the intended interface and
print what they *would* do. They are safe to run and have no heavyweight
dependencies. Replace them with real implementations in MVP 3.
