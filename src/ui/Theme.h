// ============================================================================
//  ui/Theme.h
//
//  The UI layer's single entry point to the theme. It re-exports the generated,
//  static theme constants and declares a small set of flat vector drawing
//  primitives. All colors/spacing/radii/font sizes come from the generated
//  header — UI drawing code must not contain raw color literals.
//
//  Runtime never reads JSON: the values below originate from a build-time
//  generated header (see tools/generate_theme_header.py).
// ============================================================================
#pragma once

#include "IGraphicsStructs.h"            // IRECT, IColor, IText
#include "../generated/DefaultTheme.h"   // odf::Theme:: tokens (generated)

// Forward declaration keeps this header light; the .cpp pulls full IGraphics.
namespace iplug { namespace igraphics { class IGraphics; } }

using namespace iplug;
using namespace igraphics;

namespace odf {

// Logical font id registered with IGraphics, and the resource filenames the UI
// tries to load (preferred first, then a graceful fallback). These are resource
// names only — no external/absolute paths.
static constexpr const char* kUIFontID           = "ui";
static constexpr const char* kUIFontFile         = "InterVariable.ttf";
static constexpr const char* kUIFontFallbackFile = "Roboto-Regular.ttf";

/** Convert a generated theme Color (float RGBA 0..1) to an IGraphics IColor. */
inline IColor ToIColor(const Theme::Color& c)
{
  auto q = [](float v) -> int {
    int i = static_cast<int>(v * 255.0f + 0.5f);
    return i < 0 ? 0 : (i > 255 ? 255 : i);
  };
  return IColor(q(c.a), q(c.r), q(c.g), q(c.b));
}

/** Same, but override alpha (0..1) — handy for subtle fills/strokes. */
inline IColor ToIColor(const Theme::Color& c, float alpha)
{
  IColor out = ToIColor(c);
  int a = static_cast<int>(alpha * 255.0f + 0.5f);
  out.A = a < 0 ? 0 : (a > 255 ? 255 : a);
  return out;
}

/** Load the UI font, preferring kUIFontFile and falling back to
 *  kUIFontFallbackFile. Returns the font id that was successfully registered
 *  (always kUIFontID), so callers can use it regardless of which file loaded.
 *  If neither is present the platform default is used; text still lays out. */
const char* LoadUIFont(IGraphics& g);

// ---------------------------------------------------------------------------
//  Flat vector drawing primitives. Compact, modern, dark, technical, minimal —
//  no skeuomorphism, no hardware imitation. All styling comes from odf::Theme.
// ---------------------------------------------------------------------------
namespace UI {

void DrawPanel(IGraphics& g, const IRECT& r, float radius);
void DrawHeaderText(IGraphics& g, const IRECT& r, const char* title, const char* subtitle);
void DrawStatusPill(IGraphics& g, const IRECT& r, const char* text);
void DrawMeterBar(IGraphics& g, const IRECT& r, float value, const char* label);
void DrawMiniSlider(IGraphics& g, const IRECT& r, float value, const char* label, const char* valueText);

} // namespace UI
} // namespace odf
