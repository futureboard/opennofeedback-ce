// ============================================================================
//  TinyModelStub.h   (ML placeholder)
//
//  Stable interface for a future tiny embedded model (GRU/MLP) that will turn a
//  FeatureFrame into per-aspect confidence masks. In the MVP it returns neutral
//  values, so multiplying any DSP amount by these confidences leaves the audio
//  unchanged.
//
//  Future flow:
//      FeatureFrame -> TinyModel -> Confidences -> DSP suppression scaling
//
//  Realtime-safety note:
//    Infer() is pure arithmetic over fixed-size data — no allocation, locking,
//    logging or file I/O. The stub is trivially realtime-safe.
// ============================================================================
#pragma once

#include "FeatureExtractor.h"

namespace odf
{

/** Per-aspect confidence masks in 0..1.
 *  Neutral defaults (feedback/noise/room = 0, direct = 1) mean "the model is
 *  not asking for any extra suppression" — the DSP behaves exactly as its hand
 *  tuned path dictates. */
struct Confidences
{
  float feedback = 0.0f; // likelihood the frame contains feedback
  float noise    = 0.0f; // likelihood of steady noise
  float room     = 0.0f; // likelihood of reverberant tail
  float direct   = 1.0f; // likelihood of wanted direct signal
};

class TinyModelStub
{
public:
  TinyModelStub() = default;

  void Prepare(double sampleRate);
  void Reset();

  /** Returns neutral confidences in the MVP. Realtime-safe. */
  Confidences Infer(const FeatureFrame& features) noexcept;

  /** Whether a real model is loaded. Always false for the stub. */
  bool IsModelLoaded() const noexcept { return mModelLoaded; }

private:
  bool mModelLoaded = false; // becomes true once real weights are embedded
};

} // namespace odf
