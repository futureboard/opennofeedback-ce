// ============================================================================
//  ModelWeightsStub.h   (ML placeholder)
//
//  Placeholder for embedded, quantised model weights. In MVP 3 the training
//  pipeline (see /training) will export a small GRU/MLP as a C array that gets
//  baked into the plugin binary here, so no external runtime or file I/O is
//  needed at audio time.
//
//  For now this only describes the intended layout and provides empty/neutral
//  data so the rest of the code can compile and link against a stable symbol.
// ============================================================================
#pragma once

#include <cstdint>

namespace odf
{
namespace model
{

// Bumped whenever the feature layout or network topology changes.
static constexpr int   kWeightsVersion = 0;     // 0 == stub / no model
static constexpr int   kInputSize      = 12;    // must match FeatureFrame layout
static constexpr int   kHiddenSize     = 0;     // 0 == no hidden layer yet
static constexpr int   kNumOutputs     = 4;     // feedback, noise, room, direct

// Empty weight blob. A real export replaces this with quantised int8/float data
// plus the corresponding scales/zero-points.
static constexpr int     kNumWeights = 0;
static constexpr float   kWeights[1] = { 0.0f }; // size-1 to remain valid C++

} // namespace model
} // namespace odf
