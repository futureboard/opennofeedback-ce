// ============================================================================
//  DereverbTailSuppressor.h
//
//  Subtle late-reverb-tail suppressor. Per channel it compares a fast envelope
//  (direct sound + transients) against a slow envelope (the decaying tail). On
//  transients/onsets the fast envelope dominates and gain is left at unity; as
//  the signal decays into a tail the slow envelope dominates and a gentle gain
//  reduction is applied. Gains are smoothed so transients are preserved.
//
//  This is a light, causal, zero-latency effect — not offline-grade dereverb.
//
//  Realtime-safety note:
//    Fixed-size per-channel state, sized in Prepare(). ProcessBlock() does no
//    allocation, locking, logging or file I/O.
// ============================================================================
#pragma once

#include <array>
#include "EnvelopeFollower.h"
#include "SmoothedValue.h"

namespace odf
{

class DereverbTailSuppressor
{
public:
  static constexpr int kMaxChannels = 2;

  DereverbTailSuppressor() = default;

  void Prepare(double sampleRate, int numChannels);
  void Reset();

  /** Amount 0..1 (driven by Room Amount * Strength). 0 == bypassed.
   *  Realtime-safe. */
  void SetAmount(float amount) noexcept;

  /** Process one block in place per channel. Realtime-safe. */
  void ProcessBlock(float** io, int nFrames, int nChannels) noexcept;

  float LastReductionDb() const noexcept { return mLastReductionDb; }

private:
  struct ChannelState
  {
    EnvelopeFollower fast;  // direct / transient energy
    EnvelopeFollower slow;  // tail energy
    SmoothedValue    gain;  // smoothed linear gain
  };

  double mSampleRate = 44100.0;
  int    mNumChannels = 2;
  float  mAmount = 0.25f;
  float  mMaxReductionDb = -8.0f;
  float  mLastReductionDb = 0.0f;

  std::array<ChannelState, kMaxChannels> mChannels;
};

} // namespace odf
