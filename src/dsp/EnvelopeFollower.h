// ============================================================================
//  EnvelopeFollower.h
//
//  Classic attack/release peak envelope follower used by the de-reverb tail
//  suppressor and various detectors.
//
//  Realtime-safety note:
//    Process()/Reset()/GetEnvelope() are allocation-free and lock-free.
//    SetTimes() computes exponential coefficients and should be called from
//    Prepare(), not per sample.
// ============================================================================
#pragma once

#include "../common/RealtimeUtils.h"

namespace odf
{

class EnvelopeFollower
{
public:
  EnvelopeFollower() = default;

  void SetTimes(float attackMs, float releaseMs, double sampleRate) noexcept
  {
    mAttackCoeff  = OnePoleCoeff(attackMs,  sampleRate);
    mReleaseCoeff = OnePoleCoeff(releaseMs, sampleRate);
  }

  void Reset() noexcept { mEnv = 0.0f; }

  inline float GetEnvelope() const noexcept { return mEnv; }

  /** Feed one sample (any sign) and return the smoothed magnitude envelope. */
  inline float Process(float x) noexcept
  {
    const float mag = std::fabs(x);
    const float coeff = (mag > mEnv) ? mAttackCoeff : mReleaseCoeff;
    mEnv = mag + (mEnv - mag) * coeff;
    mEnv = FlushDenorm(mEnv);
    return mEnv;
  }

private:
  float mEnv = 0.0f;
  float mAttackCoeff  = 0.0f;
  float mReleaseCoeff = 0.0f;
};

} // namespace odf
