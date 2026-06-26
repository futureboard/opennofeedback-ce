// ============================================================================
//  ui/Theme.cpp — flat vector UI primitives, themed entirely from odf::Theme.
// ============================================================================
#include "Theme.h"
#include "IGraphics.h"

#include <cstring>
#include <algorithm>

namespace odf {

const char* LoadUIFont(IGraphics& g)
{
  // Prefer the variable UI font; fall back to the bundled face if absent. If
  // neither resource is found the platform default is used and text still lays
  // out — the UI degrades gracefully rather than failing.
  if (g.LoadFont(kUIFontID, kUIFontFile))
    return kUIFontID;
  g.LoadFont(kUIFontID, kUIFontFallbackFile);
  return kUIFontID;
}

namespace UI {

static inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

void DrawPanel(IGraphics& g, const IRECT& r, float radius)
{
  g.FillRoundRect(ToIColor(Theme::Panel), r, radius);
  g.DrawRoundRect(ToIColor(Theme::Border), r, radius, nullptr, 1.f);
}

void DrawHeaderText(IGraphics& g, const IRECT& r, const char* title, const char* subtitle)
{
  const IText titleText(Theme::FontSizeTitle, ToIColor(Theme::TextPrimary),
                        kUIFontID, EAlign::Near, EVAlign::Middle);
  const IText subText(Theme::FontSizeSmall, ToIColor(Theme::TextSecondary),
                      kUIFontID, EAlign::Near, EVAlign::Middle);

  const IRECT titleRect = r.GetFromTop(r.H() * 0.62f);
  const IRECT subRect   = r.GetFromBottom(r.H() * 0.38f);

  g.DrawText(titleText, title ? title : "", titleRect);
  if (subtitle && subtitle[0])
    g.DrawText(subText, subtitle, subRect);

  // Thin accent tick under the title.
  const float y = titleRect.B;
  g.FillRect(ToIColor(Theme::Accent), IRECT(r.L, y - 1.f, r.L + Theme::SpacingLarge * 2.f, y + 1.f));
}

void DrawStatusPill(IGraphics& g, const IRECT& r, const char* text)
{
  g.FillRoundRect(ToIColor(Theme::PanelElevated), r, Theme::RadiusSmall);
  g.DrawRoundRect(ToIColor(Theme::BorderMuted), r, Theme::RadiusSmall, nullptr, 1.f);
  const IText t(Theme::FontSizeTiny, ToIColor(Theme::TextSecondary),
                kUIFontID, EAlign::Center, EVAlign::Middle);
  g.DrawText(t, text ? text : "", r);
}

void DrawMeterBar(IGraphics& g, const IRECT& r, float value, const char* label)
{
  // Choose the semantic meter color from the (neutral) label so the primitive
  // keeps a fixed signature while In/Out/Reduction stay visually distinct.
  Theme::Color barColor = Theme::Accent;
  if (label)
  {
    if (std::strcmp(label, "OUT") == 0)      barColor = Theme::MeterOutput;
    else if (std::strcmp(label, "RED") == 0) barColor = Theme::MeterReduction;
    else if (std::strcmp(label, "IN") == 0)  barColor = Theme::MeterInput;
  }

  const IRECT labelRect = r.GetFromLeft(30.f);
  const IRECT track     = r.GetReducedFromLeft(34.f).GetMidVPadded(r.H() * 0.30f);

  const IText lt(Theme::FontSizeTiny, ToIColor(Theme::TextMuted),
                 kUIFontID, EAlign::Near, EVAlign::Middle);
  g.DrawText(lt, label ? label : "", labelRect);

  g.FillRoundRect(ToIColor(Theme::PanelElevated), track, Theme::RadiusSmall);
  const float v = Clamp01(value);
  if (v > 0.f)
  {
    const IRECT fill = track.GetFromLeft(track.W() * v);
    g.FillRoundRect(ToIColor(barColor), fill, Theme::RadiusSmall);
  }
}

void DrawMiniSlider(IGraphics& g, const IRECT& r, float value, const char* label, const char* valueText)
{
  const IRECT labelRect = r.GetFromLeft(62.f);
  const IRECT valueRect = r.GetFromRight(104.f);
  const IRECT track     = r.GetReducedFromLeft(66.f).GetReducedFromRight(108.f)
                           .GetMidVPadded(std::min(5.f, r.H() * 0.25f));

  const IText labelText(Theme::FontSizeSmall, ToIColor(Theme::TextSecondary),
                        kUIFontID, EAlign::Near, EVAlign::Middle);
  const IText valText(Theme::FontSizeSmall, ToIColor(Theme::TextMuted),
                      kUIFontID, EAlign::Far, EVAlign::Middle);

  g.DrawText(labelText, label ? label : "", labelRect);

  g.FillRoundRect(ToIColor(Theme::PanelElevated), track, Theme::RadiusSmall);
  const float v = Clamp01(value);
  if (v > 0.f)
  {
    const IRECT fill = track.GetFromLeft(track.W() * v);
    g.FillRoundRect(ToIColor(Theme::Accent), fill, Theme::RadiusSmall);
  }

  if (valueText && valueText[0])
    g.DrawText(valText, valueText, valueRect);
}

} // namespace UI
} // namespace odf
