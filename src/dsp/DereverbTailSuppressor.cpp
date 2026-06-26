// ============================================================================
//  DereverbTailSuppressor.cpp
// ============================================================================
#include "DereverbTailSuppressor.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void DereverbTailSuppressor::Prepare(double sampleRate, int numChannels)
{
  mSampleRate = sampleRate;
  mNumChannels = std::min(numChannels, kMaxChannels);

  for (auto& cs : mChannels)
  {
    // Fast envelope snaps to onsets; slow envelope lags and represents the tail.
    cs.fast.SetTimes(2.0f,   60.0f,  sampleRate);
    cs.slow.SetTimes(80.0f,  600.0f, sampleRate);
    cs.gain.SetTimeConstant(50.0f, sampleRate); // gentle gain glide
  }

  Reset();
}

void DereverbTailSuppressor::Reset()
{
  for (auto& cs : mChannels)
  {
    cs.fast.Reset();
    cs.slow.Reset();
    cs.gain.Reset(1.0f);
  }
  mLastReductionDb = 0.0f;
}

void DereverbTailSuppressor::SetAmount(float amount) noexcept
{
  mAmount = Clamp01(amount);
  mMaxReductionDb = Lerp(0.0f, -10.0f, mAmount);
}

void DereverbTailSuppressor::ProcessBlock(float** io, int nFrames, int nChannels) noexcept
{
  if (mAmount <= 1.0e-4f)
  {
    mLastReductionDb = 0.0f;
    return;
  }

  const int nCh = std::min(nChannels, mNumChannels);
  float worstReductionDb = 0.0f;

  for (int ch = 0; ch < nCh; ++ch)
  {
    ChannelState& cs = mChannels[ch];
    float* x = io[ch];

    for (int n = 0; n < nFrames; ++n)
    {
      const float in = x[n];
      const float ef = cs.fast.Process(in);
      const float es = cs.slow.Process(in);

      // Ratio of direct(fast) to tail(slow) energy.
      //   r >> 1 : onset / transient -> leave alone
      //   r <  1 : decaying tail dominates -> attenuate
      const float r = ef / std::max(es, 1.0e-6f);

      // tailiness: 0 when transient-rich, 1 deep in the tail.
      const float tailiness = Clamp01(1.0f - r);

      const float gainDb = mMaxReductionDb * tailiness;
      cs.gain.SetTarget(DbToGain(gainDb));
      const float g = cs.gain.Process();

      x[n] = in * g;

      if (gainDb < worstReductionDb)
        worstReductionDb = gainDb;
    }
  }

  mLastReductionDb = worstReductionDb;
}

} // namespace odf
