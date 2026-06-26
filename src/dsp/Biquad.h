// ============================================================================
//  Biquad.h
//
//  Transposed Direct Form II biquad filter with helpers to design the filter
//  shapes used by OpenDeFeedback (notch, peaking, low/high shelf, lowpass).
//
//  Realtime-safety note:
//    Coefficient design (SetNotch/SetPeaking/...) uses transcendental math and
//    should ideally be done sparingly. It performs no allocation or locking,
//    so it is safe to call from the audio thread when a notch needs retuning,
//    but callers smooth the *audible* parameters (freq/depth) elsewhere to
//    avoid zipper noise. Process() is trivially realtime-safe.
// ============================================================================
#pragma once

#include "../common/RealtimeUtils.h"

namespace odf
{

class Biquad
{
public:
  Biquad() = default;

  void Reset() noexcept
  {
    mZ1 = 0.0f;
    mZ2 = 0.0f;
  }

  /** Process a single sample (Transposed Direct Form II). Realtime-safe. */
  inline float Process(float x) noexcept
  {
    const float y = mB0 * x + mZ1;
    mZ1 = mB1 * x - mA1 * y + mZ2;
    mZ2 = mB2 * x - mA2 * y;
    mZ1 = FlushDenorm(mZ1);
    mZ2 = FlushDenorm(mZ2);
    return y;
  }

  /** Bypass (unity) coefficients. */
  void SetIdentity() noexcept
  {
    mB0 = 1.0f; mB1 = 0.0f; mB2 = 0.0f;
    mA1 = 0.0f; mA2 = 0.0f;
  }

  /** Notch (band-reject). `depthDb` is the attenuation at the notch centre
   *  expressed as a negative dB value (0 == no cut). Implemented as a peaking
   *  EQ with negative gain so we get a finite, smooth cut rather than an
   *  infinitely deep mathematical notch (kinder on transients). */
  void SetNotch(float freqHz, float q, float depthDb, double sampleRate) noexcept
  {
    SetPeaking(freqHz, q, depthDb, sampleRate);
  }

  /** Peaking / bell EQ. */
  void SetPeaking(float freqHz, float q, float gainDb, double sampleRate) noexcept
  {
    const float A = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = kTwoPi * Clamp(freqHz, 10.0f, static_cast<float>(sampleRate) * 0.49f)
                     / static_cast<float>(sampleRate);
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / (2.0f * std::max(q, 0.05f));

    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cw;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cw;
    const float a2 = 1.0f - alpha / A;

    Normalise(b0, b1, b2, a0, a1, a2);
  }

  /** High shelf. */
  void SetHighShelf(float freqHz, float q, float gainDb, double sampleRate) noexcept
  {
    const float A = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = kTwoPi * Clamp(freqHz, 10.0f, static_cast<float>(sampleRate) * 0.49f)
                     / static_cast<float>(sampleRate);
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / (2.0f * std::max(q, 0.05f));
    const float twoSqrtAalpha = 2.0f * std::sqrt(A) * alpha;

    const float b0 =      A * ((A + 1.0f) + (A - 1.0f) * cw + twoSqrtAalpha);
    const float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw);
    const float b2 =      A * ((A + 1.0f) + (A - 1.0f) * cw - twoSqrtAalpha);
    const float a0 =           (A + 1.0f) - (A - 1.0f) * cw + twoSqrtAalpha;
    const float a1 =  2.0f *  ((A - 1.0f) - (A + 1.0f) * cw);
    const float a2 =           (A + 1.0f) - (A - 1.0f) * cw - twoSqrtAalpha;

    Normalise(b0, b1, b2, a0, a1, a2);
  }

  /** Second-order highpass (RBJ). Q=0.707 pairs with a matching lowpass to form
   *  a magnitude-flat (allpass-sum) crossover. */
  void SetHighpass(float freqHz, float q, double sampleRate) noexcept
  {
    const float w0 = kTwoPi * Clamp(freqHz, 10.0f, static_cast<float>(sampleRate) * 0.49f)
                     / static_cast<float>(sampleRate);
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / (2.0f * std::max(q, 0.05f));

    const float b1 = -(1.0f + cw);
    const float b0 = (1.0f + cw) * 0.5f;
    const float b2 = b0;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cw;
    const float a2 = 1.0f - alpha;

    Normalise(b0, b1, b2, a0, a1, a2);
  }

  /** Constant skirt-gain bandpass (RBJ). Peak gain == Q. Used by the feedback
   *  detector's analysis bank to extract per-band energy. */
  void SetBandpass(float freqHz, float q, double sampleRate) noexcept
  {
    const float w0 = kTwoPi * Clamp(freqHz, 10.0f, static_cast<float>(sampleRate) * 0.49f)
                     / static_cast<float>(sampleRate);
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / (2.0f * std::max(q, 0.05f));

    const float b0 = alpha;
    const float b1 = 0.0f;
    const float b2 = -alpha;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cw;
    const float a2 = 1.0f - alpha;

    Normalise(b0, b1, b2, a0, a1, a2);
  }

  /** Second-order lowpass (RBJ). */
  void SetLowpass(float freqHz, float q, double sampleRate) noexcept
  {
    const float w0 = kTwoPi * Clamp(freqHz, 10.0f, static_cast<float>(sampleRate) * 0.49f)
                     / static_cast<float>(sampleRate);
    const float cw = std::cos(w0);
    const float sw = std::sin(w0);
    const float alpha = sw / (2.0f * std::max(q, 0.05f));

    const float b1 = 1.0f - cw;
    const float b0 = b1 * 0.5f;
    const float b2 = b0;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cw;
    const float a2 = 1.0f - alpha;

    Normalise(b0, b1, b2, a0, a1, a2);
  }

private:
  inline void Normalise(float b0, float b1, float b2,
                        float a0, float a1, float a2) noexcept
  {
    const float inv = 1.0f / a0;
    mB0 = b0 * inv;
    mB1 = b1 * inv;
    mB2 = b2 * inv;
    mA1 = a1 * inv;
    mA2 = a2 * inv;
  }

  // Coefficients (a0 normalised to 1).
  float mB0 = 1.0f, mB1 = 0.0f, mB2 = 0.0f;
  float mA1 = 0.0f, mA2 = 0.0f;

  // State.
  float mZ1 = 0.0f, mZ2 = 0.0f;
};

} // namespace odf
