// ============================================================================
//  AdaptiveNotchBank.h
//
//  A small fixed-size bank of dynamic notch (negative-gain peaking) filters.
//  The bank is fed FeedbackCandidates by the Processor; it allocates notches to
//  candidates, smooths their frequency / depth to avoid zipper noise, and
//  releases notches when a candidate disappears.
//
//  Realtime-safety note:
//    Storage is a fixed std::array sized at compile time. Process()/Update()
//    perform no allocation or locking. Biquad coefficient redesign happens only
//    when a notch's smoothed frequency/depth has moved enough to matter.
// ============================================================================
#pragma once

#include <array>
#include "Biquad.h"
#include "SmoothedValue.h"
#include "FeedbackDetector.h"

namespace odf
{

class AdaptiveNotchBank
{
public:
  static constexpr int kMaxNotches = 8;

  AdaptiveNotchBank() = default;

  void Prepare(double sampleRate, int numChannels);
  void Reset();

  /** Maximum cut depth in dB (negative). Driven by Feedback Amount * Strength.
   *  Realtime-safe. */
  void SetMaxDepthDb(float depthDb) noexcept { mMaxDepthDb = depthDb; }

  /** Assign / update notches from the current candidate list. Called once per
   *  block before processing audio. Realtime-safe. */
  void Update(const FeedbackCandidate* candidates, int numCandidates) noexcept;

  /** Filter one sample for a given channel. Realtime-safe. */
  inline float Process(int channel, float x) noexcept
  {
    float y = x;
    for (int i = 0; i < kMaxNotches; ++i)
    {
      if (mNotches[i].active || mNotches[i].depth.GetCurrent() < -0.01f)
        y = mNotches[i].filter[channel].Process(y);
    }
    return y;
  }

  /** Advance per-sample smoothing for all notches. Call once per sample frame
   *  (channel-independent state). Realtime-safe. */
  void Tick() noexcept;

  int NumActiveNotches() const noexcept;

private:
  static constexpr int kMaxChannels = 2;

  struct Notch
  {
    SmoothedValue freq;   // Hz
    SmoothedValue depth;  // dB (negative == cut)
    float q = 12.0f;
    bool  active = false;
    float lastDesignedFreq = 0.0f;
    float lastDesignedDepth = 0.0f;
    std::array<Biquad, kMaxChannels> filter;
  };

  void RedesignIfNeeded(Notch& n) noexcept;

  double mSampleRate = 44100.0;
  int    mNumChannels = 2;
  float  mMaxDepthDb = -18.0f;

  std::array<Notch, kMaxNotches> mNotches;
};

} // namespace odf
