// ============================================================================
//  FeatureExtractor.cpp   (ML placeholder)
//
//  A deliberately simple, FFT-free feature extractor. It is good enough to
//  exercise the ML integration path; a production model would likely use a
//  short-time spectral representation computed on a strided schedule.
// ============================================================================
#include "FeatureExtractor.h"
#include "../common/RealtimeUtils.h"
#include <cmath>
#include <algorithm>

namespace odf
{

void FeatureExtractor::Prepare(double sampleRate, int /*maxBlockSize*/)
{
  mSampleRate = sampleRate;
  Reset();
}

void FeatureExtractor::Reset()
{
  mFrame = FeatureFrame{};
  mPrevBandEnergy.fill(0.0f);
}

const FeatureFrame& FeatureExtractor::Process(const float* monoMix, int nFrames) noexcept
{
  if (nFrames <= 0)
    return mFrame;

  const float invN = 1.0f / static_cast<float>(nFrames);

  // --- RMS / peak ---------------------------------------------------------
  float sumSq = 0.0f;
  float peak  = 0.0f;
  for (int n = 0; n < nFrames; ++n)
  {
    const float s = monoMix[n];
    sumSq += s * s;
    peak = std::max(peak, std::fabs(s));
  }
  const float rms = std::sqrt(sumSq * invN);
  mFrame.rms = rms;
  mFrame.crestFactor = peak / std::max(rms, 1.0e-6f);

  // --- Coarse band energies via cheap zero-crossing-weighted binning ------
  // Without an FFT we approximate spectral distribution by splitting the block
  // into time segments and measuring local high-frequency content (first
  // differences). This is a stand-in; the exact representation will be revised
  // when the real model is designed.
  std::array<float, FeatureFrame::kNumBands> bands{};
  float weightedFreq = 0.0f;
  float totalEnergy  = 0.0f;
  float prev = (nFrames > 0) ? monoMix[0] : 0.0f;
  for (int n = 0; n < nFrames; ++n)
  {
    const float s = monoMix[n];
    const float diff = s - prev;
    prev = s;
    const int band = std::min(FeatureFrame::kNumBands - 1,
                              (n * FeatureFrame::kNumBands) / std::max(nFrames, 1));
    const float e = s * s;
    bands[band] += e;
    // crude HF proxy: |first difference| relative to |sample|.
    weightedFreq += std::fabs(diff);
    totalEnergy  += std::fabs(s);
  }

  for (int b = 0; b < FeatureFrame::kNumBands; ++b)
  {
    const float e = bands[b] * invN;
    mFrame.bandEnergy[b] = e;
  }

  mFrame.spectralCentroid = Clamp01(weightedFreq / std::max(totalEnergy, 1.0e-6f));

  // --- Spectral flux (positive change vs previous frame) ------------------
  float flux = 0.0f;
  for (int b = 0; b < FeatureFrame::kNumBands; ++b)
  {
    const float d = mFrame.bandEnergy[b] - mPrevBandEnergy[b];
    if (d > 0.0f) flux += d;
    mPrevBandEnergy[b] = mFrame.bandEnergy[b];
  }
  mFrame.spectralFlux = flux;

  return mFrame;
}

} // namespace odf
