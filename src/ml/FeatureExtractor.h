// ============================================================================
//  FeatureExtractor.h   (ML placeholder)
//
//  Extracts a small, fixed-size feature vector from a mono analysis block. In
//  the MVP these features are computed but only lightly used (they feed the
//  inert TinyModelStub). They define the contract a future tiny embedded model
//  will consume.
//
//  Realtime-safety note:
//    Output lives in a fixed-size struct (no allocation). Process() does no
//    locking, logging or file I/O. Designed to be cheap enough for the audio
//    thread; a real model would likely run on a decimated/strided schedule.
// ============================================================================
#pragma once

#include <array>

namespace odf
{

/** Fixed-size feature frame. Keep this stable — model weights are trained
 *  against this layout. */
struct FeatureFrame
{
  static constexpr int kNumBands = 8;

  std::array<float, kNumBands> bandEnergy {};  // coarse log-band energies (dB-ish)
  float rms             = 0.0f;                // block RMS
  float spectralCentroid = 0.0f;               // normalised 0..1
  float spectralFlux     = 0.0f;               // change vs previous frame
  float crestFactor      = 0.0f;               // peak / rms (transient-ness)
};

class FeatureExtractor
{
public:
  FeatureExtractor() = default;

  void Prepare(double sampleRate, int maxBlockSize);
  void Reset();

  /** Compute features for one mono block. Realtime-safe. */
  const FeatureFrame& Process(const float* monoMix, int nFrames) noexcept;

  const FeatureFrame& Last() const noexcept { return mFrame; }

private:
  double mSampleRate = 44100.0;
  FeatureFrame mFrame;
  std::array<float, FeatureFrame::kNumBands> mPrevBandEnergy {};
};

} // namespace odf
