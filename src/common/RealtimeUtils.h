// ============================================================================
//  RealtimeUtils.h
//
//  Small, header-only helpers that are safe to call from the realtime audio
//  callback. Everything here is branch-light, allocation-free and lock-free.
// ============================================================================
#pragma once

#include <cmath>
#include <algorithm>

namespace odf
{

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------
static constexpr float kPi      = 3.14159265358979323846f;
static constexpr float kTwoPi   = 2.0f * kPi;
static constexpr float kMinGain = 1.0e-6f;   // ~ -120 dB, used to avoid log(0)

// ---------------------------------------------------------------------------
//  Denormal protection
//
//  Tiny residual values in feedback paths (filters, envelope followers) can
//  drop into the denormal range and cause large CPU spikes on some CPUs.
//  Flushing to zero is cheaper and realtime-safe.
// ---------------------------------------------------------------------------
inline float FlushDenorm(float x) noexcept
{
  // 1e-15 is comfortably below the audible noise floor.
  return (std::fabs(x) < 1.0e-15f) ? 0.0f : x;
}

// ---------------------------------------------------------------------------
//  dB <-> linear
// ---------------------------------------------------------------------------
inline float DbToGain(float db) noexcept
{
  return std::pow(10.0f, db * 0.05f);
}

inline float GainToDb(float gain) noexcept
{
  return 20.0f * std::log10(std::max(gain, kMinGain));
}

// ---------------------------------------------------------------------------
//  Clamping / mapping
// ---------------------------------------------------------------------------
inline float Clamp(float x, float lo, float hi) noexcept
{
  return std::min(std::max(x, lo), hi);
}

inline float Clamp01(float x) noexcept
{
  return Clamp(x, 0.0f, 1.0f);
}

/** Linear interpolation. t is expected in 0..1 but is not clamped. */
inline float Lerp(float a, float b, float t) noexcept
{
  return a + (b - a) * t;
}

// ---------------------------------------------------------------------------
//  One-pole smoothing coefficient from a time constant.
//
//  Returns the per-sample feedback coefficient for a one-pole lowpass that
//  reaches ~63% of a step in `timeMs`. Used by envelope followers and value
//  smoothers. Computed outside the audio loop (uses pow), so callers should
//  cache the result and only recompute on sample-rate / time changes.
// ---------------------------------------------------------------------------
inline float OnePoleCoeff(float timeMs, double sampleRate) noexcept
{
  if (timeMs <= 0.0f)
    return 0.0f;
  const float t = static_cast<float>(timeMs * 0.001 * sampleRate);
  return std::exp(-1.0f / std::max(t, 1.0f));
}

} // namespace odf
