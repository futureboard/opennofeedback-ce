// ============================================================================
//  Biquad.cpp
//
//  Biquad is implemented entirely inline in Biquad.h (the methods are small and
//  benefit from inlining in the audio loop). This translation unit exists so
//  the class has a stable compilation unit in the build and a natural home for
//  any future non-inline helpers.
// ============================================================================
#include "Biquad.h"
