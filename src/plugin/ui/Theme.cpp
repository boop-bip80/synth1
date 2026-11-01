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

float hash01 (int value)
{
    return (float) (hash (value) & 0xffffffu) / 16777215.0f;
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

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawText (text, area.translated (0, -1), justification, false);

    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.drawText (text, area.translated (0, 1), justification, false);

    g.setColour (colour);
    g.drawText (text, area, justification, false);
}

void drawChromeText (juce::Graphics& g, const juce::String& text,
                     juce::Rectangle<int> area, juce::Font fontToUse,
                     juce::Colour topTint, juce::Colour bottomTint,
                     float tracking)
{
    const auto tracked = fontToUse.withExtraKerningFactor (tracking / juce::jmax (1.0f, fontToUse.getHeight()));
    const float textWidth = tracked.getStringWidthFloat (text);
    const float x = (float) area.getCentreX() - textWidth * 0.5f;
    const float y = (float) area.getY() + tracked.getAscent();

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (tracked, text, x, y);

    juce::Path logo;
    glyphs.createPath (logo);

    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillPath (logo, juce::AffineTransform::translation (0.0f, 2.7f));

    juce::ColourGradient chrome (topTint,
                                 (float) area.getCentreX(), (float) area.getY(),
                                 bottomTint,
                                 (float) area.getCentreX(), (float) area.getBottom(), false);
    chrome.addColour (0.34, topTint.brighter (0.18f));
    chrome.addColour (0.52, bottomTint.brighter (0.30f));
    chrome.addColour (0.78, bottomTint.darker (0.18f));
    g.setGradientFill (chrome);
    g.fillPath (logo);

    g.setColour (juce::Colours::white.withAlpha (0.42f));
    g.strokePath (logo, juce::PathStrokeType (0.65f));

    g.setColour (juce::Colours::black.withAlpha (0.24f));
    g.strokePath (logo, juce::PathStrokeType (0.4f),
                  juce::AffineTransform::translation (0.0f, 1.0f));
}

//==============================================================================
void drawPlate (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize, bool raised)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    g.setColour (juce::Colours::black.withAlpha (raised ? 0.70f : 0.38f));
    g.fillRoundedRectangle (area.expanded (raised ? 2.0f : 0.9f).translated (0.0f, raised ? 2.2f : 0.9f),
                            cornerSize + 1.2f);

    juce::ColourGradient face (colour::plateTop, area.getCentreX(), area.getY() - 30.0f,
                               colour::plateBottom, area.getCentreX(), area.getBottom(), false);
    face.addColour (0.26, colour::plateMid.brighter (0.10f));
    face.addColour (0.62, colour::plateMid);
    face.addColour (0.86, juce::Colour (0xff071016));
    g.setGradientFill (face);
    g.fillPath (path);

    // Machined top edge and a soft line of light down the left cheek.
    g.setColour (juce::Colour (0xffd3aa75).withAlpha (raised ? 0.58f : 0.30f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 0.6f,
                area.getRight() - cornerSize, area.getY() + 0.6f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (raised ? 0.075f : 0.035f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 1.9f,
                area.getRight() - cornerSize, area.getY() + 1.9f, 0.8f);

    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawLine (area.getX() + 1.1f, area.getY() + cornerSize,
                area.getX() + 1.1f, area.getBottom() - cornerSize, 0.8f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             area.getCentreX(), area.getBottom() - 30.0f,
                                             juce::Colours::black.withAlpha (0.55f),
                                             area.getCentreX(), area.getBottom(), false));
    g.fillRoundedRectangle (area.reduced (1.0f), cornerSize);

    g.setColour (juce::Colours::black.withAlpha (0.86f));
    g.drawRoundedRectangle (area.reduced (0.7f), cornerSize, 1.0f);
}

void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
{
    juce::Path path;
    path.addRoundedRectangle (area, cornerSize);

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff010303),
                                                       juce::Colour (0xff0f181f), area));
    g.fillPath (path);

    g.setColour (juce::Colours::black.withAlpha (0.96f));
    g.drawRoundedRectangle (area.reduced (0.5f), cornerSize, 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawLine (area.getX() + cornerSize, area.getBottom() - 1.0f,
                area.getRight() - cornerSize, area.getBottom() - 1.0f, 0.9f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.62f),
                                             area.getCentreX(), area.getY(),
                                             juce::Colours::transparentBlack,
                                             area.getCentreX(), area.getY() + area.getHeight() * 0.38f, false));
    g.fillRoundedRectangle (area.reduced (1.0f), cornerSize);
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    g.setColour (juce::Colours::black.withAlpha (0.74f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre.translated (0.0f, 1.3f)));

    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.45f, radius * 2.45f).withCentre (centre), 1.0f);

    juce::ColourGradient metal (juce::Colour (0xffa5a9a7), centre.x, centre.y - radius,
                                juce::Colour (0xff1c2022), centre.x, centre.y + radius, false);
    metal.addColour (0.42, juce::Colour (0xff6f7778));
    g.setGradientFill (metal);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (juce::Colour (0xffe2c08a).withAlpha (0.38f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 0.7f);

    const float angle = (float) (hash ((int) (centre.x * 13.0f + centre.y)) % 360) * 0.0174533f;
    juce::Path slot;
    slot.startNewSubPath (-radius * 0.64f, 0.0f);
    slot.lineTo (radius * 0.64f, 0.0f);
    g.setColour (juce::Colour (0xff090b0c));
    g.strokePath (slot, juce::PathStrokeType (1.5f),
                  juce::AffineTransform::rotation (angle).translated (centre));

    g.setColour (juce::Colours::white.withAlpha (0.32f));
    g.drawLine (centre.x - radius * 0.42f, centre.y - radius * 0.46f,
                centre.x + radius * 0.12f, centre.y - radius * 0.76f, 0.7f);
}

void drawGlow (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour colour, float intensity)
{
    if (intensity <= 0.001f || radius <= 0.0f)
        return;

    juce::ColourGradient gradient (colour.withAlpha (0.32f * intensity), centre.x, centre.y,
                                   colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    gradient.addColour (0.25, colour.withAlpha (0.16f * intensity));
    gradient.addColour (0.52, colour.withAlpha (0.055f * intensity));
    gradient.addColour (0.78, colour.withAlpha (0.012f * intensity));
    g.setGradientFill (gradient);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
}

void drawAurora (juce::Graphics& g, juce::Rectangle<float> area,
                 juce::Colour warm, juce::Colour cool, float intensity)
{
    if (intensity <= 0.001f)
        return;

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area.toNearestInt());

    struct Blob
    {
        float x, y, r;
        juce::Colour colour;
    };

    const Blob blobs[] = {
        { 0.18f, 0.22f, 0.42f, warm },
        { 0.48f, 0.08f, 0.36f, warm },
        { 0.82f, 0.26f, 0.34f, cool },
        { 0.66f, 0.88f, 0.46f, cool },
        { 0.10f, 0.84f, 0.30f, warm },
    };

    for (const auto& blob : blobs)
    {
        const juce::Point<float> centre (area.getX() + area.getWidth() * blob.x,
                                         area.getY() + area.getHeight() * blob.y);
        const float radius = area.getWidth() * blob.r;

        juce::ColourGradient gradient (blob.colour.withAlpha (0.12f * intensity), centre.x, centre.y,
                                       blob.colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
        gradient.addColour (0.34, blob.colour.withAlpha (0.055f * intensity));
        gradient.addColour (0.70, blob.colour.withAlpha (0.012f * intensity));
        g.setGradientFill (gradient);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    }
}

void drawLed (juce::Graphics& g, juce::Rectangle<float> area, bool on,
              juce::Colour colour, float intensity)
{
    const auto centre = area.getCentre();

    if (on)
    {
        drawGlow (g, centre, area.getWidth() * 4.2f, colour, intensity);
        drawGlow (g, centre, area.getWidth() * 1.8f, colour.brighter (0.35f), 0.45f * intensity);
    }

    g.setColour (juce::Colours::black.withAlpha (0.95f));
    g.fillEllipse (area.expanded (2.2f));

    juce::ColourGradient bezel (juce::Colour (0xff686f72), centre.x, area.getY() - 1.5f,
                                juce::Colour (0xff191d20), centre.x, area.getBottom() + 1.5f, false);
    g.setGradientFill (bezel);
    g.fillEllipse (area.expanded (1.4f));

    g.setGradientFill (juce::ColourGradient (on ? colour.brighter (0.62f)
                                                : juce::Colour (0xff2c3136),
                                             centre.x, area.getY(),
                                             on ? colour.darker (0.46f)
                                                : juce::Colour (0xff080a0c),
                                             centre.x, area.getBottom(), false));
    g.fillEllipse (area);

    if (on)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.96f * intensity),
                                                 centre.x, area.getY() + 0.8f,
                                                 juce::Colours::transparentWhite,
                                                 centre.x, area.getCentreY() + 1.6f, false));
        g.fillEllipse (area.reduced (area.getWidth() * 0.20f));

        g.setColour (colour.brighter (0.55f).withAlpha (0.42f * intensity));
        g.drawEllipse (area.reduced (0.4f), 0.7f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillEllipse (area.withSizeKeepingCentre (area.getWidth() * 0.30f,
                                                   area.getHeight() * 0.20f)
                           .translated (0.0f, -area.getHeight() * 0.24f));
    }
}

//==============================================================================
void drawWoodFrame (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    juce::ColourGradient base (colour::woodMid.brighter (0.12f), bounds.getX(), bounds.getY(),
                               colour::woodDark, bounds.getRight(), bounds.getBottom(), false);
    base.addColour (0.34, colour::woodMid);
    base.addColour (0.68, juce::Colour (0xff36180c));
    g.setGradientFill (base);
    g.fillRect (bounds);

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area);

    // Long, hand-rubbed grain with a book-matched figure. The streaks stay low
    // contrast so the cheeks read as varnished wood, not a texture map.
    for (int i = 0; i < 380; ++i)
    {
        const float y = bounds.getY() + hash01 (i * 7 + 11) * bounds.getHeight();
        const float wobble = hash01 (i * 31) * 6.0f - 3.0f;
        const float alpha = 0.020f + 0.055f * hash01 (i * 17);

        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y + wobble, 0.65f);

        g.setColour (colour::woodLight.withAlpha (alpha * 0.55f));
        g.drawLine (bounds.getX(), y + 1.2f, bounds.getRight(), y + 1.2f + wobble, 0.55f);
    }

    for (int i = 0; i < 10; ++i)
    {
        const float y = bounds.getY() + bounds.getHeight() * (0.05f + 0.10f * (float) i);
        juce::ColourGradient figure (juce::Colours::transparentBlack, bounds.getX(), y,
                                     colour::woodHot.withAlpha (0.045f), bounds.getCentreX(), y, false);
        figure.addColour (0.58, colour::woodLight.withAlpha (0.028f));
        g.setGradientFill (figure);
        g.fillRect (juce::Rectangle<float> (bounds.getX(), y, bounds.getWidth() * 0.62f, 11.0f));
    }

    // A couple of slow knots give the grain somewhere to bend.
    for (int i = 0; i < 3; ++i)
    {
        const float cx = bounds.getX() + bounds.getWidth() * (0.12f + 0.38f * (float) i);
        const float cy = bounds.getY() + bounds.getHeight() * (0.24f + 0.31f * (float) i);

        for (int ring = 0; ring < 5; ++ring)
        {
            g.setColour (juce::Colours::black.withAlpha (0.040f - 0.006f * (float) ring));
            g.drawEllipse (juce::Rectangle<float> (16.0f + (float) ring * 13.0f,
                                                   7.0f + (float) ring * 7.0f).withCentre ({ cx, cy }), 0.8f);
        }
    }

    // Warm rail light at the top, cool reflected light at the bottom.
    g.setGradientFill (juce::ColourGradient (colour::woodHot.withAlpha (0.30f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getY() + 30.0f, false));
    g.fillRect (bounds.withHeight (30.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getBottom() - 26.0f,
                                             juce::Colours::black.withAlpha (0.46f),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRect (bounds.withTop (bounds.getBottom() - 26.0f));

    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.drawRect (bounds.reduced (0.5f), 1.2f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.48f),
                                             bounds.getX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getX() + 38.0f, bounds.getY(), false));
    g.fillRect (bounds.withWidth (38.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getRight() - 38.0f, bounds.getY(),
                                             juce::Colours::black.withAlpha (0.42f),
                                             bounds.getRight(), bounds.getY(), false));
    g.fillRect (bounds.withTrimmedLeft (bounds.getWidth() - 38.0f));
}

void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto bounds = area.toFloat();

    juce::ColourGradient face (juce::Colour (0xff334553),
                               bounds.getCentreX(), bounds.getY() - 110.0f,
                               colour::panelBottom,
                               bounds.getCentreX(), bounds.getBottom(), false);
    face.addColour (0.22, juce::Colour (0xff1b2a35));
    face.addColour (0.48, colour::panelMid);
    face.addColour (0.72, juce::Colour (0xff0a1319));
    face.addColour (0.92, juce::Colour (0xff04090d));
    g.setGradientFill (face);
    g.fillRect (bounds);

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area);

    // Fine horizontal brushed finish.
    for (int i = 0; i < 340; ++i)
    {
        const float y = bounds.getY() + (float) i * bounds.getHeight() / 340.0f;
        g.setColour (juce::Colours::white.withAlpha (0.008f + 0.020f * hash01 (i * 3 + 7)));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y, 0.6f);
    }

    // Long machining glints, angled like light across a metal sheet.
    for (int i = 0; i < 11; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (0.02f + 0.095f * (float) i);
        const float width = 120.0f + 90.0f * hash01 (i * 13);

        juce::ColourGradient glint (juce::Colours::transparentBlack, x, bounds.getY(),
                                    juce::Colours::white.withAlpha (0.018f + 0.010f * hash01 (i * 5)),
                                    x + width, bounds.getBottom(), false);
        glint.addColour (0.4, juce::Colours::white.withAlpha (0.012f));
        g.setGradientFill (glint);
        g.fillRect (juce::Rectangle<float> (x, bounds.getY(), width, bounds.getHeight()));
    }

    // Fine dust and machining pinholes.
    for (int i = 0; i < 300; ++i)
    {
        const float x = bounds.getX() + hash01 (i * 23 + 5) * bounds.getWidth();
        const float y = bounds.getY() + hash01 (i * 41 + 9) * bounds.getHeight();
        g.setColour ((i % 3 == 0 ? juce::Colours::white : juce::Colours::black)
                         .withAlpha (0.010f + 0.014f * hash01 (i * 11)));
        g.fillEllipse (x, y, 1.0f, 1.0f);
    }

    // Warm key light above, teal bounce below.
    g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.070f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.55f, false));
    g.fillRect (bounds);

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getBottom() - 190.0f,
                                             colour::aurora.withAlpha (0.038f),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRect (bounds);

    drawAurora (g, bounds, colour::amber, colour::aurora, 0.52f);

    // Edge vignettes.
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.52f),
                                             bounds.getX(), bounds.getY(),
                                             juce::Colours::transparentBlack,
                                             bounds.getX() + 58.0f, bounds.getY(), false));
    g.fillRect (bounds.withWidth (58.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getRight() - 58.0f, bounds.getY(),
                                             juce::Colours::black.withAlpha (0.48f),
                                             bounds.getRight(), bounds.getY(), false));
    g.fillRect (bounds.withTrimmedLeft (bounds.getWidth() - 58.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             bounds.getCentreX(), bounds.getBottom() - 140.0f,
                                             juce::Colours::black.withAlpha (0.34f),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRect (bounds.withTop (bounds.getBottom() - 140.0f));
}

void drawBrassRail (juce::Graphics& g, juce::Rectangle<float> area,
                    bool vertical, float alpha)
{
    juce::ColourGradient metal (juce::Colour (0xff5b3c18), area.getX(), area.getY(),
                                juce::Colour (0xff2a1a0a), area.getX(), area.getBottom(), false);
    metal.addColour (0.22, juce::Colour (0xffd9b073));
    metal.addColour (0.42, juce::Colour (0xfff5dfb1));
    metal.addColour (0.62, juce::Colour (0xff9a6a34));
    metal.addColour (0.82, juce::Colour (0xff5a3a18));

    if (vertical)
    {
        juce::ColourGradient v (juce::Colour (0xff3a2510), area.getX(), area.getY(),
                                juce::Colour (0xff3a2510), area.getRight(), area.getY(), false);
        v.addColour (0.25, juce::Colour (0xffc9a167));
        v.addColour (0.5, juce::Colour (0xfff1d29c));
        v.addColour (0.78, juce::Colour (0xff7b5227));
        g.setGradientFill (v);
    }
    else
    {
        g.setGradientFill (metal);
    }

    g.setOpacity (alpha);
    g.fillRect (area);
    g.setOpacity (1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.42f * alpha));
    if (vertical)
        g.drawLine (area.getX() + area.getWidth() * 0.30f, area.getY(),
                    area.getX() + area.getWidth() * 0.30f, area.getBottom(), 0.7f);
    else
        g.drawLine (area.getX(), area.getY() + area.getHeight() * 0.30f,
                    area.getRight(), area.getY() + area.getHeight() * 0.30f, 0.7f);

    g.setColour (juce::Colours::black.withAlpha (0.65f * alpha));
    if (vertical)
    {
        g.drawLine (area.getX(), area.getY(), area.getX(), area.getBottom(), 0.8f);
        g.drawLine (area.getRight(), area.getY(), area.getRight(), area.getBottom(), 0.8f);
    }
    else
    {
        g.drawLine (area.getX(), area.getY(), area.getRight(), area.getY(), 0.8f);
        g.drawLine (area.getX(), area.getBottom(), area.getRight(), area.getBottom(), 0.8f);
    }
}

void drawVentGrille (juce::Graphics& g, juce::Rectangle<float> area,
                     juce::Colour tint, float alpha)
{
    drawInsetWell (g, area, 2.5f);

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area.toNearestInt());

    const int slots = juce::jmax (4, (int) (area.getWidth() / 9.0f));

    for (int i = 0; i < slots; ++i)
    {
        const float x = area.getX() + 4.0f + (float) i * (area.getWidth() - 8.0f) / (float) slots;

        g.setColour (juce::Colour (0xff010203));
        g.drawLine (x, area.getY() + 4.0f, x, area.getBottom() - 4.0f, 3.0f);

        g.setColour (tint.withAlpha (0.12f * alpha));
        g.drawLine (x + 1.0f, area.getY() + 4.0f, x + 1.0f, area.getBottom() - 4.0f, 0.8f);
    }

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f * alpha),
                                             area.getCentreX(), area.getY(),
                                             juce::Colours::transparentWhite,
                                             area.getCentreX(), area.getCentreY(), false));
    g.fillRect (area);
}

void drawGlassSheen (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
{
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.075f),
                                             area.getX(), area.getY(),
                                             juce::Colours::transparentWhite,
                                             area.getRight(), area.getY() + area.getHeight() * 0.58f, false));
    g.fillRoundedRectangle (area, cornerSize);

    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.drawLine (area.getX() + cornerSize, area.getY() + 1.0f,
                area.getRight() - cornerSize, area.getY() + 1.0f, 0.8f);

    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.drawLine (area.getX() + 10.0f, area.getY() + 5.0f,
                area.getCentreX() + 30.0f, area.getY() + 5.0f, 2.0f);
}

void drawFeltRail (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient feltGradient (juce::Colour (0xff8a2a2e), area.getCentreX(), area.getY(),
                                       juce::Colour (0xff2b0a0d), area.getCentreX(), area.getBottom(), false);
    feltGradient.addColour (0.45, juce::Colour (0xff5f171b));
    g.setGradientFill (feltGradient);
    g.fillRect (area);

    for (int i = 0; i < 34; ++i)
    {
        const float x = area.getX() + (float) i / 34.0f * area.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.14f + 0.10f * hash01 (i * 3)));
        g.drawLine (x, area.getY(), x + 3.0f, area.getBottom(), 0.9f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (area.getX(), area.getY() + 0.5f, area.getRight(), area.getY() + 0.5f, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.60f));
    g.drawLine (area.getX(), area.getBottom() - 0.5f, area.getRight(), area.getBottom() - 0.5f, 1.0f);
}

juce::Colour categoryColour (const juce::String& category)
{
    if (category == "BRASS")   return juce::Colour (0xffffbd72);
    if (category == "PAD")     return juce::Colour (0xff84e0cb);
    if (category == "LEAD")    return juce::Colour (0xffffdf8e);
    if (category == "KEY")     return juce::Colour (0xffcbb4f2);
    if (category == "BASS")    return juce::Colour (0xffffa47c);
    if (category == "FX")      return juce::Colour (0xffb8e886);
    if (category == "ORGAN")   return juce::Colour (0xff9fd6f4);
    if (category == "STRINGS") return juce::Colour (0xffe4c596);
    if (category == "SPLIT")   return juce::Colour (0xffeea9d3);

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
