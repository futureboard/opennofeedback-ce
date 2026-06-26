// ============================================================================
//  ArtifactGuard.cpp
// ============================================================================
#include "ArtifactGuard.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void ArtifactGuard::Prepare(double sampleRate, int numChannels)
{
  mSampleRate = sampleRate;
  mNumChannels = std::min(numChannels, kMaxChannels);

  for (auto& cs : mChannels)
  {
    // Reasonably quick envelopes so the guard reacts within a syllable, but not
    // so fast that it tracks individual cycles.
    cs.dryEnv.SetTimes(5.0f, 80.0f, sampleRate);
    cs.wetEnv.SetTimes(5.0f, 80.0f, sampleRate);
    cs.correction.SetTimeConstant(30.0f, sampleRate);
  }

  Reset();
}

void ArtifactGuard::Reset()
{
  for (auto& cs : mChannels)
  {
    cs.dryEnv.Reset();
    cs.wetEnv.Reset();
    cs.correction.Reset(1.0f);
  }
}

void ArtifactGuard::SetGuard(float guard) noexcept
{
  guard = Clamp01(guard);
  // guard 0 -> allow down to -30 dB; guard 1 -> limit to -3 dB.
  const float floorDb = Lerp(-30.0f, -3.0f, guard);
  mMinGain = DbToGain(floorDb);
}

void ArtifactGuard::ProcessBlock(const float* const* dry, float** wet,
                                 int nFrames, int nChannels) noexcept
{
  const int nCh = std::min(nChannels, mNumChannels);

  for (int ch = 0; ch < nCh; ++ch)
  {
    ChannelState& cs = mChannels[ch];
    const float* d = dry[ch];
    float*       w = wet[ch];

    for (int n = 0; n < nFrames; ++n)
    {
      const float de = cs.dryEnv.Process(d[n]);
      const float we = cs.wetEnv.Process(w[n]);

      // Net attenuation applied upstream (wet relative to dry).
      const float applied = we / std::max(de, 1.0e-6f);

      // If we have cut more than the floor allows, compute a make-up gain that
      // brings the net attenuation back up to exactly the floor.
      float makeup = 1.0f;
      if (applied < mMinGain && de > 1.0e-5f)
        makeup = mMinGain / std::max(applied, 1.0e-6f);

      cs.correction.SetTarget(makeup);
      const float g = cs.correction.Process();

      w[n] = w[n] * g;
    }
  }
}

} // namespace odf
