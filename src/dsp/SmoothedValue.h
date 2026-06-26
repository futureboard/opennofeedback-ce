// ============================================================================
//  SmoothedValue.h
//
//  Header-only one-pole parameter smoother. Used everywhere a control value or
//  internal gain needs to change without zipper noise.
//
//  Realtime-safety note:
//    SetTarget()/Process()/GetCurrent() are allocation-free and lock-free.
//    Reset()/SetTimeConstant() only touch scalars. The (expensive) coefficient
//    computation happens in SetTimeConstant(), which should be called from
//    Prepare(), not from the audio loop.
// ============================================================================
#pragma once

#include "../common/RealtimeUtils.h"

namespace odf
{

class SmoothedValue
{
public:
  SmoothedValue() = default;

  /** Configure the smoothing time. NOT meant for per-sample calls. */
  void SetTimeConstant(float timeMs, double sampleRate) noexcept
  {
    mCoeff = OnePoleCoeff(timeMs, sampleRate);
  }

  /** Jump immediately to a value (no smoothing). Realtime-safe. */
  void Reset(float value) noexcept
  {
    mCurrent = value;
    mTarget  = value;
  }

  /** Set the destination value. Realtime-safe. */
  inline void SetTarget(float value) noexcept { mTarget = value; }

  inline float GetTarget()  const noexcept { return mTarget; }
  inline float GetCurrent() const noexcept { return mCurrent; }

  inline bool IsSmoothing() const noexcept
  {
    return std::fabs(mTarget - mCurrent) > 1.0e-7f;
  }

  /** Advance one sample toward the target and return the new value. */
  inline float Process() noexcept
  {
    mCurrent = mTarget + (mCurrent - mTarget) * mCoeff;
    mCurrent = FlushDenorm(mCurrent);
    return mCurrent;
  }

private:
  float mCurrent = 0.0f;
  float mTarget  = 0.0f;
  float mCoeff   = 0.0f; // 0 == no smoothing (instant)
};

} // namespace odf
