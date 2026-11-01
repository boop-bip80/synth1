#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cs80::ui
{
namespace colour
{
inline const juce::Colour windowBg    { 0xff03060a };
inline const juce::Colour stageTop    { 0xff0b1620 };
inline const juce::Colour stageBottom { 0xff010204 };

inline const juce::Colour woodDark    { 0xff210c05 };
inline const juce::Colour woodMid     { 0xff5c2c16 };
inline const juce::Colour woodLight   { 0xffb5773f };
inline const juce::Colour woodHot     { 0xffe2a76d };

inline const juce::Colour panelTop    { 0xff2b3844 };
inline const juce::Colour panelMid    { 0xff131d25 };
inline const juce::Colour panelBottom { 0xff070c11 };

inline const juce::Colour plateTop    { 0xff23303b };
inline const juce::Colour plateMid    { 0xff111a22 };
inline const juce::Colour plateBottom { 0xff080d12 };

inline const juce::Colour line        { 0xff3d4e5b };
inline const juce::Colour shadow      { 0xff010203 };
inline const juce::Colour text        { 0xfff4ecdc };
inline const juce::Colour textDim     { 0xffc8b69a };
inline const juce::Colour textFaint   { 0xff8b8172 };

inline const juce::Colour amber       { 0xffffb765 };
inline const juce::Colour amberHot    { 0xffffe5bb };
inline const juce::Colour amberDim    { 0xff855324 };
inline const juce::Colour gold        { 0xffe4bd83 };
inline const juce::Colour brass       { 0xffcfa66b };
inline const juce::Colour brassDark   { 0xff6d4d28 };

inline const juce::Colour lcdBg       { 0xff0a0702 };
inline const juce::Colour lcdText     { 0xffffc77a };
inline const juce::Colour lcdGlow     { 0xffffa844 };

inline const juce::Colour red         { 0xffe06a48 };
inline const juce::Colour green       { 0xff96c6a0 };
inline const juce::Colour steel       { 0xff9aa8b5 };
inline const juce::Colour keyIvory    { 0xfff5ecd9 };
inline const juce::Colour cream       { 0xfff7eeda };
inline const juce::Colour copper      { 0xffe59d6d };
inline const juce::Colour sage        { 0xffa6c99c };
inline const juce::Colour slate       { 0xff94bce0 };
inline const juce::Colour rose        { 0xffe6a6ac };
inline const juce::Colour violet      { 0xffbcaee8 };
inline const juce::Colour cyan        { 0xff9bded8 };
inline const juce::Colour felt        { 0xff6c1d21 };
inline const juce::Colour keyBlack    { 0xff14191e };
inline const juce::Colour keyBlackHi  { 0xff49525a };

inline const juce::Colour screenLine  { 0xffffb765 };
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

//==============================================================================
void drawPlate (juce::Graphics& g, juce::Rectangle<float> area,
                float cornerSize = 5.0f, bool raised = true);

void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area,
                    float cornerSize = 4.0f);

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius);

void drawGlow (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour colour, float intensity);

void drawWoodFrame (juce::Graphics& g, juce::Rectangle<int> area);

void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<int> area);

void drawGlassSheen (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize);

void drawFeltRail (juce::Graphics& g, juce::Rectangle<float> area);

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity = 1.0f);

juce::Colour categoryColour (const juce::String& category);

juce::String formatTime (float seconds);
juce::String formatHz (float hz);
} // namespace cs80::ui
