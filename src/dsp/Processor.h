// ============================================================================
//  Processor.h
//
//  Central realtime DSP engine for OpenDeFeedback. Owns and chains the analysis
//  and suppression stages:
//
//      input -> (mono analysis) -> FeedbackDetector -> AdaptiveNotchBank
//            -> NoiseEstimator -> DereverbTailSuppressor -> ArtifactGuard
//            -> Strength dry/wet blend -> output
//
//  The ML stubs (FeatureExtractor / TinyModelStub) are wired in but currently
//  return neutral confidences and therefore do not alter the audio. They mark
//  the integration point for a future tiny embedded model.
//
//  Realtime-safety contract (see also the per-module headers):
//    * All buffers are allocated in Prepare(); none in ProcessBlock().
//    * No locks, no file I/O, no logging, no exceptions in ProcessBlock().
//    * Parameters are delivered as a copied POD snapshot via SetParameters().
//    * Metering values are published through std::atomic for the UI thread.
// ============================================================================
#pragma once

#include <vector>
#include <atomic>

#include "../common/Parameters.h"
#include "FeedbackDetector.h"
#include "AdaptiveNotchBank.h"
#include "NoiseEstimator.h"
#include "DereverbTailSuppressor.h"
#include "ArtifactGuard.h"
#include "SmoothedValue.h"
#include "../ml/FeatureExtractor.h"
#include "../ml/TinyModelStub.h"

namespace odf
{

class Processor
{
public:
  Processor() = default;

  /** Allocate all working memory. NOT realtime-safe — call from the plugin's
   *  OnReset()/prepare path. */
  void Prepare(double sampleRate, int maxBlockSize, int numChannels);

  /** Clear all internal state without reallocating. Realtime-safe. */
  void Reset();

  /** Push a new parameter snapshot. Cheap; safe to call from the audio thread
   *  (the plugin builds the snapshot on the message thread and copies it in).*/
  void SetParameters(const Parameters& params) noexcept;

  /** Process one block. inputs/outputs may alias the same buffers.
   *  Realtime-safe. */
  void ProcessBlock(float** inputs, float** outputs, int nFrames, int nChannels) noexcept;

  // ---- Metering / readouts (UI thread reads these atomics) ---------------
  float GetDetectedFreqHz() const noexcept { return mDetectedFreqHz.load(std::memory_order_relaxed); }
  float GetReductionDb()    const noexcept { return mReductionDb.load(std::memory_order_relaxed); }
  int   GetActiveNotches()  const noexcept { return mActiveNotches.load(std::memory_order_relaxed); }
  int   GetLatencySamples() const noexcept { return 0; } // strictly causal, zero-latency

private:
  // Per-mode enable flags & scaling, derived from Parameters + Mode.
  struct StageConfig
  {
    bool  feedbackOn = true;
    bool  noiseOn    = true;
    bool  roomOn     = true;
    float feedbackSensitivity = 1.0f;
    float feedbackMaxDepthDb   = -18.0f;
    float noiseAmount = 0.35f;
    float roomAmount  = 0.25f;
    float guard       = 0.75f;
    float strength    = 1.0f;
    bool  mute        = false;
    bool  bypass      = false;
  };

  void ApplyConfig(const StageConfig& cfg) noexcept;

  static constexpr int kMaxChannels = 2;

  double mSampleRate = 44100.0;
  int    mMaxBlockSize = 512;
  int    mNumChannels = 2;

  // DSP stages.
  FeedbackDetector       mFeedbackDetector;
  AdaptiveNotchBank      mNotchBank;
  NoiseEstimator         mNoiseEstimator;
  DereverbTailSuppressor mDereverb;
  ArtifactGuard          mArtifactGuard;
  SmoothedValue          mStrength;

  // ML placeholders (inert in the MVP).
  FeatureExtractor mFeatureExtractor;
  TinyModelStub    mTinyModel;

  // Preallocated scratch buffers.
  std::vector<float>               mMonoMix;            // [maxBlockSize]
  std::array<std::vector<float>, kMaxChannels> mDry;    // dry copy per channel
  std::array<float*, kMaxChannels> mDryPtrs{};          // stable pointer array

  // Cached parameters / derived config (written by SetParameters).
  StageConfig mConfig;

  // Metering (published to UI thread).
  std::atomic<float> mDetectedFreqHz {0.0f};
  std::atomic<float> mReductionDb    {0.0f};
  std::atomic<int>   mActiveNotches  {0};
};

} // namespace odf
