// ============================================================================
//  TinyModelStub.cpp   (ML placeholder)
// ============================================================================
#include "TinyModelStub.h"
#include "ModelWeightsStub.h"

namespace odf
{

void TinyModelStub::Prepare(double /*sampleRate*/)
{
  // A real implementation would validate ModelWeightsStub against the expected
  // feature layout and set mModelLoaded accordingly.
  mModelLoaded = (model::kWeightsVersion != 0);
}

void TinyModelStub::Reset()
{
  // No recurrent state in the stub. A GRU model would clear its hidden state.
}

Confidences TinyModelStub::Infer(const FeatureFrame& /*features*/) noexcept
{
  // Neutral output: do not influence the DSP. Multiplying any suppression
  // amount by these values is a no-op for `direct` and adds nothing for the
  // others until a trained model replaces this stub.
  return Confidences{};
}

} // namespace odf
