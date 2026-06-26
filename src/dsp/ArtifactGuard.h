// ============================================================================
//  ArtifactGuard.h
//
//  Final safety stage that prevents over-processing. By comparing the wet
//  (processed) signal against the dry (input) signal it measures the net
//  attenuation the upstream stages have applied, and limits that attenuation to
//  a ceiling set by the Artifact Guard parameter. This avoids sudden tonal
//  collapse / pumping when the detectors over-react.
//
//  Conceptually: out = wet, but never quieter than (dry * minGain), where
//  minGain is governed by the guard amount. The correction gain is smoothed.
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

class ArtifactGuard
{
public:
  static constexpr int kMaxChannels = 2;

  ArtifactGuard() = default;

  void Prepare(double sampleRate, int numChannels);
  void Reset();

  /** Guard amount 0..1. 0 == allow heavy processing, 1 == very gentle (limits
   *  net attenuation to a few dB). Realtime-safe. */
  void SetGuard(float guard) noexcept;

  /** Apply the guard in place. `dry` is the unprocessed input captured before
   *  the chain ran; `wet` (== io) is the processed signal to be limited.
   *  Realtime-safe. */
  void ProcessBlock(const float* const* dry, float** wet, int nFrames, int nChannels) noexcept;

private:
  struct ChannelState
  {
    EnvelopeFollower dryEnv;
    EnvelopeFollower wetEnv;
    SmoothedValue    correction; // smoothed make-up gain toward the floor
  };

  double mSampleRate = 44100.0;
  int    mNumChannels = 2;
  float  mMinGain = 0.125f; // ~ -18 dB ceiling by default

  std::array<ChannelState, kMaxChannels> mChannels;
};

} // namespace odf
