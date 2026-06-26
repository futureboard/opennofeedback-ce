// ============================================================================
//  OpenDeFeedback_Controls.cpp
// ============================================================================
#include "OpenDeFeedback_Controls.h"

#include <cmath>
#include <cstring>
#include <algorithm>

namespace odf {

// ---------------------------------------------------------------------------
//  Shared styles (colors sourced entirely from odf::Theme).
// ---------------------------------------------------------------------------
IVStyle ThemedStyle()
{
  return DEFAULT_STYLE
    .WithColor(kBG, ToIColor(Theme::Panel))
    .WithColor(kFG, ToIColor(Theme::AccentMuted))
    .WithColor(kPR, ToIColor(Theme::Accent))
    .WithColor(kFR, ToIColor(Theme::Border))
    .WithColor(kHL, ToIColor(Theme::Accent))
    .WithColor(kX1, ToIColor(Theme::Accent))
    .WithColor(kX3, ToIColor(Theme::TextPrimary))
    .WithLabelText(IText(Theme::FontSizeSmall, ToIColor(Theme::TextSecondary), kUIFontID, EAlign::Center))
    .WithValueText(IText(Theme::FontSizeBody,  ToIColor(Theme::TextPrimary),   kUIFontID, EAlign::Center))
    .WithDrawShadows(false)
    .WithDrawFrame(true)
    .WithFrameThickness(1.f)
    .WithRoundness(0.2f)
    .WithWidgetFrac(0.9f);
}

IVStyle ThemedDangerStyle()
{
  return ThemedStyle()
    .WithColor(kPR, ToIColor(Theme::Danger))
    .WithColor(kX1, ToIColor(Theme::Danger))
    .WithColor(kHL, ToIColor(Theme::Danger));
}

// ---------------------------------------------------------------------------
//  HeaderControl
// ---------------------------------------------------------------------------
HeaderControl::HeaderControl(const IRECT& bounds, const char* title, const char* subtitle)
: IControl(bounds), mTitle(title), mSubtitle(subtitle)
{
  mIgnoreMouse = true;
}

void HeaderControl::Draw(IGraphics& g)
{
  UI::DrawHeaderText(g, mRECT, mTitle.Get(), mSubtitle.Get());
}

// ---------------------------------------------------------------------------
//  SectionPanelControl
// ---------------------------------------------------------------------------
SectionPanelControl::SectionPanelControl(const IRECT& bounds, float radius)
: IControl(bounds), mRadius(radius)
{
  mIgnoreMouse = true;
}

void SectionPanelControl::Draw(IGraphics& g)
{
  UI::DrawPanel(g, mRECT, mRadius);
}

// ---------------------------------------------------------------------------
//  StatusPillControl
// ---------------------------------------------------------------------------
StatusPillControl::StatusPillControl(const IRECT& bounds, const char* text)
: IControl(bounds), mText(text)
{
  mIgnoreMouse = true;
}

void StatusPillControl::Draw(IGraphics& g)
{
  UI::DrawStatusPill(g, mRECT, mText.Get());
}

void StatusPillControl::SetText(const char* text)
{
  if (std::strcmp(text, mText.Get()) != 0)
  {
    mText.Set(text);
    SetDirty(false);
  }
}

// ---------------------------------------------------------------------------
//  ReadoutControl
// ---------------------------------------------------------------------------
ReadoutControl::ReadoutControl(const IRECT& bounds, const char* text)
: IControl(bounds), mText(text)
{
  mIgnoreMouse = true;
}

void ReadoutControl::Draw(IGraphics& g)
{
  g.FillRoundRect(ToIColor(Theme::PanelElevated), mRECT, Theme::RadiusSmall);
  g.DrawRoundRect(ToIColor(Theme::BorderMuted), mRECT, Theme::RadiusSmall, nullptr, 1.f);
  const IText t(Theme::FontSizeBody, ToIColor(Theme::Accent), kUIFontID, EAlign::Center, EVAlign::Middle);
  g.DrawText(t, mText.Get(), mRECT);
}

void ReadoutControl::SetText(const char* text)
{
  if (std::strcmp(text, mText.Get()) != 0)
  {
    mText.Set(text);
    SetDirty(false);
  }
}

// ---------------------------------------------------------------------------
//  MetersControl
// ---------------------------------------------------------------------------
MetersControl::MetersControl(const IRECT& bounds) : IControl(bounds)
{
  mIgnoreMouse = true;
}

void MetersControl::Draw(IGraphics& g)
{
  const IRECT inner = mRECT.GetPadded(-Theme::SpacingSmall);
  const float rowH = inner.H() / 3.f;
  UI::DrawMeterBar(g, inner.GetGridCell(0, 3, 1).GetMidVPadded(rowH * 0.5f - 7.f), mIn,        "IN");
  UI::DrawMeterBar(g, inner.GetGridCell(1, 3, 1).GetMidVPadded(rowH * 0.5f - 7.f), mOut,       "OUT");
  UI::DrawMeterBar(g, inner.GetGridCell(2, 3, 1).GetMidVPadded(rowH * 0.5f - 7.f), mReduction, "RED");
}

void MetersControl::SetLevels(float inNorm, float outNorm, float reductionNorm)
{
  mIn = inNorm; mOut = outNorm; mReduction = reductionNorm;
  SetDirty(false);
}

// ---------------------------------------------------------------------------
//  StrengthControl — large numeric intensity readout, drag to change.
// ---------------------------------------------------------------------------
StrengthControl::StrengthControl(const IRECT& bounds, int paramIdx)
: IKnobControlBase(bounds, paramIdx)
{
}

void StrengthControl::Draw(IGraphics& g)
{
  const float v = static_cast<float>(GetValue());

  // Big number (0..100).
  WDL_String num;
  num.SetFormatted(8, "%d", static_cast<int>(std::round(v * 100.f)));
  const IText big(Theme::FontSizeDisplay, ToIColor(Theme::TextPrimary),
                  kUIFontID, EAlign::Center, EVAlign::Middle);
  g.DrawText(big, num.Get(), mRECT.GetFromTop(mRECT.H() * 0.55f));

  // Caption.
  const IText cap(Theme::FontSizeSmall, ToIColor(Theme::TextMuted),
                  kUIFontID, EAlign::Center, EVAlign::Middle);
  g.DrawText(cap, "clean intensity", mRECT.GetMidVPadded(8.f).GetVShifted(8.f));

  // Thin progress bar.
  const IRECT track = mRECT.GetFromBottom(6.f).GetMidHPadded(mRECT.W() * 0.32f);
  g.FillRoundRect(ToIColor(Theme::PanelElevated), track, Theme::RadiusSmall);
  if (v > 0.f)
    g.FillRoundRect(ToIColor(Theme::Accent), track.GetFromLeft(track.W() * v), Theme::RadiusSmall);
}

// ---------------------------------------------------------------------------
//  ModeListControl — vertical selectable list bound to an enum param.
// ---------------------------------------------------------------------------
ModeListControl::ModeListControl(const IRECT& bounds, int paramIdx, int numItems)
: IControl(bounds, paramIdx), mNumItems(numItems)
{
}

int ModeListControl::HitItem(float y) const
{
  const float itemH = mRECT.H() / static_cast<float>(mNumItems);
  int idx = static_cast<int>((y - mRECT.T) / itemH);
  return std::min(std::max(idx, 0), mNumItems - 1);
}

void ModeListControl::Draw(IGraphics& g)
{
  const IParam* pParam = GetParam();
  const int current = pParam ? pParam->Int() : 0;
  const float itemH = mRECT.H() / static_cast<float>(mNumItems);

  for (int i = 0; i < mNumItems; ++i)
  {
    const IRECT row = mRECT.GetFromTop(itemH * (i + 1)).GetFromBottom(itemH).GetPadded(-2.f);
    const bool selected = (i == current);

    if (selected)
      g.FillRoundRect(ToIColor(Theme::AccentMuted), row, Theme::RadiusSmall);

    const IColor textColor = selected ? ToIColor(Theme::Accent) : ToIColor(Theme::TextSecondary);
    const IText t(Theme::FontSizeBody, textColor, kUIFontID, EAlign::Near, EVAlign::Middle);
    const char* label = pParam ? pParam->GetDisplayTextAtIdx(i) : "";
    g.DrawText(t, label, row.GetHPadded(-Theme::SpacingSmall));
  }
}

void ModeListControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  const int idx = HitItem(y);
  if (const IParam* pParam = GetParam())
  {
    SetValue(pParam->ToNormalized(static_cast<double>(idx)));
    SetDirty(true);
  }
}

// ---------------------------------------------------------------------------
//  MiniSliderControl
// ---------------------------------------------------------------------------
MiniSliderControl::MiniSliderControl(const IRECT& bounds, int paramIdx, const char* label)
: ISliderControlBase(bounds, paramIdx, EDirection::Horizontal), mLabel(label)
{
}

void MiniSliderControl::Draw(IGraphics& g)
{
  const float v = static_cast<float>(GetValue());
  WDL_String valStr;
  valStr.SetFormatted(16, "%d%%", static_cast<int>(std::round(v * 100.f)));
  UI::DrawMiniSlider(g, mRECT, v, mLabel.Get(), valStr.Get());
}

} // namespace odf
