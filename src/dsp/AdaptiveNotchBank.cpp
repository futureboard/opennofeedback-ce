// ============================================================================
//  AdaptiveNotchBank.cpp
// ============================================================================
#include "AdaptiveNotchBank.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void AdaptiveNotchBank::Prepare(double sampleRate, int numChannels)
{
  mSampleRate = sampleRate;
  mNumChannels = std::min(numChannels, kMaxChannels);

  for (auto& n : mNotches)
  {
    // Fairly quick frequency glide so a notch can track a moving resonance,
    // slower depth glide so engaging/releasing a notch is inaudible.
    n.freq.SetTimeConstant(30.0f, sampleRate);
    n.depth.SetTimeConstant(60.0f, sampleRate);
  }

  Reset();
}

void AdaptiveNotchBank::Reset()
{
  for (auto& n : mNotches)
  {
    n.active = false;
    n.freq.Reset(1000.0f);
    n.depth.Reset(0.0f);
    n.q = 12.0f;
    n.lastDesignedFreq = 0.0f;
    n.lastDesignedDepth = 0.0f;
    for (auto& f : n.filter)
      f.SetIdentity();
  }
}

void AdaptiveNotchBank::Update(const FeedbackCandidate* candidates, int numCandidates) noexcept
{
  // --- 1. Mark all notches as "not matched this block" ---------------------
  std::array<bool, kMaxNotches> matched{};
  matched.fill(false);

  // --- 2. For each candidate, find an existing notch on a nearby frequency,
  //        otherwise grab a free slot. ------------------------------------
  for (int c = 0; c < numCandidates; ++c)
  {
    const FeedbackCandidate& cand = candidates[c];

    int slot = -1;
    // Try to reuse a notch already near this frequency (track a moving tone).
    float bestRatio = 1.06f; // within ~1 semitone
    for (int i = 0; i < kMaxNotches; ++i)
    {
      if (!mNotches[i].active || matched[i]) continue;
      const float r = std::max(cand.frequencyHz, mNotches[i].freq.GetTarget()) /
                      std::min(cand.frequencyHz, mNotches[i].freq.GetTarget());
      if (r < bestRatio)
      {
        bestRatio = r;
        slot = i;
      }
    }

    // Otherwise claim an inactive slot.
    if (slot < 0)
    {
      for (int i = 0; i < kMaxNotches; ++i)
      {
        if (!mNotches[i].active && !matched[i])
        {
          slot = i;
          mNotches[i].freq.Reset(cand.frequencyHz); // start on-frequency
          break;
        }
      }
    }

    if (slot < 0)
      continue; // bank full; ignore weakest extra candidates

    Notch& n = mNotches[slot];
    n.active = true;
    matched[slot] = true;
    n.q = cand.q;
    n.freq.SetTarget(cand.frequencyHz);
    // Depth scales with confidence, clamped to the user/guard ceiling.
    const float depthDb = mMaxDepthDb * Clamp01(cand.confidence);
    n.depth.SetTarget(depthDb);
  }

  // --- 3. Release notches that no candidate matched ------------------------
  for (int i = 0; i < kMaxNotches; ++i)
  {
    if (mNotches[i].active && !matched[i])
    {
      mNotches[i].depth.SetTarget(0.0f); // smoothly back out
      // Deactivate only once the depth has actually returned to ~0 (handled in
      // Tick()), so the release is audible-free.
    }
  }
}

void AdaptiveNotchBank::Tick() noexcept
{
  for (auto& n : mNotches)
  {
    n.freq.Process();
    n.depth.Process();

    // If a released notch has fully recovered, free the slot.
    if (n.active && n.depth.GetTarget() == 0.0f && n.depth.GetCurrent() > -0.05f)
    {
      n.active = false;
      n.depth.Reset(0.0f);
      for (auto& f : n.filter)
        f.SetIdentity();
      continue;
    }

    RedesignIfNeeded(n);
  }
}

void AdaptiveNotchBank::RedesignIfNeeded(Notch& n) noexcept
{
  const float f = n.freq.GetCurrent();
  const float d = n.depth.GetCurrent();

  // Only redesign when the smoothed values have moved enough to matter. This
  // keeps the (transcendental) coefficient math off the per-sample hot path.
  const float freqRatio = (n.lastDesignedFreq > 0.0f)
                          ? std::fabs(f - n.lastDesignedFreq) / n.lastDesignedFreq
                          : 1.0f;
  const float depthDelta = std::fabs(d - n.lastDesignedDepth);

  if (freqRatio < 0.002f && depthDelta < 0.05f)
    return;

  for (int ch = 0; ch < mNumChannels; ++ch)
    n.filter[ch].SetNotch(f, n.q, d, mSampleRate);

  n.lastDesignedFreq = f;
  n.lastDesignedDepth = d;
}

int AdaptiveNotchBank::NumActiveNotches() const noexcept
{
  int count = 0;
  for (const auto& n : mNotches)
    if (n.active) ++count;
  return count;
}

} // namespace odf
