// ============================================================================
//  FeedbackDetector.h
//
//  Lightweight, FFT-free feedback (acoustic howl-round) detector.
//
//  Strategy: a fixed log-spaced bank of resonant bandpass analysis filters runs
//  over a mono mix of the input. Per band we track:
//    * short-window energy ("fast")
//    * long-window energy ("slow")
//    * growth rate (fast vs slow) -> ringing that is building up
//    * peak persistence            -> ringing that stays put over time
//    * narrowness vs neighbours     -> tonal peak rather than broadband content
//  These cues are combined into a per-band confidence. The strongest bands are
//  reported as FeedbackCandidates for the notch bank to act on.
//
//  Realtime-safety note:
//    All processing happens in fixed-size arrays sized in Prepare(). No
//    allocation, locking, logging or file access occurs in Process().
// ============================================================================
#pragma once

#include <array>
#include "Biquad.h"

namespace odf
{

/** A single suspected feedback resonance. */
struct FeedbackCandidate
{
  float frequencyHz = 0.0f;
  float confidence  = 0.0f; // 0..1
  float q           = 0.0f; // suggested notch Q
};

class FeedbackDetector
{
public:
  // Tuning constants. Kept as compile-time sizes so all storage is fixed.
  static constexpr int kNumBands      = 28;  // analysis bands
  static constexpr int kMaxCandidates = 8;   // matches the notch bank size

  FeedbackDetector() = default;

  void Prepare(double sampleRate, int maxBlockSize);
  void Reset();

  /** Sensitivity 0..1 (driven by Feedback Amount * Strength). Higher == more
   *  eager detection / lower confidence threshold. Realtime-safe. */
  void SetSensitivity(float s) noexcept { mSensitivity = Clamp01(s); }

  /** Analyse one block (mono mix supplied by the caller's scratch buffer).
   *  Returns the number of valid candidates written to OutCandidates().
   *  Realtime-safe. */
  int Process(const float* monoMix, int nFrames) noexcept;

  /** Candidate array, valid up to the count returned by Process(). */
  const std::array<FeedbackCandidate, kMaxCandidates>& Candidates() const noexcept
  {
    return mCandidates;
  }

  int NumCandidates() const noexcept { return mNumCandidates; }

private:
  struct Band
  {
    Biquad bandpass;
    float  centreHz   = 0.0f;
    float  fastEnergy = 0.0f;
    float  slowEnergy = 0.0f;
    float  persistence = 0.0f; // 0..1
  };

  double mSampleRate = 44100.0;
  float  mSensitivity = 1.0f;

  float mFastCoeff = 0.0f;
  float mSlowCoeff = 0.0f;
  float mPersistRise = 0.0f;
  float mPersistFall = 0.0f;

  std::array<Band, kNumBands> mBands;
  std::array<FeedbackCandidate, kMaxCandidates> mCandidates;
  int mNumCandidates = 0;
};

} // namespace odf
