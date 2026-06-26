// ============================================================================
//  FeedbackDetector.cpp
// ============================================================================
#include "FeedbackDetector.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void FeedbackDetector::Prepare(double sampleRate, int /*maxBlockSize*/)
{
  mSampleRate = sampleRate;

  // Log-spaced band centres. Feedback typically lives between ~150 Hz and the
  // upper-mid region where small rooms / PA systems ring.
  const float fLo = 150.0f;
  const float fHi = std::min(12000.0f, static_cast<float>(sampleRate) * 0.45f);
  const float ratio = std::pow(fHi / fLo, 1.0f / static_cast<float>(kNumBands - 1));

  // Analysis Q: high enough to isolate narrow tones, low enough to stay cheap
  // and stable. ~ matches the geometric band spacing.
  const float analysisQ = 6.0f;

  float f = fLo;
  for (int i = 0; i < kNumBands; ++i)
  {
    mBands[i].centreHz = f;
    mBands[i].bandpass.SetBandpass(f, analysisQ, sampleRate);
    f *= ratio;
  }

  // Envelope coefficients: "fast" tracks the current ring, "slow" is the
  // longer-term reference used to spot growth.
  mFastCoeff = OnePoleCoeff(15.0f,  sampleRate);
  mSlowCoeff = OnePoleCoeff(400.0f, sampleRate);

  // Persistence rises slowly (needs sustained energy) and falls quickly.
  mPersistRise = OnePoleCoeff(120.0f, sampleRate);
  mPersistFall = OnePoleCoeff(60.0f,  sampleRate);

  Reset();
}

void FeedbackDetector::Reset()
{
  for (auto& b : mBands)
  {
    b.bandpass.Reset();
    b.fastEnergy = 0.0f;
    b.slowEnergy = 0.0f;
    b.persistence = 0.0f;
  }
  mCandidates.fill(FeedbackCandidate{});
  mNumCandidates = 0;
}

int FeedbackDetector::Process(const float* monoMix, int nFrames) noexcept
{
  if (nFrames <= 0)
  {
    mNumCandidates = 0;
    return 0;
  }

  const float invN = 1.0f / static_cast<float>(nFrames);

  // ---- 1. Per-band power for this block ----------------------------------
  // Scratch power values live on the stack (fixed size, no allocation).
  std::array<float, kNumBands> blockPower{};
  for (int i = 0; i < kNumBands; ++i)
  {
    Biquad& bp = mBands[i].bandpass;
    float acc = 0.0f;
    for (int n = 0; n < nFrames; ++n)
    {
      const float y = bp.Process(monoMix[n]);
      acc += y * y;
    }
    blockPower[i] = acc * invN; // mean power in band
  }

  // ---- 2. Update envelopes / persistence ---------------------------------
  float meanPower = 0.0f;
  for (int i = 0; i < kNumBands; ++i)
  {
    Band& b = mBands[i];
    b.fastEnergy = blockPower[i] + (b.fastEnergy - blockPower[i]) * mFastCoeff;
    b.slowEnergy = blockPower[i] + (b.slowEnergy - blockPower[i]) * mSlowCoeff;
    b.fastEnergy = FlushDenorm(b.fastEnergy);
    b.slowEnergy = FlushDenorm(b.slowEnergy);
    meanPower += b.fastEnergy;
  }
  meanPower *= (1.0f / static_cast<float>(kNumBands));
  const float floor = std::max(meanPower, 1.0e-9f);

  // ---- 3. Score each band -------------------------------------------------
  // Lower threshold when the user asks for more aggressive feedback removal.
  const float confThreshold = Lerp(0.55f, 0.2f, mSensitivity);

  for (int i = 0; i < kNumBands; ++i)
  {
    Band& b = mBands[i];

    // (a) Narrowness: how much this band stands out from its neighbours.
    const int lo = std::max(0, i - 2);
    const int hi = std::min(kNumBands - 1, i + 2);
    float neighbour = 0.0f;
    int   ncount = 0;
    for (int j = lo; j <= hi; ++j)
    {
      if (j == i) continue;
      neighbour += mBands[j].fastEnergy;
      ++ncount;
    }
    neighbour = (ncount > 0) ? neighbour / static_cast<float>(ncount) : floor;
    const float prominence = b.fastEnergy / std::max(neighbour, 1.0e-9f);
    // Map prominence (1 == flat, >>1 == sharp peak) into 0..1.
    const float narrowness = Clamp01((prominence - 2.0f) / 8.0f);

    // (b) Growth: ringing energy rising above its own slow reference.
    const float growth = Clamp01((b.fastEnergy / std::max(b.slowEnergy, 1.0e-9f) - 1.0f) * 0.5f);

    // (c) Above the overall floor (ignore quiet bands).
    const float aboveFloor = Clamp01((b.fastEnergy / floor - 1.5f) * 0.5f);

    // Instantaneous tonal cue, gated by being meaningfully above the floor.
    const float instant = narrowness * aboveFloor;

    // (d) Persistence: a peak that sticks around is far more likely feedback
    // than a transient musical note. Integrate the instantaneous cue.
    const float target = instant;
    const float coeff = (target > b.persistence) ? mPersistRise : mPersistFall;
    b.persistence = target + (b.persistence - target) * coeff;

    // Combine: must be persistent AND narrow; growth boosts confidence.
    b.persistence = FlushDenorm(b.persistence);
  }

  // ---- 4. Build candidate list -------------------------------------------
  // Confidence is computed from the persisted state so it is stable frame to
  // frame. We then keep the strongest few above threshold.
  std::array<float, kNumBands> conf{};
  for (int i = 0; i < kNumBands; ++i)
  {
    Band& b = mBands[i];
    const float growth = Clamp01((b.fastEnergy / std::max(b.slowEnergy, 1.0e-9f) - 1.0f) * 0.5f);
    conf[i] = Clamp01(b.persistence * (0.7f + 0.3f * growth));
  }

  mNumCandidates = 0;
  // Greedy selection of the highest-confidence bands, skipping immediate
  // neighbours so we don't stack notches on the same peak.
  std::array<bool, kNumBands> used{};
  used.fill(false);
  for (int slot = 0; slot < kMaxCandidates; ++slot)
  {
    int best = -1;
    float bestConf = confThreshold;
    for (int i = 0; i < kNumBands; ++i)
    {
      if (used[i]) continue;
      if (conf[i] > bestConf)
      {
        bestConf = conf[i];
        best = i;
      }
    }
    if (best < 0)
      break;

    FeedbackCandidate& c = mCandidates[mNumCandidates++];
    c.frequencyHz = mBands[best].centreHz;
    c.confidence  = conf[best];
    // Sharper notch for stronger / higher confidence detections.
    c.q = Lerp(8.0f, 24.0f, conf[best]);

    used[best] = true;
    if (best > 0)              used[best - 1] = true;
    if (best < kNumBands - 1)  used[best + 1] = true;
  }

  return mNumCandidates;
}

} // namespace odf
