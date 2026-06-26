// ============================================================================
//  Processor.cpp
// ============================================================================
#include "Processor.h"
#include "../common/RealtimeUtils.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void Processor::Prepare(double sampleRate, int maxBlockSize, int numChannels)
{
  mSampleRate   = sampleRate;
  mMaxBlockSize = std::max(maxBlockSize, 1);
  mNumChannels  = std::min(std::max(numChannels, 1), kMaxChannels);

  // ---- Allocate all scratch storage up front (NEVER in ProcessBlock) -----
  mMonoMix.assign(static_cast<size_t>(mMaxBlockSize), 0.0f);
  for (int ch = 0; ch < kMaxChannels; ++ch)
  {
    mDry[ch].assign(static_cast<size_t>(mMaxBlockSize), 0.0f);
    mDryPtrs[ch] = mDry[ch].data();
  }

  // ---- Prepare stages ----------------------------------------------------
  mFeedbackDetector.Prepare(sampleRate, mMaxBlockSize);
  mNotchBank.Prepare(sampleRate, mNumChannels);
  mNoiseEstimator.Prepare(sampleRate, mNumChannels);
  mDereverb.Prepare(sampleRate, mNumChannels);
  mArtifactGuard.Prepare(sampleRate, mNumChannels);
  mFeatureExtractor.Prepare(sampleRate, mMaxBlockSize);
  mTinyModel.Prepare(sampleRate);

  mStrength.SetTimeConstant(20.0f, sampleRate);
  mStrength.Reset(mConfig.strength);

  Reset();
}

void Processor::Reset()
{
  mFeedbackDetector.Reset();
  mNotchBank.Reset();
  mNoiseEstimator.Reset();
  mDereverb.Reset();
  mArtifactGuard.Reset();
  mFeatureExtractor.Reset();
  mTinyModel.Reset();

  std::fill(mMonoMix.begin(), mMonoMix.end(), 0.0f);
  for (auto& d : mDry)
    std::fill(d.begin(), d.end(), 0.0f);

  mDetectedFreqHz.store(0.0f, std::memory_order_relaxed);
  mReductionDb.store(0.0f, std::memory_order_relaxed);
  mActiveNotches.store(0, std::memory_order_relaxed);
  mInputPeak.store(0.0f, std::memory_order_relaxed);
  mOutputPeak.store(0.0f, std::memory_order_relaxed);
}

void Processor::SetParameters(const Parameters& p) noexcept
{
  // Translate the user-facing parameters + Mode into per-stage configuration.
  StageConfig cfg;
  cfg.strength = Clamp01(p.strength);
  cfg.guard    = Clamp01(p.artifactGuard);
  cfg.mute     = p.mute;
  cfg.bypass   = p.bypass;

  // Base amounts from the dedicated controls.
  float fbAmt   = Clamp01(p.feedbackAmount);
  float nzAmt   = Clamp01(p.noiseAmount);
  float roomAmt = Clamp01(p.roomAmount);

  // Mode masks / per-mode emphasis.
  bool fbOn = true, nzOn = true, roomOn = true;
  switch (p.mode)
  {
    case Mode::FullClean:    /* all on, defaults */                         break;
    case Mode::FeedbackOnly: nzOn = false; roomOn = false;                  break;
    case Mode::NoiseOnly:    fbOn = false; roomOn = false;                  break;
    case Mode::RoomOnly:     fbOn = false; nzOn = false;                    break;
    case Mode::LiveVocal:    roomAmt *= 0.6f;                               break;
    case Mode::Speech:       nzAmt = std::min(1.0f, nzAmt * 1.3f); roomAmt *= 0.4f; break;
    case Mode::Instrument:   nzAmt *= 0.5f; roomOn = false;                 break;
    default: break;
  }

  cfg.feedbackOn = fbOn;
  cfg.noiseOn    = nzOn;
  cfg.roomOn     = roomOn;

  // Feedback: more amount -> more eager detection and deeper max notch.
  cfg.feedbackSensitivity = fbAmt;
  cfg.feedbackMaxDepthDb   = Lerp(0.0f, -24.0f, fbAmt);

  cfg.noiseAmount = nzAmt;
  cfg.roomAmount  = roomAmt;

  ApplyConfig(cfg);
  mConfig = cfg;
}

void Processor::ApplyConfig(const StageConfig& cfg) noexcept
{
  mFeedbackDetector.SetSensitivity(cfg.feedbackSensitivity);
  mNotchBank.SetMaxDepthDb(cfg.feedbackMaxDepthDb);
  mNoiseEstimator.SetAmount(cfg.noiseOn ? cfg.noiseAmount : 0.0f);
  mDereverb.SetAmount(cfg.roomOn ? cfg.roomAmount : 0.0f);
  mArtifactGuard.SetGuard(cfg.guard);
  mStrength.SetTarget(cfg.strength);
}

void Processor::ProcessBlock(float** inputs, float** outputs, int nFrames, int nChannels) noexcept
{
  const int nCh = std::min(nChannels, mNumChannels);
  const int nF  = std::min(nFrames, mMaxBlockSize);

  // ---- Hard bypass: copy through, publish neutral meters -----------------
  if (mConfig.bypass)
  {
    float peak = 0.0f;
    for (int ch = 0; ch < nChannels; ++ch)
    {
      if (inputs[ch] != outputs[ch])
        std::copy(inputs[ch], inputs[ch] + nFrames, outputs[ch]);
      for (int n = 0; n < nFrames; ++n)
        peak = std::max(peak, std::fabs(static_cast<float>(inputs[ch][n])));
    }
    mDetectedFreqHz.store(0.0f, std::memory_order_relaxed);
    mReductionDb.store(0.0f, std::memory_order_relaxed);
    mActiveNotches.store(0, std::memory_order_relaxed);
    mInputPeak.store(peak,  std::memory_order_relaxed);
    mOutputPeak.store(peak, std::memory_order_relaxed);
    return;
  }

  // ---- Mute: output silence ----------------------------------------------
  if (mConfig.mute)
  {
    for (int ch = 0; ch < nChannels; ++ch)
      std::fill(outputs[ch], outputs[ch] + nFrames, 0.0f);
    mDetectedFreqHz.store(0.0f, std::memory_order_relaxed);
    mReductionDb.store(0.0f, std::memory_order_relaxed);
    mInputPeak.store(0.0f,  std::memory_order_relaxed);
    mOutputPeak.store(0.0f, std::memory_order_relaxed);
    return;
  }

  // ---- 1. Build mono mix for analysis & capture dry copy -----------------
  const float chNorm = (nCh > 0) ? 1.0f / static_cast<float>(nCh) : 1.0f;
  for (int n = 0; n < nF; ++n)
  {
    float mix = 0.0f;
    for (int ch = 0; ch < nCh; ++ch)
    {
      const float s = inputs[ch][n];
      mDry[ch][n] = s;          // dry reference for the artifact guard
      outputs[ch][n] = s;       // process in place on the output buffers
      mix += s;
    }
    mMonoMix[n] = mix * chNorm;
  }

  // ---- 2. ML features (inert in MVP) -------------------------------------
  // Computed and fed to the stub so the integration path is exercised. The
  // neutral confidences returned do not currently scale any DSP amount.
  const FeatureFrame& features = mFeatureExtractor.Process(mMonoMix.data(), nF);
  const Confidences   conf     = mTinyModel.Infer(features);
  (void)conf;

  // ---- 3. Feedback detection + adaptive notching -------------------------
  float reductionDb = 0.0f;
  if (mConfig.feedbackOn)
  {
    const int nCand = mFeedbackDetector.Process(mMonoMix.data(), nF);
    mNotchBank.Update(mFeedbackDetector.Candidates().data(), nCand);

    // Sample-outer loop so the bank's per-frame smoothing (Tick) runs once per
    // sample frame while the per-channel filters share that state.
    for (int n = 0; n < nF; ++n)
    {
      mNotchBank.Tick();
      for (int ch = 0; ch < nCh; ++ch)
        outputs[ch][n] = mNotchBank.Process(ch, outputs[ch][n]);
    }

    // Publish the strongest detected candidate for the UI readout.
    const auto& cands = mFeedbackDetector.Candidates();
    if (nCand > 0)
      mDetectedFreqHz.store(cands[0].frequencyHz, std::memory_order_relaxed);
    else
      mDetectedFreqHz.store(0.0f, std::memory_order_relaxed);

    mActiveNotches.store(mNotchBank.NumActiveNotches(), std::memory_order_relaxed);
  }
  else
  {
    mDetectedFreqHz.store(0.0f, std::memory_order_relaxed);
    mActiveNotches.store(0, std::memory_order_relaxed);
  }

  // ---- 4. Noise reduction ------------------------------------------------
  if (mConfig.noiseOn)
  {
    mNoiseEstimator.ProcessBlock(outputs, nF, nCh);
    reductionDb = std::min(reductionDb, mNoiseEstimator.LastReductionDb());
  }

  // ---- 5. De-reverb tail suppression -------------------------------------
  if (mConfig.roomOn)
  {
    mDereverb.ProcessBlock(outputs, nF, nCh);
    reductionDb = std::min(reductionDb, mDereverb.LastReductionDb());
  }

  // ---- 6. Artifact guard (limits net over-attenuation) -------------------
  mArtifactGuard.ProcessBlock(mDryPtrs.data(), outputs, nF, nCh);

  // ---- 7. Global Strength dry/wet blend + peak metering ------------------
  float inPeak = 0.0f;
  float outPeak = 0.0f;
  for (int n = 0; n < nF; ++n)
  {
    const float s = mStrength.Process();
    for (int ch = 0; ch < nCh; ++ch)
    {
      inPeak = std::max(inPeak, std::fabs(mDry[ch][n]));
      const float y = Lerp(mDry[ch][n], outputs[ch][n], s);
      outputs[ch][n] = y;
      outPeak = std::max(outPeak, std::fabs(y));
    }
  }

  // ---- 8. Publish metering ----------------------------------------------
  mReductionDb.store(reductionDb, std::memory_order_relaxed);
  mInputPeak.store(inPeak,  std::memory_order_relaxed);
  mOutputPeak.store(outPeak, std::memory_order_relaxed);
}

} // namespace odf
