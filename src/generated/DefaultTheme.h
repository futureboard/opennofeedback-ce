// =============================================================================
//  DefaultTheme.h  -- GENERATED FILE, DO NOT EDIT BY HAND.
//
//  Produced by tools/generate_theme_header.py from a design-time theme source.
//  This is the ONLY theme source the plugin uses at runtime. The plugin never
//  opens, reads or parses any JSON at runtime. Tokens are neutral semantic
//  names only.
// =============================================================================
#pragma once

namespace odf { namespace Theme {

struct Color {
  float r;
  float g;
  float b;
  float a;
};

inline constexpr Color Background     {0.07f, 0.075f, 0.085f, 1.0f};
inline constexpr Color Panel          {0.095f, 0.102f, 0.118f, 1.0f};
inline constexpr Color PanelElevated  {0.12f, 0.13f, 0.15f, 1.0f};
inline constexpr Color Border         {0.21f, 0.225f, 0.25f, 1.0f};
inline constexpr Color BorderMuted    {0.15f, 0.16f, 0.18f, 1.0f};
inline constexpr Color TextPrimary    {0.88f, 0.9f, 0.93f, 1.0f};
inline constexpr Color TextSecondary  {0.6f, 0.64f, 0.7f, 1.0f};
inline constexpr Color TextMuted      {0.42f, 0.45f, 0.5f, 1.0f};
inline constexpr Color Accent         {0.48f, 0.62f, 1.0f, 1.0f};
inline constexpr Color AccentMuted    {0.23f, 0.3f, 0.46f, 1.0f};
inline constexpr Color Warning        {1.0f, 0.639216f, 0.25098f, 1.0f};
inline constexpr Color Danger         {1.0f, 0.360784f, 0.360784f, 1.0f};
inline constexpr Color Success        {0.4f, 0.84f, 0.6f, 1.0f};
inline constexpr Color MeterInput     {0.48f, 0.62f, 1.0f, 1.0f};
inline constexpr Color MeterOutput    {0.4f, 0.84f, 0.6f, 1.0f};
inline constexpr Color MeterReduction {1.0f, 0.639216f, 0.25098f, 1.0f};

inline constexpr float RadiusSmall     = 5.0f;
inline constexpr float RadiusMedium    = 8.0f;
inline constexpr float RadiusLarge     = 12.0f;
inline constexpr float SpacingXSmall   = 4.0f;
inline constexpr float SpacingSmall    = 8.0f;
inline constexpr float SpacingMedium   = 12.0f;
inline constexpr float SpacingLarge    = 16.0f;
inline constexpr float FontSizeTiny    = 10.0f;
inline constexpr float FontSizeSmall   = 11.0f;
inline constexpr float FontSizeBody    = 12.0f;
inline constexpr float FontSizeTitle   = 14.0f;
inline constexpr float FontSizeDisplay = 28.0f;

} } // namespace odf::Theme
