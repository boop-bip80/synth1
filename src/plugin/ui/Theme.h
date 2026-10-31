#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cs80::ui
{
namespace colour
{
inline const juce::Colour windowBg    { 0xff060709 };
inline const juce::Colour woodDark    { 0xff2a1408 };
inline const juce::Colour woodMid     { 0xff6b3a1a };
inline const juce::Colour woodLight   { 0xffa9743f };
inline const juce::Colour panelTop    { 0xff2f343b };
inline const juce::Colour panelBottom { 0xff121519 };
inline const juce::Colour plateTop    { 0xff2b3038 };
inline const juce::Colour plateBottom { 0xff181b20 };
inline const juce::Colour line        { 0xff3d444e };
inline const juce::Colour shadow      { 0xff06080a };
inline const juce::Colour text        { 0xffd5d9e0 };
inline const juce::Colour textDim     { 0xff9ba1ab };
inline const juce::Colour textFaint   { 0xff6d7480 };
inline const juce::Colour amber       { 0xffff9d3c };
inline const juce::Colour amberHot    { 0xffffd39a };
inline const juce::Colour amberDim    { 0xff7a451a };
inline const juce::Colour lcdBg       { 0xff140d05 };
inline const juce::Colour lcdText     { 0xffffb45a };
inline const juce::Colour red         { 0xffe0533f };
inline const juce::Colour green       { 0xff7fce9b };
inline const juce::Colour steel       { 0xff9aa1ab };
inline const juce::Colour keyIvory    { 0xffe9e2d3 };
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
