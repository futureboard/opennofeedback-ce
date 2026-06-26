// ============================================================================
//  Parameters.h
//
//  Plain-old-data parameter snapshot shared between the plugin (control side)
//  and the realtime DSP Processor.
//
//  Realtime-safety note:
//    This struct contains only trivially-copyable scalar members. The plugin
//    builds one of these on the message thread and hands a *copy* to the DSP
//    via Processor::SetParameters(). The DSP never reads plugin parameter
//    objects directly, so there is no shared mutable state and no locking
//    required in the audio callback.
// ============================================================================
#pragma once

#include <cstdint>

namespace odf
{

/** Processing modes. Keep the ordering in sync with the plugin parameter enum
 *  (kMode) and the GUI mode selector labels. */
enum class Mode : int
{
  FullClean = 0,
  FeedbackOnly,
  NoiseOnly,
  RoomOnly,
  LiveVocal,
  Speech,
  Instrument,
  NumModes
};

/** Human readable labels for each Mode. Index with the Mode value. */
static constexpr const char* kModeLabels[] = {
  "Full Clean",
  "Feedback Only",
  "Noise Only",
  "Room Only",
  "Live Vocal",
  "Speech",
  "Instrument"
};

/** A flat, copyable snapshot of every user-facing parameter.
 *  All gains are normalised 0..1 "amount" controls; the DSP decides how to map
 *  them to actual attenuation. */
struct Parameters
{
  float strength      = 1.0f;   // global wet/dry & aggressiveness 0..1
  float feedbackAmount = 1.0f;  // feedback suppression amount 0..1
  float noiseAmount   = 0.35f;  // noise reduction amount 0..1
  float roomAmount    = 0.25f;  // de-reverb amount 0..1
  float artifactGuard = 0.75f;  // 0 = allow heavy processing, 1 = very gentle

  Mode  mode          = Mode::FullClean;

  bool  mute          = false;
  bool  bypass        = false;
};

} // namespace odf
