// ============================================================================
//  OpenDeFeedback_UI.cpp
//
//  GUI layout for OpenDeFeedback, kept separate from the audio/host plumbing.
//  Minimal dark live-console style, flat vector controls (NanoVG via IGraphics).
//
//      +-------------------------------------------------------------+
//      | OpenDeFeedback                                              |
//      |  live cleanup                                               |
//      +-----------+---------------------------+---------------------+
//      |  MODE     |                           |  Feedback  [----]   |
//      |  (o) Full |        ( STRENGTH )       |  Noise     [----]   |
//      |  ( ) Fbk  |          large knob       |  Room      [----]   |
//      |  ...      |                           |  Guard     [----]   |
//      +-----------+---------------------------+---------------------+
//      | [ In  ==== ] [ Out ==== ]  Detected:.. Reduction:..  Mute Byp|
//      +-------------------------------------------------------------+
// ============================================================================
#include "OpenDeFeedback.h"
#include "OpenDeFeedback_Controls.h"

#if IPLUG_EDITOR

using namespace odfui;

void OpenDeFeedback::BuildUI(IGraphics* pGraphics)
{
  pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
  pGraphics->AttachPanelBackground(kColorBg);
  pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

  const IVStyle style     = ODFStyle();
  const IVStyle warnStyle = ODFWarnStyle();

  const IRECT bounds = pGraphics->GetBounds().GetPadded(-10.f);

  // ---- Header ------------------------------------------------------------
  const IRECT header = bounds.GetFromTop(40.f);
  pGraphics->AttachControl(new ODFTitleControl(header, "OpenDeFeedback", "realtime live cleanup"));

  // ---- Bottom bar (meters / readout / buttons) ---------------------------
  const IRECT bottom = bounds.GetFromBottom(64.f);
  const IRECT mid    = bounds.GetReducedFromTop(46.f).GetReducedFromBottom(70.f);

  // ---- Centre : Strength -------------------------------------------------
  const IRECT centre = mid.GetMidHPadded(105.f);
  pGraphics->AttachControl(new IVKnobControl(centre.GetCentredInside(170.f, 180.f),
                                             kStrength, "Strength", style));

  // ---- Left : Mode selector ----------------------------------------------
  const IRECT left = mid.GetFromLeft(150.f);
  pGraphics->AttachControl(new IVRadioButtonControl(left, kMode,
    { "Full Clean", "Feedback Only", "Noise Only", "Room Only",
      "Live Vocal", "Speech", "Instrument" },
    "Mode", style, EVShape::Ellipse, EDirection::Vertical, 8.f));

  // ---- Right : amount sliders --------------------------------------------
  const IRECT right = mid.GetFromRight(170.f);
  const float rowH = right.H() / 4.f;
  pGraphics->AttachControl(new IVSliderControl(right.GetGridCell(0, 4, 1).GetMidVPadded(rowH * 0.5f - 6.f),
                                               kFeedbackAmount, "Feedback", style, false, EDirection::Horizontal));
  pGraphics->AttachControl(new IVSliderControl(right.GetGridCell(1, 4, 1).GetMidVPadded(rowH * 0.5f - 6.f),
                                               kNoiseAmount, "Noise", style, false, EDirection::Horizontal));
  pGraphics->AttachControl(new IVSliderControl(right.GetGridCell(2, 4, 1).GetMidVPadded(rowH * 0.5f - 6.f),
                                               kRoomAmount, "Room", style, false, EDirection::Horizontal));
  pGraphics->AttachControl(new IVSliderControl(right.GetGridCell(3, 4, 1).GetMidVPadded(rowH * 0.5f - 6.f),
                                               kArtifactGuard, "Guard", style, false, EDirection::Horizontal));

  // ---- Bottom bar contents -----------------------------------------------
  const IRECT meters  = bottom.GetFromLeft(190.f);
  const IRECT inMeter  = meters.GetFromTop(meters.H() * 0.5f).GetVPadded(-2.f);
  const IRECT outMeter = meters.GetFromBottom(meters.H() * 0.5f).GetVPadded(-2.f);

  pGraphics->AttachControl(new IVMeterControl<2>(inMeter, "In", style, EDirection::Horizontal),
                           kCtrlTagInputMeter);
  pGraphics->AttachControl(new IVMeterControl<2>(outMeter, "Out", style, EDirection::Horizontal),
                           kCtrlTagOutputMeter);

  // Readout + buttons share the remaining width.
  const IRECT rightBottom = bottom.GetReducedFromLeft(200.f);
  const IRECT buttons  = rightBottom.GetFromRight(150.f);
  const IRECT readout  = rightBottom.GetReducedFromRight(160.f).GetMidVPadded(16.f);

  pGraphics->AttachControl(new ODFReadoutControl(readout,
    "Detected: --   Reduction: 0.0 dB   Notches: 0   Latency: 0"),
    kCtrlTagReadout);

  pGraphics->AttachControl(new IVToggleControl(buttons.GetFromLeft(70.f).GetMidVPadded(16.f),
                                               kMute, "Mute", warnStyle, "Mute", "MUTE"));
  pGraphics->AttachControl(new IVToggleControl(buttons.GetFromRight(70.f).GetMidVPadded(16.f),
                                               kBypass, "Bypass", style, "Bypass", "BYP"));
}

#endif // IPLUG_EDITOR
