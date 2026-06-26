# OpenDeFeedback — DSP Design Notes

This document explains *why* each DSP stage works the way it does. The guiding
principle for the MVP is **readable, conservative, realtime-safe DSP** over
perfect algorithms. None of the stages use lookahead; everything is causal so
the plugin reports **0 samples latency**.

All filters are RBJ biquads (`dsp/Biquad.h`) in Transposed Direct Form II.
Coefficient design uses transcendental math, so audible parameters (notch
frequency/depth, gains) are smoothed and coefficients are only recomputed when
the smoothed value has moved enough to matter.

## FeedbackDetector

FFT-free, to keep latency and CPU low. A fixed bank of `kNumBands` (28)
log-spaced resonant **bandpass** analysis filters (150 Hz … ~12 kHz) extracts
per-band energy from a mono mix of the input. For each band we track:

* **fast energy** — short (~15 ms) envelope of band power: the current ring.
* **slow energy** — long (~400 ms) envelope: a moving reference.
* **growth** — `fast/slow`; a band whose energy is climbing above its own
  recent average is *building up* (the signature of acoustic howl-round).
* **narrowness** — band energy vs the average of its neighbours; feedback is a
  narrow tonal peak, not broadband content. This is the main defence against
  false-triggering on musical notes.
* **persistence** — the instantaneous "narrow + above floor" cue integrated
  over time (slow rise, quick fall). Real feedback sticks around for hundreds
  of ms; a plucked note or vowel does not.

Confidence combines persistence and growth. The strongest bands above a
sensitivity-dependent threshold are emitted as `FeedbackCandidate`s (frequency,
confidence, suggested Q), skipping immediate neighbours so two notches never
land on the same peak.

> Limitation: band resolution is coarse (geometric spacing). A future version
> could refine the exact frequency with a short Goertzel or parabolic
> interpolation around the winning band, or hand the decision to the ML model.

## AdaptiveNotchBank

A fixed bank of `kMaxNotches` (8) negative-gain peaking filters (a finite
"notch" is gentler on transients than an infinitely deep one). Each block the
bank matches incoming candidates to existing notches within ~1 semitone (so a
drifting resonance is *tracked* rather than re-allocated), claims free slots for
new candidates, and smoothly releases notches whose candidate disappeared.

Frequency and depth are smoothed (`SmoothedValue`) to avoid zipper noise; depth
scales with candidate confidence and is clamped to the user/guard ceiling. A
released notch is only freed once its depth has glided back to ~0 dB.

## NoiseEstimator

A conservative, FFT-free **band expander** — *not* spectral subtraction. The
signal is split into 3 bands with complementary (Butterworth Q≈0.707,
allpass-sum) crossovers so that at unity gain the bands reconstruct flat. Each
band runs:

* an envelope follower,
* a noise-floor tracker (fast down / slow up — settles on the quiet level),
* a soft **downward expander** around `floor + margin`.

Below the tracked floor the band is gently attenuated (max cut scales with the
Noise Amount control); above it, programme material passes untouched. Gains are
smoothed to avoid musical noise. Kept deliberately subtle in the MVP.

## DereverbTailSuppressor

Per channel it compares a **fast** envelope (direct sound + transients) to a
**slow** envelope (the decaying tail). On onsets the fast envelope dominates
(`fast/slow >> 1`) and gain stays at unity, preserving transients. As the signal
decays into a reverberant tail the slow envelope dominates and a gentle gain
reduction (max ~−10 dB, scaled by Room Amount) is applied. This is a light,
causal tail-ducker, not offline-grade dereverb.

## ArtifactGuard

The safety net. It measures the **net** attenuation the chain applied by
comparing wet vs dry envelopes per channel. If the chain has cut more than the
Artifact Guard parameter allows (a floor from −30 dB at guard=0 to −3 dB at
guard=1), it applies a smoothed make-up gain that brings the net attenuation
back up to exactly the floor. This prevents sudden tonal collapse / pumping when
the detectors over-react.

## Strength & Mode

* **Strength** is the final global dry/wet blend (0 = bypass-through,
  1 = fully processed), smoothed per sample.
* **Mode** masks and scales stages: e.g. *Feedback Only* runs just the
  detector + notch bank; *Speech* biases toward stronger noise reduction and
  lighter dereverb; *Instrument* drops dereverb and softens noise reduction.

## Numerical hygiene

* `FlushDenorm` zeroes sub-1e-15 values in every recursive path.
* All public DSP entry points are `noexcept` and free of allocation.
* `Prepare()` is the only place that allocates or computes expensive constants.
