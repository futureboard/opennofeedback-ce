// ============================================================================
//  OpenDeFeedback_Controls.h
//
//  Custom flat vector controls for the compact, modern, dark, technical,
//  minimal, editor-like UI. Every color/spacing/radius/font size is taken from
//  the generated static theme (odf::Theme) via the UI primitives in
//  ui/Theme.h. No raw color literals appear in this file or its .cpp.
// ============================================================================
#pragma once

#include "IControl.h"
#include "IControls.h"
#include "../ui/Theme.h"

using namespace iplug;
using namespace igraphics;

namespace odf {

/** Shared IVStyle for the interactive vector controls (knob list / toggles),
 *  with all colors sourced from the generated theme. */
IVStyle ThemedStyle();

/** Variant using the Danger accent (e.g. the Mute toggle). */
IVStyle ThemedDangerStyle();

// ---------------------------------------------------------------------------
//  HeaderControl — product title + subtitle and a thin accent tick.
// ---------------------------------------------------------------------------
class HeaderControl : public IControl
{
public:
  HeaderControl(const IRECT& bounds, const char* title, const char* subtitle);
  void Draw(IGraphics& g) override;
private:
  WDL_String mTitle, mSubtitle;
};

// ---------------------------------------------------------------------------
//  SectionPanelControl — a flat rounded background panel for grouping.
// ---------------------------------------------------------------------------
class SectionPanelControl : public IControl
{
public:
  SectionPanelControl(const IRECT& bounds, float radius);
  void Draw(IGraphics& g) override;
private:
  float mRadius;
};

// ---------------------------------------------------------------------------
//  StatusPillControl — small pill with a settable text (e.g. latency / CPU).
// ---------------------------------------------------------------------------
class StatusPillControl : public IControl
{
public:
  StatusPillControl(const IRECT& bounds, const char* text);
  void Draw(IGraphics& g) override;
  void SetText(const char* text);
private:
  WDL_String mText;
};

// ---------------------------------------------------------------------------
//  ReadoutControl — single technical readout line (detected / reduction).
// ---------------------------------------------------------------------------
class ReadoutControl : public IControl
{
public:
  ReadoutControl(const IRECT& bounds, const char* text);
  void Draw(IGraphics& g) override;
  void SetText(const char* text);
private:
  WDL_String mText;
};

// ---------------------------------------------------------------------------
//  MetersControl — input / output / reduction level bars (read-only).
//  Levels are pushed from the editor's idle callback.
// ---------------------------------------------------------------------------
class MetersControl : public IControl
{
public:
  explicit MetersControl(const IRECT& bounds);
  void Draw(IGraphics& g) override;
  void SetLevels(float inNorm, float outNorm, float reductionNorm);
private:
  float mIn = 0.f, mOut = 0.f, mReduction = 0.f;
};

// ---------------------------------------------------------------------------
//  StrengthControl — large numeric "intensity" display, drag to change.
//  Flat / technical: a big number plus a thin progress bar, no skeuomorphism.
// ---------------------------------------------------------------------------
class StrengthControl : public IKnobControlBase
{
public:
  StrengthControl(const IRECT& bounds, int paramIdx);
  void Draw(IGraphics& g) override;
};

// ---------------------------------------------------------------------------
//  ModeListControl — vertical selectable list bound to the Mode enum param.
// ---------------------------------------------------------------------------
class ModeListControl : public IControl
{
public:
  ModeListControl(const IRECT& bounds, int paramIdx, int numItems);
  void Draw(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
private:
  int mNumItems;
  int HitItem(float y) const;
};

// ---------------------------------------------------------------------------
//  MiniSliderControl — compact horizontal slider bound to a 0..1 param, drawn
//  via the themed DrawMiniSlider primitive. Shows a label and a value string.
// ---------------------------------------------------------------------------
class MiniSliderControl : public ISliderControlBase
{
public:
  MiniSliderControl(const IRECT& bounds, int paramIdx, const char* label);
  void Draw(IGraphics& g) override;
private:
  WDL_String mLabel;
};

} // namespace odf
