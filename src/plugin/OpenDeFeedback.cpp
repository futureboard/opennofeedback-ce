// ============================================================================
//  OpenDeFeedback.cpp
// ============================================================================
#include "OpenDeFeedback.h"
#include "IPlug_include_in_plug_src.h"
#include "OpenDeFeedback_Controls.h"   // themed custom controls + meters/readout

#include <algorithm>
#include <cmath>

OpenDeFeedback::OpenDeFeedback(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // ---- Parameters --------------------------------------------------------
  // All "amount" controls are exposed as 0..100% and mapped to 0..1 for the DSP.
  GetParam(kStrength)->InitPercentage("Strength", 100.0);
  GetParam(kFeedbackAmount)->InitPercentage("Feedback Amount", 100.0);
  GetParam(kNoiseAmount)->InitPercentage("Noise Amount", 35.0);
  GetParam(kRoomAmount)->InitPercentage("Room Amount", 25.0);
  GetParam(kArtifactGuard)->InitPercentage("Artifact Guard", 75.0);

  GetParam(kMode)->InitEnum("Mode", 0,
    { "Full Clean", "Feedback Only", "Noise Only", "Room Only",
      "Live Vocal", "Speech", "Instrument" });

  GetParam(kMute)->InitBool("Mute", false);
  GetParam(kBypass)->InitBool("Bypass", false);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    BuildUI(pGraphics);
  };
#endif
}

#if IPLUG_DSP
void OpenDeFeedback::OnReset()
{
  const double sr = GetSampleRate();
  const int blockSize = std::max(GetBlockSize(), 1);
  const int nChans = std::min(std::max(NOutChansConnected(), 1), 2);

  // (Re)allocate float scratch on the prepare path (never in ProcessBlock).
  if (blockSize != mAllocatedBlock)
  {
    mFloatStorage.assign(static_cast<size_t>(blockSize) * 4u, 0.0f); // 2 in + 2 out
    mInPtrs[0]  = mFloatStorage.data() + 0 * blockSize;
    mInPtrs[1]  = mFloatStorage.data() + 1 * blockSize;
    mOutPtrs[0] = mFloatStorage.data() + 2 * blockSize;
    mOutPtrs[1] = mFloatStorage.data() + 3 * blockSize;
    mAllocatedBlock = blockSize;
  }

  mProcessor.Prepare(sr, blockSize, nChans);
  PushParameters();
}

void OpenDeFeedback::OnParamChange(int /*paramIdx*/)
{
  // Build a fresh snapshot and hand it to the realtime processor. Called on the
  // message thread; SetParameters only copies POD, so this is safe.
  PushParameters();
}

void OpenDeFeedback::PushParameters()
{
  odf::Parameters p;
  p.strength      = static_cast<float>(GetParam(kStrength)->Value() / 100.0);
  p.feedbackAmount = static_cast<float>(GetParam(kFeedbackAmount)->Value() / 100.0);
  p.noiseAmount   = static_cast<float>(GetParam(kNoiseAmount)->Value() / 100.0);
  p.roomAmount    = static_cast<float>(GetParam(kRoomAmount)->Value() / 100.0);
  p.artifactGuard = static_cast<float>(GetParam(kArtifactGuard)->Value() / 100.0);
  p.mode          = static_cast<odf::Mode>(GetParam(kMode)->Int());
  p.mute          = GetParam(kMute)->Bool();
  p.bypass        = GetParam(kBypass)->Bool();
  mProcessor.SetParameters(p);
}

void OpenDeFeedback::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = std::min(NOutChansConnected(), 2);
  const int nF = std::min(nFrames, mAllocatedBlock);

  // ---- sample (double) -> float ------------------------------------------
  for (int ch = 0; ch < nChans; ++ch)
    for (int n = 0; n < nF; ++n)
      mInPtrs[ch][n] = static_cast<float>(inputs[ch][n]);

  // ---- Realtime DSP ------------------------------------------------------
  mProcessor.ProcessBlock(mInPtrs.data(), mOutPtrs.data(), nF, nChans);

  // ---- float -> sample (double) ------------------------------------------
  for (int ch = 0; ch < nChans; ++ch)
    for (int n = 0; n < nF; ++n)
      outputs[ch][n] = static_cast<sample>(mOutPtrs[ch][n]);
}

void OpenDeFeedback::OnIdle()
{
#if IPLUG_EDITOR
  auto* pGraphics = GetUI();
  if (!pGraphics)
    return;

  // Map a linear peak to a 0..1 meter position on a -60..0 dB scale.
  auto peakToNorm = [](float peak) -> float {
    if (peak <= 1.0e-5f) return 0.0f;
    const float db = 20.0f * std::log10(peak);
    const float n = (db + 60.0f) / 60.0f;
    return n < 0.f ? 0.f : (n > 1.f ? 1.f : n);
  };

  const float inNorm  = peakToNorm(mProcessor.GetInputPeak());
  const float outNorm = peakToNorm(mProcessor.GetOutputPeak());
  const float redDb   = mProcessor.GetReductionDb();           // <= 0
  const float redNorm = std::min(1.0f, (-redDb) / 24.0f);      // 24 dB == full

  if (auto* pMeters = pGraphics->GetControlWithTag(kCtrlTagMeters))
    pMeters->As<odf::MetersControl>()->SetLevels(inNorm, outNorm, redNorm);

  if (auto* pReadout = pGraphics->GetControlWithTag(kCtrlTagReadout))
  {
    const float hz = mProcessor.GetDetectedFreqHz();
    WDL_String str;
    if (hz > 1.0f)
      str.SetFormatted(128, "Detected: %.2f kHz    Reduction: %.1f dB", hz / 1000.0f, redDb);
    else
      str.SetFormatted(128, "Detected: --    Reduction: %.1f dB", redDb);
    pReadout->As<odf::ReadoutControl>()->SetText(str.Get());
  }
#endif
}
#endif // IPLUG_DSP
