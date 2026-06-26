// ============================================================================
//  OpenDeFeedback_UI.cpp
//
//  GUI layout. Compact, modern, dark, technical, minimal, editor-like, flat,
//  realtime-system focused. No skeuomorphic controls, no hardware imitation.
//  Every visual value comes from the generated static theme (odf::Theme) via
//  the UI primitives / custom controls — there are no raw color literals here.
//
//      +------------------------------------------------------------+
//      | OpenDeFeedback                       Latency: 0 samples     |
//      +------------+----------------------------+------------------+
//      | MODE       |          STRENGTH          | METERS           |
//      | Full Clean |             100            | IN  ===          |
//      | Live Vocal |       clean intensity      | OUT ===          |
//      | ...        |                            | RED =            |
//      +------------+----------------------------+------------------+
//      | Feedback ====  Noise ===  Room ==  Guard ====              |
//      +------------------------------------------------------------+
//      | [Mute] [Bypass]   Detected: .. Reduction: ..   CPU: -- %    |
//      +------------------------------------------------------------+
// ============================================================================
#include "OpenDeFeedback.h"
#include "OpenDeFeedback_Controls.h"

#if IPLUG_EDITOR

using namespace odf;

void OpenDeFeedback::BuildUI(IGraphics* pGraphics)
{
  pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
  pGraphics->AttachPanelBackground(ToIColor(Theme::Background));
  LoadUIFont(*pGraphics);

  const IVStyle style     = ThemedStyle();
  const IVStyle warnStyle = ThemedDangerStyle();

  const float pad = Theme::SpacingMedium;
  const float gap = Theme::SpacingSmall;
  const IRECT bounds = pGraphics->GetBounds().GetPadded(-pad);

  // Small helper for neutral section titles.
  auto attachSectionTitle = [&](const IRECT& r, const char* text) {
    const IText t(Theme::FontSizeTiny, ToIColor(Theme::TextMuted), kUIFontID,
                  EAlign::Near, EVAlign::Middle);
    pGraphics->AttachControl(new ITextControl(r, text, t));
  };

  // ---- Header ------------------------------------------------------------
  const IRECT header = bounds.GetFromTop(28.f);
  pGraphics->AttachControl(new HeaderControl(header.GetFromLeft(280.f),
                                             "OpenDeFeedback", "realtime cleanup"));
  pGraphics->AttachControl(new StatusPillControl(header.GetFromRight(140.f).GetMidVPadded(9.f),
                                                 "Latency: 0 samples"));

  // ---- Vertical regions --------------------------------------------------
  IRECT rest        = bounds.GetReducedFromTop(28.f + gap);
  const IRECT bottomRow   = rest.GetFromBottom(26.f);
  const IRECT controlsArea = rest.GetReducedFromBottom(26.f + gap).GetFromBottom(74.f);
  const IRECT midArea     = rest.GetReducedFromBottom(26.f + gap + 74.f + gap);

  // ---- Middle : three columns -------------------------------------------
  const IRECT modePanel    = midArea.GetFromLeft(150.f);
  const IRECT metersPanel  = midArea.GetFromRight(120.f);
  const IRECT strengthPanel = midArea.GetReducedFromLeft(150.f + gap)
                                     .GetReducedFromRight(120.f + gap);

  // Panels (drawn first, content on top).
  pGraphics->AttachControl(new SectionPanelControl(modePanel, Theme::RadiusMedium));
  pGraphics->AttachControl(new SectionPanelControl(strengthPanel, Theme::RadiusMedium));
  pGraphics->AttachControl(new SectionPanelControl(metersPanel, Theme::RadiusMedium));

  // MODE column.
  {
    const IRECT inner = modePanel.GetPadded(-Theme::SpacingSmall);
    attachSectionTitle(inner.GetFromTop(14.f), "MODE");
    const IRECT list = inner.GetReducedFromTop(18.f);
    pGraphics->AttachControl(new ModeListControl(list, kMode,
                                                 static_cast<int>(odf::Mode::NumModes)));
  }

  // STRENGTH column.
  {
    const IRECT inner = strengthPanel.GetPadded(-Theme::SpacingSmall);
    attachSectionTitle(inner.GetFromTop(14.f), "STRENGTH");
    pGraphics->AttachControl(new StrengthControl(inner.GetReducedFromTop(16.f), kStrength));
  }

  // METERS column.
  {
    const IRECT inner = metersPanel.GetPadded(-Theme::SpacingSmall);
    attachSectionTitle(inner.GetFromTop(14.f), "METERS");
    pGraphics->AttachControl(new MetersControl(inner.GetReducedFromTop(16.f)), kCtrlTagMeters);
  }

  // ---- Amount sliders panel ---------------------------------------------
  pGraphics->AttachControl(new SectionPanelControl(controlsArea, Theme::RadiusMedium));
  {
    const IRECT inner = controlsArea.GetPadded(-Theme::SpacingSmall);
    const int kRows = 4;
    const int kParams[kRows]   = { kFeedbackAmount, kNoiseAmount, kRoomAmount, kArtifactGuard };
    const char* kLabels[kRows] = { "Feedback", "Noise", "Room", "Guard" };
    for (int i = 0; i < kRows; ++i)
    {
      const IRECT row = inner.GetGridCell(i, kRows, 1).GetMidVPadded(8.f);
      pGraphics->AttachControl(new MiniSliderControl(row, kParams[i], kLabels[i]));
    }
  }

  // ---- Bottom row : buttons / readout / cpu -----------------------------
  pGraphics->AttachControl(new IVToggleControl(bottomRow.GetFromLeft(64.f).GetMidVPadded(11.f),
                                               kMute, "", warnStyle, "Mute", "MUTE"));
  pGraphics->AttachControl(new IVToggleControl(bottomRow.GetFromLeft(132.f).GetFromRight(64.f).GetMidVPadded(11.f),
                                               kBypass, "", style, "Bypass", "BYP"));

  pGraphics->AttachControl(new StatusPillControl(bottomRow.GetFromRight(90.f).GetMidVPadded(11.f),
                                                 "CPU: -- %"));

  const IRECT readout = bottomRow.GetReducedFromLeft(140.f).GetReducedFromRight(98.f).GetMidVPadded(11.f);
  pGraphics->AttachControl(new ReadoutControl(readout,
    "Detected: --    Reduction: 0.0 dB"), kCtrlTagReadout);
}

#endif // IPLUG_EDITOR
