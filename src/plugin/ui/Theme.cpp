#include "Theme.h"

#include "BinaryData.h"

#include <cmath>

namespace cs80::ui
{
namespace
{
juce::uint32 hash (int value)
{
    auto x = (juce::uint32) value;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}
} // namespace

//==============================================================================
namespace
{
juce::Typeface::Ptr makeTypeface (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
}

juce::Typeface::Ptr uiRegular()
{
    static auto face = makeTypeface (BinaryData::ChakraPetchRegular_ttf, BinaryData::ChakraPetchRegular_ttfSize);
    return face;
}

juce::Typeface::Ptr uiSemiBold()
{
    static auto face = makeTypeface (BinaryData::ChakraPetchSemiBold_ttf, BinaryData::ChakraPetchSemiBold_ttfSize);
    return face;
}

juce::Typeface::Ptr uiBold()
{
    static auto face = makeTypeface (BinaryData::ChakraPetchBold_ttf, BinaryData::ChakraPetchBold_ttfSize);
    return face;
}

juce::Typeface::Ptr monoRegular()
{
    static auto face = makeTypeface (BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
    return face;
}

juce::Typeface::Ptr monoBold()
{
    static auto face = makeTypeface (BinaryData::JetBrainsMonoBold_ttf, BinaryData::JetBrainsMonoBold_ttfSize);
    return face;
}

juce::Typeface::Ptr displayFace()
{
    static auto face = makeTypeface (BinaryData::MichromaRegular_ttf, BinaryData::MichromaRegular_ttfSize);
    return face;
}

juce::Font fromTypeface (const juce::Typeface::Ptr& face, float height)
{
    if (face == nullptr)
        return juce::Font (juce::FontOptions (height));

    juce::Font result (face);
    result.setHeight (height);
    return result;
}
} // namespace

juce::Font font (float height, bool bold)
{
    return fromTypeface (bold ? uiBold() : uiRegular(), height);
}

juce::Font monoFont (float height, bool bold)
{
    return fromTypeface (bold ? monoBold() : monoRegular(), height);
}

juce::Font displayFont (float height)
{
    return fromTypeface (displayFace(), height);
}

juce::Font trackedFont (float height, float tracking, bool bold)
{
    return fromTypeface (bold ? uiBold() : uiSemiBold(), height)
               .withExtraKerningFactor (tracking / juce::jmax (1.0f, height));
}

void drawTrackedText (juce::Graphics& g, const juce::String& text,
                      juce::Rectangle<int> area, juce::Colour colour,
                      juce::Justification justification,
                      juce::Font fontToUse, float tracking)
{
    g.setColour (colour);
    g.setFont (fontToUse.withExtraKerningFactor (tracking / juce::jmax (1.0f, fontToUse.getHeight())));
    g.drawText (text, area, justification, false);
}

void drawEngravedText (juce::Graphics& g, const juce::String& text,
                       juce::Rectangle<int> area, juce::Colour colour,
                       juce::Justification justification,
                       juce::Font fontToUse, float tracking)
{
    g.setFont (fontToUse.withExtraKerningFactor (tracking / juce::jmax (1.0f, fontToUse.getHeight())));

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText (text, area.translated (0, 1), justification, false);

    g.setColour (colour);
    g.drawText (text, area, justification, false);
}

//==============================================================================
void drawPlate (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize, bool raised)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    g.setGradientFill (juce::ColourGradient::vertical (colour::plateTop, colour::plateBottom, area));
    g.fillPath (path);

    // Top light catch.
    g.setColour (juce::Colours::white.withAlpha (raised ? 0.07f : 0.035f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 0.5f,
                area.getRight() - cornerSize, area.getY() + 0.5f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawLine (area.getX() + cornerSize, area.getBottom() - 0.5f,
                area.getRight() - cornerSize, area.getBottom() - 0.5f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (area.reduced (0.5f), cornerSize, 1.0f);
}

void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff080a0c),
                                                       juce::Colour (0xff171a1f), area));
    g.fillPath (path);

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle (area.reduced (0.5f), cornerSize, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (area.getX() + cornerSize, area.getBottom() - 1.0f,
                area.getRight() - cornerSize, area.getBottom() - 1.0f, 1.0f);
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre.translated (0.0f, 0.8f)));

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff8d949d), centre.y - radius,
                                                       juce::Colour (0xff3a3f46), centre.y + radius));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 0.7f);

    const float angle = (float) (hash ((int) (centre.x * 13.0f + centre.y)) % 360) * 0.0174533f;
    juce::Path slot;
    slot.startNewSubPath (-radius * 0.62f, 0.0f);
    slot.lineTo (radius * 0.62f, 0.0f);
    g.setColour (juce::Colour (0xff191c20));
    g.strokePath (slot, juce::PathStrokeType (1.4f),
                  juce::AffineTransform::rotation (angle).translated (centre));
}

void drawGlow (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour colour, float intensity)
{
    if (intensity <= 0.001f)
        return;

    juce::ColourGradient gradient (colour.withAlpha (0.30f * intensity), centre.x, centre.y,
                                   colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    g.setGradientFill (gradient);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
}

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity)
{
    const auto centre = area.getCentre();

    if (on)
        drawGlow (g, centre, area.getWidth() * 2.3f, colour, intensity);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillEllipse (area.expanded (1.2f));

    g.setGradientFill (juce::ColourGradient (on ? colour.brighter (0.35f)
                                                : juce::Colour (0xff23262b),
                                             centre.x, area.getY(),
                                             on ? colour.darker (0.55f)
                                                : juce::Colour (0xff0c0e10),
                                             centre.x, area.getBottom(), false));
    g.fillEllipse (area);

    g.setColour (juce::Colours::white.withAlpha (on ? 0.55f * intensity : 0.10f));
    g.fillEllipse (area.withSizeKeepingCentre (area.getWidth() * 0.28f,
                                               area.getHeight() * 0.22f)
                       .translated (0.0f, -area.getHeight() * 0.20f));
}

//==============================================================================
void drawWoodFrame (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    g.setGradientFill (juce::ColourGradient::vertical (colour::woodMid, colour::woodDark, bounds));
    g.fillRect (bounds);

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area);

    // Grain: long horizontal streaks with a slow wobble.
    for (int i = 0; i < 240; ++i)
    {
        const float y = bounds.getY() + (float) (hash (i * 7 + (int) bounds.getY()) % 100000) / 100000.0f * bounds.getHeight();
        const float wobble = (float) (hash (i * 31) % 1000) / 1000.0f;
        const float alpha = 0.038f + 0.075f * (float) (hash (i * 17) % 1000) / 1000.0f;

        g.setColour (juce::Colours::black.withAlpha (alpha * 0.9f));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y + wobble * 2.0f - 1.0f, 0.7f);

        g.setColour (colour::woodLight.withAlpha (alpha * 0.7f));
        g.drawLine (bounds.getX(), y + 1.2f, bounds.getRight(), y + 1.2f + wobble * 2.0f, 0.5f);
    }

    // Deep outer edge and a crisp inner bevel.
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.55f),
                                             bounds.getX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getX() + 26.0f, bounds.getY(), false));
    g.fillRect (bounds.withWidth (26.0f));

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRect (bounds.reduced (0.5f), 1.0f);
}

void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    g.setGradientFill (juce::ColourGradient (colour::panelTop.brighter (0.06f),
                                             bounds.getCentreX(), bounds.getY() - 40.0f,
                                             colour::panelBottom,
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRect (bounds);

    // Brushed metal grain: fine horizontal lines at very low contrast.
    for (int i = 0; i < 150; ++i)
    {
        const float y = bounds.getY() + (float) i * bounds.getHeight() / 150.0f;
        g.setColour (juce::Colours::white.withAlpha (0.016f + 0.020f * (float) (hash (i) % 100) / 100.0f));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y, 0.6f);
    }

    // Warm key light from above.
    juce::ColourGradient light (colour::amber.withAlpha (0.035f),
                                bounds.getCentreX(), bounds.getY(),
                                juce::Colours::transparentBlack,
                                bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.65f,
                                false);
    g.setGradientFill (light);
    g.fillRect (bounds);
}

juce::Colour categoryColour (const juce::String& category)
{
    if (category == "BRASS") return juce::Colour (0xfff2a84c);
    if (category == "PAD")   return juce::Colour (0xff77c9c1);
    if (category == "LEAD")  return juce::Colour (0xfff4d06f);
    if (category == "KEY")   return juce::Colour (0xffb7a2e0);
    if (category == "BASS")  return juce::Colour (0xffe3896c);
    if (category == "FX")    return juce::Colour (0xff9fce5a);

    return colour::amber;
}

juce::String formatTime (float seconds)
{
    if (seconds < 0.01f)
        return juce::String (seconds * 1000.0f, 1) + " ms";

    if (seconds < 1.0f)
        return juce::String (seconds * 1000.0f, 0) + " ms";

    return juce::String (seconds, 2) + " s";
}

juce::String formatHz (float hz)
{
    if (hz >= 1000.0f)
        return juce::String (hz / 1000.0f, 2) + " kHz";

    return juce::String (hz, 0) + " Hz";
}
} // namespace cs80::ui
