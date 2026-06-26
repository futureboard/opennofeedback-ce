// ============================================================================
//  NoiseEstimator.cpp
// ============================================================================
#include "NoiseEstimator.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void NoiseEstimator::Prepare(double sampleRate, int numChannels)
{
  mSampleRate = sampleRate;
  mNumChannels = std::min(numChannels, kMaxChannels);

  const float f1 = 400.0f;   // low / mid split
  const float f2 = 3000.0f;  // mid / high split
  const float q  = 0.7071f;  // Butterworth -> allpass-sum crossover

  for (auto& ch : mChannels)
  {
    ch.lp1.SetLowpass (f1, q, sampleRate);
    ch.hp1.SetHighpass(f1, q, sampleRate);
    ch.lp2.SetLowpass (f2, q, sampleRate);
    ch.hp2.SetHighpass(f2, q, sampleRate);
    for (auto& b : ch.bands)
      b.gain.SetTimeConstant(40.0f, sampleRate); // smooth gain -> no musical noise
  }

  // Envelope and floor tracker time constants.
  mEnvCoeff  = OnePoleCoeff(10.0f,  sampleRate);
  mFloorRise = OnePoleCoeff(2000.0f, sampleRate); // floor creeps up slowly
  mFloorFall = OnePoleCoeff(50.0f,   sampleRate); // but drops to quiet quickly

  Reset();
}

void NoiseEstimator::Reset()
{
  for (auto& ch : mChannels)
  {
    ch.lp1.Reset(); ch.hp1.Reset(); ch.lp2.Reset(); ch.hp2.Reset();
    for (auto& b : ch.bands)
    {
      b.gain.Reset(1.0f);
      b.envelope = 0.0f;
      b.noiseFloor = 1.0e-4f;
    }
  }
  mLastReductionDb = 0.0f;
}

void NoiseEstimator::SetAmount(float amount) noexcept
{
  mAmount = Clamp01(amount);
  // Scale the maximum band reduction with the amount control. Kept gentle.
  mMaxReductionDb = Lerp(0.0f, -18.0f, mAmount);
}

void NoiseEstimator::ProcessBlock(float** io, int nFrames, int nChannels) noexcept
{
  if (mAmount <= 1.0e-4f)
  {
    mLastReductionDb = 0.0f;
    return;
  }

  const int nCh = std::min(nChannels, mNumChannels);

  // Downward-expander parameters (above the tracked floor we want unity gain,
  // below it we attenuate). Margin keeps real programme material safe.
  const float thresholdMarginDb = 8.0f; // signals within 8 dB of floor start to expand
  const float ratio = Lerp(1.2f, 2.5f, mAmount);

  float worstReductionDb = 0.0f;

  for (int ch = 0; ch < nCh; ++ch)
  {
    ChannelState& cs = mChannels[ch];
    float* x = io[ch];

    for (int n = 0; n < nFrames; ++n)
    {
      const float in = x[n];

      // ---- band split (allpass-sum complementary crossovers) -------------
      const float low  = cs.lp1.Process(in);
      const float high0 = cs.hp1.Process(in);
      const float mid  = cs.lp2.Process(high0);
      const float high = cs.hp2.Process(high0);

      float bandSig[kNumBands] = { low, mid, high };
      float out = 0.0f;

      for (int b = 0; b < kNumBands; ++b)
      {
        BandState& bs = cs.bands[b];
        const float mag = std::fabs(bandSig[b]);

        // Envelope follower.
        bs.envelope = mag + (bs.envelope - mag) * mEnvCoeff;
        bs.envelope = FlushDenorm(bs.envelope);

        // Noise-floor tracker: quick down, slow up. Settles on the quiet level.
        if (bs.envelope < bs.noiseFloor)
          bs.noiseFloor = bs.envelope + (bs.noiseFloor - bs.envelope) * mFloorFall;
        else
          bs.noiseFloor = bs.envelope + (bs.noiseFloor - bs.envelope) * mFloorRise;
        bs.noiseFloor = std::max(FlushDenorm(bs.noiseFloor), 1.0e-6f);

        // Soft downward expander around (floor + margin).
        const float envDb   = GainToDb(std::max(bs.envelope, kMinGain));
        const float thrDb    = GainToDb(bs.noiseFloor) + thresholdMarginDb;
        float gainDb = 0.0f;
        if (envDb < thrDb)
          gainDb = std::max((envDb - thrDb) * (ratio - 1.0f), mMaxReductionDb);

        bs.gain.SetTarget(DbToGain(gainDb));
        const float g = bs.gain.Process();

        out += bandSig[b] * g;

        if (gainDb < worstReductionDb)
          worstReductionDb = gainDb;
      }

      x[n] = out;
    }
  }

  mLastReductionDb = worstReductionDb;
}

} // namespace odf
