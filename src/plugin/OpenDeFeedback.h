// ============================================================================
//  OpenDeFeedback.h
//
//  iPlug2 plugin shell. The audio thread converts iPlug's `sample` buffers to
//  float, hands them to the realtime-safe odf::Processor, and converts back.
//  Metering is deferred to the UI thread via IPeakSender + processor atomics.
// ============================================================================
#pragma once

#include "IPlug_include_in_plug_hdr.h"

#include <array>
#include <vector>

#include "../dsp/Processor.h"

const int kNumPresets = 1;

// User-facing parameters. Order is the host-visible automation order.
enum EParams
{
  kStrength = 0,
  kFeedbackAmount,
  kNoiseAmount,
  kRoomAmount,
  kArtifactGuard,
  kMode,
  kMute,
  kBypass,
  kNumParams
};

// Control tags for UI elements updated from the editor's idle callback.
enum ECtrlTags
{
  kCtrlTagMeters = 0,
  kCtrlTagReadout,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class OpenDeFeedback final : public Plugin
{
public:
  OpenDeFeedback(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
#endif

private:
#if IPLUG_EDITOR
  // Defined in OpenDeFeedback_UI.cpp — keeps the heavy layout code separate.
  void BuildUI(IGraphics* pGraphics);
#endif

#if IPLUG_DSP
  // Build a parameter snapshot from the current param values (message thread or
  // start of block) and push it to the realtime processor.
  void PushParameters();

  odf::Processor mProcessor;

  // Preallocated float scratch (iPlug `sample` may be double).
  std::vector<float>                       mFloatStorage; // contiguous backing
  std::array<float*, 2>                    mInPtrs {nullptr, nullptr};
  std::array<float*, 2>                    mOutPtrs {nullptr, nullptr};
  int mAllocatedBlock = 0;
#endif
};
