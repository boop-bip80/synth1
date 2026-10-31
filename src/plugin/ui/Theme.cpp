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

    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.drawText (text, area.translated (0, 1), justification, false);

    g.setColour (colour);
    g.drawText (text, area, justification, false);
}

//==============================================================================
void drawPlate (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize, bool raised)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    juce::ColourGradient face (colour::plateTop, area.getCentreX(), area.getY() - 30.0f,
                               colour::plateBottom, area.getCentreX(), area.getBottom(), false);
    face.addColour (0.55, juce::Colour (0xff0f151b));
    g.setGradientFill (face);
    g.fillPath (path);

    // A warm hairline catches the light along the top, like a machined edge.
    g.setColour (juce::Colour (0xff8e7149).withAlpha (raised ? 0.55f : 0.32f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 0.6f,
                area.getRight() - cornerSize, area.getY() + 0.6f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (raised ? 0.045f : 0.025f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 1.6f,
                area.getRight() - cornerSize, area.getY() + 1.6f, 0.8f);

    // Inner shadow along the bottom.
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             area.getCentreX(), area.getBottom() - 16.0f,
                                             juce::Colours::black.withAlpha (0.55f),
                                             area.getCentreX(), area.getBottom(), false));
    g.fillRoundedRectangle (area.reduced (1.0f), cornerSize);

    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (area.reduced (0.7f), cornerSize, 1.0f);
}

void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff030405),
                                                       juce::Colour (0xff10151a), area));
    g.fillPath (path);

    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (area.reduced (0.5f), cornerSize, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawLine (area.getX() + cornerSize, area.getBottom() - 1.0f,
                area.getRight() - cornerSize, area.getBottom() - 1.0f, 0.8f);
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre.translated (0.0f, 1.1f)));

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff6d747c), centre.y - radius,
                                                       juce::Colour (0xff20252b), centre.y + radius));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 0.7f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.6f, radius * 2.6f).withCentre (centre), 1.0f);

    const float angle = (float) (hash ((int) (centre.x * 13.0f + centre.y)) % 360) * 0.0174533f;
    juce::Path slot;
    slot.startNewSubPath (-radius * 0.62f, 0.0f);
    slot.lineTo (radius * 0.62f, 0.0f);
    g.setColour (juce::Colour (0xff0c0e11));
    g.strokePath (slot, juce::PathStrokeType (1.5f),
                  juce::AffineTransform::rotation (angle).translated (centre));
}

void drawGlow (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour colour, float intensity)
{
    if (intensity <= 0.001f)
        return;

    juce::ColourGradient gradient (colour.withAlpha (0.32f * intensity), centre.x, centre.y,
                                   colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    gradient.addColour (0.35, colour.withAlpha (0.10f * intensity));
    g.setGradientFill (gradient);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
}

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity)
{
    const auto centre = area.getCentre();

    if (on)
        drawGlow (g, centre, area.getWidth() * 3.0f, colour, intensity);

    // Black oxide bezel.
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.fillEllipse (area.expanded (1.6f));

    g.setColour (juce::Colour (0xff2a3037));
    g.drawEllipse (area.expanded (1.2f), 1.0f);

    g.setGradientFill (juce::ColourGradient (on ? colour.brighter (0.45f)
                                                : juce::Colour (0xff252a30),
                                             centre.x, area.getY(),
                                             on ? colour.darker (0.45f)
                                                : juce::Colour (0xff0a0c0e),
                                             centre.x, area.getBottom(), false));
    g.fillEllipse (area);

    if (on)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.85f * intensity),
                                                 centre.x, area.getY() + 1.0f,
                                                 juce::Colours::transparentWhite,
                                                 centre.x, area.getCentreY() + 1.0f, false));
        g.fillEllipse (area.reduced (area.getWidth() * 0.18f));
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.fillEllipse (area.withSizeKeepingCentre (area.getWidth() * 0.3f,
                                                   area.getHeight() * 0.22f)
                           .translated (0.0f, -area.getHeight() * 0.22f));
    }
}

//==============================================================================
void drawWoodFrame (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    g.setGradientFill (juce::ColourGradient::vertical (colour::woodMid, colour::woodDark, bounds));
    g.fillRect (bounds);

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area);

    // Grain: long horizontal streaks with a slow wobble, plus a satin sheen.
    for (int i = 0; i < 300; ++i)
    {
        const float y = bounds.getY() + (float) (hash (i * 7 + (int) bounds.getY()) % 100000) / 100000.0f * bounds.getHeight();
        const float wobble = (float) (hash (i * 31) % 1000) / 1000.0f;
        const float alpha = 0.032f + 0.070f * (float) (hash (i * 17) % 1000) / 1000.0f;

        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y + wobble * 2.4f - 1.2f, 0.7f);

        g.setColour (colour::woodLight.withAlpha (alpha * 0.75f));
        g.drawLine (bounds.getX(), y + 1.2f, bounds.getRight(), y + 1.2f + wobble * 2.4f, 0.55f);
    }

    // A warm highlight across the top rail.
    g.setGradientFill (juce::ColourGradient (colour::woodLight.withAlpha (0.22f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getY() + 18.0f, false));
    g.fillRect (bounds.withHeight (18.0f));

    // Deep shadow where the metal face sits on the wood.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRect (bounds.reduced (0.5f), 1.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.45f),
                                             bounds.getX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getX() + 30.0f, bounds.getY(), false));
    g.fillRect (bounds.withWidth (30.0f));
}

void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    juce::ColourGradient face (colour::panelTop.brighter (0.08f),
                               bounds.getCentreX(), bounds.getY() - 60.0f,
                               colour::panelBottom,
                               bounds.getCentreX(), bounds.getBottom(), false);
    face.addColour (0.42, juce::Colour (0xff121820));
    g.setGradientFill (face);
    g.fillRect (bounds);

    // Fine horizontal brushed grain.
    for (int i = 0; i < 220; ++i)
    {
        const float y = bounds.getY() + (float) i * bounds.getHeight() / 220.0f;
        g.setColour (juce::Colours::white.withAlpha (0.012f + 0.018f * (float) (hash (i) % 100) / 100.0f));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y, 0.6f);
    }

    // Diagonal machining glints, very low contrast.
    for (int i = 0; i < 9; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (0.06f + 0.11f * (float) i);
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                 x, bounds.getY(),
                                                 juce::Colours::white.withAlpha (0.018f),
                                                 x + 120.0f, bounds.getBottom(), false));
        g.fillRect (juce::Rectangle<float> (x, bounds.getY(), 120.0f, bounds.getHeight()));
    }

    // Warm key light falling from above, and a cool bounce at the bottom.
    g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.055f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.62f, false));
    g.fillRect (bounds);

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getBottom() - 120.0f,
                                             juce::Colour (0xff4f6f86).withAlpha (0.035f),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRect (bounds);
}

juce::Colour categoryColour (const juce::String& category)
{
    if (category == "BRASS")   return juce::Colour (0xfff2b26a);
    if (category == "PAD")     return juce::Colour (0xff8ecfbe);
    if (category == "LEAD")    return juce::Colour (0xfff4d17c);
    if (category == "KEY")     return juce::Colour (0xffc1addf);
    if (category == "BASS")    return juce::Colour (0xffe59a76);
    if (category == "FX")      return juce::Colour (0xffa8ce7a);
    if (category == "ORGAN")   return juce::Colour (0xff9fc7df);
    if (category == "STRINGS") return juce::Colour (0xffd7b98c);

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
