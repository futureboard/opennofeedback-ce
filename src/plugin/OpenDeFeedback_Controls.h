// ============================================================================
//  OpenDeFeedback_Controls.h
//
//  Shared GUI theme (dark live-console palette) and a couple of small custom
//  IGraphics controls used by the layout. Drawing is vector-based (NanoVG via
//  IGraphics), no bitmaps.
// ============================================================================
#pragma once

#include "IControl.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;

namespace odfui
{

// ---- Dark graphite palette -------------------------------------------------
const IColor kColorBg        = IColor(255, 18, 20, 24);   // deep graphite
const IColor kColorPanel     = IColor(255, 28, 31, 37);   // raised panel
const IColor kColorPanelEdge = IColor(255, 44, 48, 56);
const IColor kColorText      = IColor(255, 210, 214, 220);
const IColor kColorTextDim   = IColor(255, 130, 136, 146);
const IColor kColorAccent    = IColor(255, 0, 200, 170);  // teal accent
const IColor kColorAccentDim = IColor(255, 0, 120, 104);
const IColor kColorWarn      = IColor(255, 235, 96, 80);   // mute/clip red

/** The house IVStyle used by every vector control for a consistent flat look. */
IVStyle ODFStyle();

/** Variant with the warning (red) accent — used by Mute. */
IVStyle ODFWarnStyle();

// ---------------------------------------------------------------------------
//  ODFTitleControl — draws the plugin name + a thin accent underline.
// ---------------------------------------------------------------------------
class ODFTitleControl : public IControl
{
public:
  ODFTitleControl(const IRECT& bounds, const char* title, const char* subtitle);
  void Draw(IGraphics& g) override;

private:
  WDL_String mTitle;
  WDL_String mSubtitle;
};

// ---------------------------------------------------------------------------
//  ODFReadoutControl — text readout (detected freq / reduction / latency).
//  A thin subclass of ITextControl so we have a concrete type to target by tag.
// ---------------------------------------------------------------------------
class ODFReadoutControl : public ITextControl
{
public:
  ODFReadoutControl(const IRECT& bounds, const char* initialStr);
  void Draw(IGraphics& g) override;
};

} // namespace odfui
