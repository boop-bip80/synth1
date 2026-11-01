#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cs80::ui
{
namespace colour
{
// The Series 5 panel is a black-lacquer instrument: graphite and cold steel
// under warm brass work lights, with walnut cheeks and amber glass. Accents are
// jewel tones, one per section, so the whole surface reads at a glance.
inline const juce::Colour windowBg    { 0xff02050a };
inline const juce::Colour stageTop    { 0xff07131c };
inline const juce::Colour stageBottom { 0xff010204 };

inline const juce::Colour woodDark    { 0xff170904 };
inline const juce::Colour woodMid     { 0xff4f2513 };
inline const juce::Colour woodLight   { 0xffa96a38 };
inline const juce::Colour woodHot     { 0xffe5a96c };

inline const juce::Colour panelTop    { 0xff24333f };
inline const juce::Colour panelMid    { 0xff0f1921 };
inline const juce::Colour panelBottom { 0xff050a0e };

inline const juce::Colour plateTop    { 0xff1e2c37 };
inline const juce::Colour plateMid    { 0xff0c151c };
inline const juce::Colour plateBottom { 0xff04080b };

inline const juce::Colour line        { 0xff405265 };
inline const juce::Colour shadow      { 0xff010203 };
inline const juce::Colour text        { 0xfff7efdf };
inline const juce::Colour textDim     { 0xffcbbaa0 };
inline const juce::Colour textFaint   { 0xff8d8273 };

inline const juce::Colour amber       { 0xffffb45f };
inline const juce::Colour amberHot    { 0xffffe7ba };
inline const juce::Colour amberDim    { 0xff8b5b26 };
inline const juce::Colour gold        { 0xffecc781 };
inline const juce::Colour brass       { 0xffd9ae72 };
inline const juce::Colour brassDark   { 0xff6f502a };
inline const juce::Colour brassLight  { 0xfff3d7a3 };

inline const juce::Colour lcdBg       { 0xff080502 };
inline const juce::Colour lcdText     { 0xffffcb82 };
inline const juce::Colour lcdGlow     { 0xffffa640 };

inline const juce::Colour red         { 0xffff7b59 };
inline const juce::Colour green       { 0xffa6e3b4 };
inline const juce::Colour steel       { 0xffaab9c7 };
inline const juce::Colour keyIvory    { 0xfff8f0de };
inline const juce::Colour cream       { 0xfffcf4e3 };
inline const juce::Colour copper      { 0xffffa274 };
inline const juce::Colour sage        { 0xffa5e0b9 };
inline const juce::Colour slate       { 0xff8ccdf2 };
inline const juce::Colour rose        { 0xffffa6c5 };
inline const juce::Colour violet      { 0xffc7b4f2 };
inline const juce::Colour cyan        { 0xff7fe3dc };
inline const juce::Colour felt        { 0xff7a2026 };
inline const juce::Colour keyBlack    { 0xff10161b };
inline const juce::Colour keyBlackHi  { 0xff4d5861 };

// Additive stage light used behind the badge, LCD and scope.
inline const juce::Colour aurora      { 0xff58d6c2 };
inline const juce::Colour auroraDeep  { 0xff1b6d72 };
inline const juce::Colour ice         { 0xff9bd9ff };
inline const juce::Colour emerald     { 0xff8fe3ad };
inline const juce::Colour tangerine   { 0xffffa56b };
inline const juce::Colour magenta     { 0xffef9ad4 };

inline const juce::Colour screenLine  { 0xffffc16e };
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
