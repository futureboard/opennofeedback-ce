# OpenDeFeedback — Architecture

OpenDeFeedback is a realtime, zero-latency live-audio cleanup plugin. The
codebase is split into three layers so the DSP can be developed, tested and
(eventually) embedded independently of the plugin host glue.

```
src/
  common/   POD parameter snapshot + realtime-safe utilities
  dsp/      the realtime DSP engine and its stages
  ml/       placeholder interfaces for a future tiny embedded model
  plugin/   iPlug2 host glue + IGraphics/NanoVG GUI
```

## Threading model

There are two threads that matter:

* **Audio (realtime) thread** — runs `OpenDeFeedback::ProcessBlock`, which
  converts the host's `sample` buffers to `float`, calls
  `odf::Processor::ProcessBlock`, and converts back. Everything reachable from
  here obeys the realtime contract (see below).
* **Message / UI thread** — host parameter changes arrive via
  `OnParamChange`, which builds an `odf::Parameters` snapshot and copies it
  into the processor. The GUI is updated from `OnIdle`.

Data crosses threads in exactly two safe ways:

1. **Parameters** flow UI → audio as a copied POD struct (`odf::Parameters`).
   No shared mutable parameter object, so no locking.
2. **Metering / readouts** flow audio → UI via:
   * iPlug2 `IPeakSender` lock-free queues for the input/output level meters,
   * `std::atomic<float/int>` in `odf::Processor` for the detected frequency,
     reduction amount and active-notch count, read in `OnIdle`.

## Realtime-safety contract

Inside `Processor::ProcessBlock` (and everything it calls):

* no heap allocation,
* no locks / mutexes,
* no file I/O,
* no logging,
* no exceptions.

All buffers are sized in `Prepare()`. Fixed-size `std::array`s are used for the
feedback-candidate list and the notch bank so their capacity is known at
compile time. Filter state and envelopes flush denormals to zero to avoid CPU
spikes.

## Signal flow

```
Audio In
  -> mono mix (analysis only)              [Processor]
  -> FeatureExtractor -> TinyModelStub     [ml, inert in MVP]
  -> FeedbackDetector  (candidates)        [dsp]
  -> AdaptiveNotchBank (dynamic notches)   [dsp]
  -> NoiseEstimator    (band expander)     [dsp]
  -> DereverbTailSuppressor (tail ducking) [dsp]
  -> ArtifactGuard     (limits net cut)    [dsp]
  -> Strength dry/wet blend                [Processor]
Audio Out
```

The `Mode` parameter masks/scales which stages are active (e.g. *Feedback Only*
disables the noise and reverb stages). `Strength` is the global dry/wet blend
applied at the very end. `Artifact Guard` caps how much total attenuation the
chain is allowed to apply.

## ML integration point (future)

`FeatureExtractor` produces a fixed-size `FeatureFrame`; `TinyModelStub::Infer`
maps it to a `Confidences` struct (feedback / noise / room / direct). In the
MVP the stub returns neutral values and does **not** affect audio. When a
trained model lands (MVP 3), those confidences will scale the per-stage
suppression amounts. See [docs/dsp_design.md](dsp_design.md) and
[training/README.md](../training/README.md).
