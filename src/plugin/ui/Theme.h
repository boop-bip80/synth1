#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cs80::ui
{
namespace colour
{
inline const juce::Colour windowBg    { 0xff050608 };
inline const juce::Colour woodDark    { 0xff190b04 };
inline const juce::Colour woodMid     { 0xff48230f };
inline const juce::Colour woodLight   { 0xff98602c };
inline const juce::Colour panelTop    { 0xff1b242d };
inline const juce::Colour panelBottom { 0xff070a0d };
inline const juce::Colour plateTop    { 0xff161d24 };
inline const juce::Colour plateBottom { 0xff0a0f14 };
inline const juce::Colour line        { 0xff2b353f };
inline const juce::Colour shadow      { 0xff020304 };
inline const juce::Colour text        { 0xffe9dfcd };
inline const juce::Colour textDim     { 0xffa89b86 };
inline const juce::Colour textFaint   { 0xff6d6558 };
inline const juce::Colour amber       { 0xffffb257 };
inline const juce::Colour amberHot    { 0xffffe0b0 };
inline const juce::Colour amberDim    { 0xff7d4d1c };
inline const juce::Colour lcdBg       { 0xff0b0804 };
inline const juce::Colour lcdText     { 0xffffbe6e };
inline const juce::Colour red         { 0xffd95a3e };
inline const juce::Colour green       { 0xff91b89a };
inline const juce::Colour steel       { 0xff8d96a1 };
inline const juce::Colour keyIvory    { 0xffe6dcc8 };
inline const juce::Colour cream       { 0xfff2e7d0 };
inline const juce::Colour copper      { 0xffd98a52 };
inline const juce::Colour sage        { 0xff9db88d };
inline const juce::Colour slate       { 0xff8ba9bd };
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

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity = 1.0f);

juce::Colour categoryColour (const juce::String& category);

juce::String formatTime (float seconds);
juce::String formatHz (float hz);
} // namespace cs80::ui
