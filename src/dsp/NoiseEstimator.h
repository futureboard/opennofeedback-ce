// ============================================================================
//  NoiseEstimator.h
//
//  Conservative, FFT-free broadband noise reducer. The signal is split into a
//  few bands with complementary (allpass-sum) crossovers. Each band runs a
//  slow noise-floor tracker and a gentle downward expander, so steady low-level
//  hiss/hum below the tracked floor is attenuated while programme material
//  above it passes through. Gains are smoothed to avoid musical noise.
//
//  This is intentionally subtle in the MVP — it is a band expander, not a
//  spectral subtraction engine.
//
//  Realtime-safety note:
//    All state lives in fixed-size arrays sized in Prepare(). ProcessBlock()
//    performs no allocation, locking, logging or file I/O.
// ============================================================================
#pragma once

#include <array>
#include "Biquad.h"
#include "SmoothedValue.h"

namespace odf
{

class NoiseEstimator
{
public:
  static constexpr int kNumBands    = 3;
  static constexpr int kMaxChannels = 2;

  NoiseEstimator() = default;

  void Prepare(double sampleRate, int numChannels);
  void Reset();

  /** Amount 0..1 (driven by Noise Amount * Strength). 0 == bypassed.
   *  Realtime-safe. */
  void SetAmount(float amount) noexcept;

  /** Process one block in place per channel. Realtime-safe. */
  void ProcessBlock(float** io, int nFrames, int nChannels) noexcept;

  /** Most recent total gain reduction applied (dB, negative). For metering. */
  float LastReductionDb() const noexcept { return mLastReductionDb; }

private:
  struct BandState
  {
    // Crossover filters needed to isolate this band on a given channel.
    // (Filled out per channel in the channel state.)
    SmoothedValue gain; // smoothed linear gain applied to the band
    float envelope = 0.0f;
    float noiseFloor = 1.0e-4f;
  };

  struct ChannelState
  {
    // Crossover topology: split at f1 and f2.
    Biquad lp1, hp1;   // low / (mid+high)
    Biquad lp2, hp2;   // mid / high (applied to hp1 output)
    std::array<BandState, kNumBands> bands;
  };

  double mSampleRate = 44100.0;
  int    mNumChannels = 2;
  float  mAmount = 0.35f;

  float mEnvCoeff = 0.0f;
  float mFloorRise = 0.0f; // floor tracks upward slowly
  float mFloorFall = 0.0f; // floor follows downward quickly

  float mMaxReductionDb = -12.0f;
  float mLastReductionDb = 0.0f;

  std::array<ChannelState, kMaxChannels> mChannels;
};

} // namespace odf
