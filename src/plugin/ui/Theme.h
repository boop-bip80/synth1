#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cs80::ui
{
namespace colour
{
// The Series 5 panel is a warm-lacquer instrument: near-black walnut and
// smoked bronze under brass work lights, with an aged-ivory legend silkscreen
// and jewel-tone section cards. The palette is deliberately warmer and more
// saturated than a plain dark UI, so the whole surface reads as a lit object.
inline const juce::Colour windowBg    { 0xff050402 };
inline const juce::Colour stageTop    { 0xff120d08 };
inline const juce::Colour stageBottom { 0xff020201 };

inline const juce::Colour woodDark    { 0xff1a0b05 };
inline const juce::Colour woodMid     { 0xff5a2a14 };
inline const juce::Colour woodLight   { 0xffb97a41 };
inline const juce::Colour woodHot     { 0xfff0c084 };

inline const juce::Colour panelTop    { 0xff453b2f };
inline const juce::Colour panelMid    { 0xff211b15 };
inline const juce::Colour panelBottom { 0xff080604 };

inline const juce::Colour plateTop    { 0xff3a3127 };
inline const juce::Colour plateMid    { 0xff1a1510 };
inline const juce::Colour plateBottom { 0xff060504 };

inline const juce::Colour line        { 0xff5b4a37 };
inline const juce::Colour shadow      { 0xff010101 };
inline const juce::Colour text        { 0xfffaf0dc };
inline const juce::Colour textDim     { 0xffe3d1b4 };
inline const juce::Colour textFaint   { 0xffb7a184 };

inline const juce::Colour amber       { 0xffffb35c };
inline const juce::Colour amberHot    { 0xffffe6b8 };
inline const juce::Colour amberDim    { 0xff9a6428 };
inline const juce::Colour gold        { 0xfff4cf8a };
inline const juce::Colour brass       { 0xffe0b273 };
inline const juce::Colour brassDark   { 0xff7c582c };
inline const juce::Colour brassLight  { 0xfff8dfb0 };

inline const juce::Colour lcdBg       { 0xff0a0603 };
inline const juce::Colour lcdText     { 0xffffcc85 };
inline const juce::Colour lcdGlow     { 0xffffa63c };

inline const juce::Colour red         { 0xffff6f55 };
inline const juce::Colour green       { 0xffa9e3b0 };
inline const juce::Colour steel       { 0xffb9c6d2 };
inline const juce::Colour keyIvory    { 0xfffff6e4 };
inline const juce::Colour cream       { 0xfffff7e7 };
inline const juce::Colour copper      { 0xffff9f6a };
inline const juce::Colour sage        { 0xffb3e3a8 };
inline const juce::Colour slate       { 0xff86cdec };
inline const juce::Colour rose        { 0xffff9ab8 };
inline const juce::Colour violet      { 0xffc3aef5 };
inline const juce::Colour cyan        { 0xff7de8de };
inline const juce::Colour felt        { 0xff8d1f26 };
inline const juce::Colour keyBlack    { 0xff0c1014 };
inline const juce::Colour keyBlackHi  { 0xff5b666f };

// Additive stage light used behind the badge, LCD and scope.
inline const juce::Colour aurora      { 0xff57d9c5 };
inline const juce::Colour auroraDeep  { 0xff1a6e72 };
inline const juce::Colour ice         { 0xffa5dcff };
inline const juce::Colour emerald     { 0xff8fe0ad };
inline const juce::Colour tangerine   { 0xffff9a5c };
inline const juce::Colour magenta     { 0xfff2a0d8 };

inline const juce::Colour screenLine  { 0xffffc47a };
} // namespace colour

//==============================================================================
juce::Font font (float height, bool bold = false);
juce::Font monoFont (float height, bool bold = false);
juce::Font displayFont (float height);
juce::Font trackedFont (float height, float tracking, bool bold = false);

void drawTrackedText (juce::Graphics& g, const juce::String& text,
                      juce::Rectangle<int> area, juce::Colour colour,
                      juce::Justification justification,
                      juce::Font fontToUse, float tracking);

void drawEngravedText (juce::Graphics& g, const juce::String& text,
                       juce::Rectangle<int> area, juce::Colour colour,
                       juce::Justification justification,
                       juce::Font fontToUse, float tracking);

/** Draws chrome-filled text with a dark seat, a warm lower reflection and a
    fine white edge. Used for the CS-80E badge and the LCD title. */
void drawChromeText (juce::Graphics& g, const juce::String& text,
                     juce::Rectangle<int> area, juce::Font fontToUse,
                     juce::Colour topTint, juce::Colour bottomTint,
                     float tracking = 0.0f);

//==============================================================================
void drawPlate (juce::Graphics& g, juce::Rectangle<float> area,
                float cornerSize = 5.0f, bool raised = true);

void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area,
                    float cornerSize = 4.0f);

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius);

void drawGlow (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour colour, float intensity);

void drawAurora (juce::Graphics& g, juce::Rectangle<float> area,
                 juce::Colour warm, juce::Colour cool, float intensity);

void drawWoodFrame (juce::Graphics& g, juce::Rectangle<int> area);

void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<int> area);

void drawBrassRail (juce::Graphics& g, juce::Rectangle<float> area,
                    bool vertical = false, float alpha = 1.0f);

void drawVentGrille (juce::Graphics& g, juce::Rectangle<float> area,
                     juce::Colour tint, float alpha = 1.0f);

void drawGlassSheen (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize);

void drawFeltRail (juce::Graphics& g, juce::Rectangle<float> area);

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity = 1.0f);

juce::Colour categoryColour (const juce::String& category);

juce::String formatTime (float seconds);
juce::String formatHz (float hz);
} // namespace cs80::ui
