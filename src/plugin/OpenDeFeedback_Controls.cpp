// ============================================================================
//  OpenDeFeedback_Controls.cpp
// ============================================================================
#include "OpenDeFeedback_Controls.h"

namespace odfui
{

IVStyle ODFStyle()
{
  // Flat, dark, thin-stroke style shared by all vector controls.
  return DEFAULT_STYLE
    .WithColor(kBG, kColorPanel)
    .WithColor(kFG, kColorAccentDim)
    .WithColor(kPR, kColorAccent)        // pressed / active value
    .WithColor(kFR, kColorPanelEdge)     // frame
    .WithColor(kHL, kColorAccent)        // highlight
    .WithColor(kX1, kColorAccent)        // handle / indicator
    .WithColor(kX3, kColorText)
    .WithLabelText(IText(14.f, kColorTextDim, "Roboto-Regular", EAlign::Center))
    .WithValueText(IText(14.f, kColorText, "Roboto-Regular", EAlign::Center))
    .WithDrawShadows(false)
    .WithDrawFrame(true)
    .WithFrameThickness(1.f)
    .WithRoundness(0.18f)
    .WithWidgetFrac(0.9f);
}

IVStyle ODFWarnStyle()
{
  return ODFStyle()
    .WithColor(kPR, kColorWarn)
    .WithColor(kX1, kColorWarn)
    .WithColor(kHL, kColorWarn);
}

// ---------------------------------------------------------------------------
//  ODFTitleControl
// ---------------------------------------------------------------------------
ODFTitleControl::ODFTitleControl(const IRECT& bounds, const char* title, const char* subtitle)
: IControl(bounds)
, mTitle(title)
, mSubtitle(subtitle)
{
  mIgnoreMouse = true;
}

void ODFTitleControl::Draw(IGraphics& g)
{
  const IRECT b = mRECT;

  IText titleText(20.f, kColorText, "Roboto-Regular", EAlign::Near, EVAlign::Middle);
  IText subText(11.f, kColorTextDim, "Roboto-Regular", EAlign::Near, EVAlign::Middle);

  const IRECT titleRect = b.GetFromTop(b.H() * 0.6f);
  const IRECT subRect   = b.GetFromBottom(b.H() * 0.4f);

  g.DrawText(titleText, mTitle.Get(), titleRect);
  g.DrawText(subText, mSubtitle.Get(), subRect);

  // Thin accent underline beneath the title.
  const float y = titleRect.B;
  g.DrawLine(kColorAccent, b.L, y, b.L + 54.f, y, &mBlend, 2.f);
}

// ---------------------------------------------------------------------------
//  ODFReadoutControl
// ---------------------------------------------------------------------------
ODFReadoutControl::ODFReadoutControl(const IRECT& bounds, const char* initialStr)
: ITextControl(bounds, initialStr,
               IText(13.f, kColorAccent, "Roboto-Regular", EAlign::Center, EVAlign::Middle))
{
  mIgnoreMouse = true;
}

void ODFReadoutControl::Draw(IGraphics& g)
{
  // Inset panel behind the monospace-ish readout.
  g.FillRoundRect(kColorPanel, mRECT, 4.f);
  g.DrawRoundRect(kColorPanelEdge, mRECT, 4.f, &mBlend, 1.f);
  ITextControl::Draw(g);
}

} // namespace odfui
