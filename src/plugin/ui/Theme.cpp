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

    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawText (text, area.translated (0, -1), justification, false);

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.drawText (text, area.translated (0, 1), justification, false);

    g.setColour (colour);
    g.drawText (text, area, justification, false);
}

//==============================================================================
void drawPlate (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize, bool raised)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    // The card sits on the faceplate with a soft, tight shadow.
    g.setColour (juce::Colours::black.withAlpha (raised ? 0.62f : 0.35f));
    g.fillRoundedRectangle (area.expanded (raised ? 1.6f : 0.8f).translated (0.0f, raised ? 1.8f : 0.8f),
                            cornerSize + 1.0f);

    juce::ColourGradient face (colour::plateTop, area.getCentreX(), area.getY() - 24.0f,
                               colour::plateBottom, area.getCentreX(), area.getBottom(), false);
    face.addColour (0.36, colour::plateMid);
    face.addColour (0.78, juce::Colour (0xff0c141b));
    g.setGradientFill (face);
    g.fillPath (path);

    // A warm machined edge catching the room light.
    g.setColour (juce::Colour (0xffc39a67).withAlpha (raised ? 0.48f : 0.26f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 0.6f,
                area.getRight() - cornerSize, area.getY() + 0.6f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (raised ? 0.055f : 0.028f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 1.8f,
                area.getRight() - cornerSize, area.getY() + 1.8f, 0.8f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (area.getX() + 1.0f, area.getY() + cornerSize,
                area.getX() + 1.0f, area.getBottom() - cornerSize, 0.8f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             area.getCentreX(), area.getBottom() - 24.0f,
                                             juce::Colours::black.withAlpha (0.52f),
                                             area.getCentreX(), area.getBottom(), false));
    g.fillRoundedRectangle (area.reduced (1.0f), cornerSize);

    g.setColour (juce::Colours::black.withAlpha (0.84f));
    g.drawRoundedRectangle (area.reduced (0.7f), cornerSize, 1.0f);
}

void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff020404),
                                                       juce::Colour (0xff0d141a), area));
    g.fillPath (path);

    g.setColour (juce::Colours::black.withAlpha (0.94f));
    g.drawRoundedRectangle (area.reduced (0.5f), cornerSize, 1.1f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (area.getX() + cornerSize, area.getBottom() - 1.0f,
                area.getRight() - cornerSize, area.getBottom() - 1.0f, 0.9f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.55f),
                                             area.getCentreX(), area.getY(),
                                             juce::Colours::transparentBlack,
                                             area.getCentreX(), area.getY() + area.getHeight() * 0.35f, false));
    g.fillRoundedRectangle (area.reduced (1.0f), cornerSize);
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre.translated (0.0f, 1.2f)));

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.35f, radius * 2.35f).withCentre (centre), 1.0f);

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff8a8e8d), centre.y - radius,
                                                       juce::Colour (0xff26292b), centre.y + radius));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (juce::Colour (0xffd2b483).withAlpha (0.34f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 0.7f);

    const float angle = (float) (hash ((int) (centre.x * 13.0f + centre.y)) % 360) * 0.0174533f;
    juce::Path slot;
    slot.startNewSubPath (-radius * 0.62f, 0.0f);
    slot.lineTo (radius * 0.62f, 0.0f);
    g.setColour (juce::Colour (0xff0b0d0e));
    g.strokePath (slot, juce::PathStrokeType (1.4f),
                  juce::AffineTransform::rotation (angle).translated (centre));

    g.setColour (juce::Colours::white.withAlpha (0.30f));
    g.drawLine (centre.x - radius * 0.4f, centre.y - radius * 0.45f,
                centre.x + radius * 0.15f, centre.y - radius * 0.75f, 0.7f);
}

void drawGlow (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour colour, float intensity)
{
    if (intensity <= 0.001f)
        return;

    juce::ColourGradient gradient (colour.withAlpha (0.30f * intensity), centre.x, centre.y,
                                   colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    gradient.addColour (0.32, colour.withAlpha (0.10f * intensity));
    gradient.addColour (0.66, colour.withAlpha (0.025f * intensity));
    g.setGradientFill (gradient);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
}

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity)
{
    const auto centre = area.getCentre();

    if (on)
        drawGlow (g, centre, area.getWidth() * 3.5f, colour, intensity);

    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.fillEllipse (area.expanded (1.8f));

    g.setColour (juce::Colour (0xff3a4147));
    g.drawEllipse (area.expanded (1.3f), 0.9f);

    g.setGradientFill (juce::ColourGradient (on ? colour.brighter (0.55f)
                                                : juce::Colour (0xff2b3036),
                                             centre.x, area.getY(),
                                             on ? colour.darker (0.48f)
                                                : juce::Colour (0xff090b0d),
                                             centre.x, area.getBottom(), false));
    g.fillEllipse (area);

    if (on)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.92f * intensity),
                                                 centre.x, area.getY() + 1.0f,
                                                 juce::Colours::transparentWhite,
                                                 centre.x, area.getCentreY() + 1.4f, false));
        g.fillEllipse (area.reduced (area.getWidth() * 0.20f));
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillEllipse (area.withSizeKeepingCentre (area.getWidth() * 0.30f,
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

    // Long, hand-rubbed grain with a gentle figure. The streaks are deliberately
    // low contrast so the wood reads as material rather than a texture map.
    for (int i = 0; i < 260; ++i)
    {
        const float y = bounds.getY() + (float) (hash (i * 7 + (int) bounds.getHeight()) % 100000) / 100000.0f * bounds.getHeight();
        const float wobble = (float) (hash (i * 31) % 1000) / 1000.0f;
        const float alpha = 0.026f + 0.062f * (float) (hash (i * 17) % 1000) / 1000.0f;

        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y + wobble * 3.0f - 1.5f, 0.7f);

        g.setColour (colour::woodLight.withAlpha (alpha * 0.60f));
        g.drawLine (bounds.getX(), y + 1.1f, bounds.getRight(), y + 1.1f + wobble * 3.0f, 0.55f);
    }

    // Broad figure across the cheeks.
    for (int i = 0; i < 8; ++i)
    {
        const float y = bounds.getY() + bounds.getHeight() * (0.06f + 0.12f * (float) i);
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, bounds.getX(), y,
                                                 colour::woodHot.withAlpha (0.035f), bounds.getCentreX(), y, false));
        g.fillRect (juce::Rectangle<float> (bounds.getX(), y, bounds.getWidth() * 0.55f, 9.0f));
    }

    // Warm rail light and the shadow under the metal face.
    g.setGradientFill (juce::ColourGradient (colour::woodHot.withAlpha (0.26f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getY() + 26.0f, false));
    g.fillRect (bounds.withHeight (26.0f));

    g.setColour (juce::Colours::black.withAlpha (0.52f));
    g.drawRect (bounds.reduced (0.5f), 1.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.42f),
                                             bounds.getX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getX() + 34.0f, bounds.getY(), false));
    g.fillRect (bounds.withWidth (34.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getRight() - 34.0f, bounds.getY(),
                                             juce::Colours::black.withAlpha (0.35f),
                                             bounds.getRight(), bounds.getY(), false));
    g.fillRect (bounds.withTrimmedLeft (bounds.getWidth() - 34.0f));
}

void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    juce::ColourGradient face (juce::Colour (0xff354553),
                               bounds.getCentreX(), bounds.getY() - 100.0f,
                               colour::panelBottom,
                               bounds.getCentreX(), bounds.getBottom(), false);
    face.addColour (0.30, juce::Colour (0xff1c2833));
    face.addColour (0.62, juce::Colour (0xff111a22));
    g.setGradientFill (face);
    g.fillRect (bounds);

    // Fine horizontal brushed finish.
    for (int i = 0; i < 260; ++i)
    {
        const float y = bounds.getY() + (float) i * bounds.getHeight() / 260.0f;
        g.setColour (juce::Colours::white.withAlpha (0.010f + 0.017f * (float) (hash (i) % 100) / 100.0f));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y, 0.6f);
    }

    // Long machining glints, angled like light across a metal sheet.
    for (int i = 0; i < 8; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (0.04f + 0.13f * (float) i);
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                 x, bounds.getY(),
                                                 juce::Colours::white.withAlpha (0.020f),
                                                 x + 150.0f, bounds.getBottom(), false));
        g.fillRect (juce::Rectangle<float> (x, bounds.getY(), 150.0f, bounds.getHeight()));
    }

    // Warm key light above, a cool bounce below.
    g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.060f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.60f, false));
    g.fillRect (bounds);

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getBottom() - 150.0f,
                                             juce::Colour (0xff4e7d96).withAlpha (0.045f),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRect (bounds);
}

void drawGlassSheen (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
{
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.060f),
                                             area.getX(), area.getY(),
                                             juce::Colours::transparentWhite,
                                             area.getRight(), area.getY() + area.getHeight() * 0.55f, false));
    g.fillRoundedRectangle (area, cornerSize);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 1.0f,
                area.getRight() - cornerSize, area.getY() + 1.0f, 0.8f);
}

void drawFeltRail (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a2528), area.getCentreX(), area.getY(),
                                             juce::Colour (0xff2b0b0e), area.getCentreX(), area.getBottom(), false));
    g.fillRect (area);

    for (int i = 0; i < 26; ++i)
    {
        const float x = area.getX() + (float) i / 26.0f * area.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.drawLine (x, area.getY(), x + 3.0f, area.getBottom(), 0.9f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawLine (area.getX(), area.getY(), area.getRight(), area.getY(), 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (area.getX(), area.getBottom() - 0.5f, area.getRight(), area.getBottom() - 0.5f, 0.8f);
}

juce::Colour categoryColour (const juce::String& category)
{
    if (category == "BRASS")   return juce::Colour (0xfff0ba72);
    if (category == "PAD")     return juce::Colour (0xff8fd3c1);
    if (category == "LEAD")    return juce::Colour (0xfff4d782);
    if (category == "KEY")     return juce::Colour (0xffc5b0e8);
    if (category == "BASS")    return juce::Colour (0xffe7a07b);
    if (category == "FX")      return juce::Colour (0xffaed47f);
    if (category == "ORGAN")   return juce::Colour (0xff9fcbe9);
    if (category == "STRINGS") return juce::Colour (0xffdcbf8f);
    if (category == "SPLIT")   return juce::Colour (0xffe4a8ce);

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
