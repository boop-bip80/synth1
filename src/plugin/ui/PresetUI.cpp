#include "PresetUI.h"

#include "../PluginProcessor.h"

#include <cmath>

namespace cs80::ui
{
namespace
{
constexpr float panelWidth = 1320.0f;
constexpr float panelHeight = 860.0f;
constexpr int columns = 4;
constexpr int rowsPerColumn = 9;
constexpr float rowHeight = 60.0f;
constexpr float rowGap = 7.0f;
constexpr int rowCount = columns * rowsPerColumn;
constexpr float chipsY = 92.0f;
constexpr float chipsHeight = 22.0f;
constexpr float chipGap = 6.0f;
constexpr float rowsStart = 164.0f;
constexpr float footerHeight = 56.0f;

const char* const categoryFilters[] = { "ALL", "BRASS", "LEAD", "PAD", "STRINGS",
                                        "KEY", "BASS", "ORGAN", "FX", "SPLIT" };

juce::Rectangle<float> browserPanelBounds (juce::Rectangle<float> full)
{
    return juce::Rectangle<float> (panelWidth, panelHeight).withCentre (full.getCentre());
}

int presetCount()
{
    return (int) factoryPresets().size();
}

juce::String chipLabel (const juce::String& category)
{
    return category;
}
} // namespace

//==============================================================================
PresetDisplay::PresetDisplay (Cs80AudioProcessor& processorToUse) : processor (processorToUse)
{
    addAndMakeVisible (previous);
    addAndMakeVisible (next);

    previous.setTooltip ("Previous patch");
    next.setTooltip ("Next patch");

    previous.onClick = [this]
    {
        const int count = (int) factoryPresets().size();
        processor.loadPreset ((processor.currentPresetIndex() - 1 + count) % count);
        repaint();
    };

    next.onClick = [this]
    {
        const int count = (int) factoryPresets().size();
        processor.loadPreset ((processor.currentPresetIndex() + 1) % count);
        repaint();
    };

    setTooltip ("Click the glass to open the sound library");
    startTimerHz (12);
}

void PresetDisplay::resized()
{
    const auto area = getLocalBounds();
    previous.setBounds (5, area.getCentreY() - 16, 32, 32);
    next.setBounds (area.getRight() - 37, area.getCentreY() - 16, 32, 32);
}

void PresetDisplay::timerCallback()
{
    const int index = processor.currentPresetIndex();
    const bool edited = processor.isPresetEdited();

    if (index != cachedIndex || edited != cachedEdited)
    {
        cachedIndex = index;
        cachedEdited = edited;
        repaint();
    }
}

void PresetDisplay::mouseDown (const juce::MouseEvent& event)
{
    if (previous.getBounds().contains (event.getPosition())
        || next.getBounds().contains (event.getPosition()))
        return;

    if (onOpenBrowser)
        onOpenBrowser();
}

void PresetDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    drawPlate (g, bounds, 5.0f);

    // Brass bezel around the glass, with four proud screws.
    const juce::Rectangle<float> bezel (bounds.reduced (37.0f, 4.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc8a16b),
                                             bezel.getCentreX(), bezel.getY(),
                                             juce::Colour (0xff3a2813),
                                             bezel.getCentreX(), bezel.getBottom(), false));
    g.fillRoundedRectangle (bezel.expanded (1.5f), 3.5f);
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.drawRoundedRectangle (bezel.expanded (1.5f), 3.5f, 1.0f);

    auto glass = bezel.reduced (2.5f);
    drawInsetWell (g, glass, 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e1a08),
                                             glass.getCentreX(), glass.getY(),
                                             juce::Colour (0xff0a0602),
                                             glass.getCentreX(), glass.getBottom(), false));
    g.fillRoundedRectangle (glass.reduced (1.0f), 2.5f);

    drawGlow (g, { glass.getCentreX(), glass.getCentreY() - 8.0f },
              glass.getWidth() * 0.46f, colour::amber, 0.25f);

    g.setColour (colour::amber.withAlpha (0.18f));
    g.drawRoundedRectangle (glass.reduced (0.8f), 2.5f, 0.8f);

    // Scanlines and a glass reflection.
    g.setColour (juce::Colours::black.withAlpha (0.13f));
    for (float y = glass.getY() + 2.0f; y < glass.getBottom(); y += 3.0f)
        g.drawLine (glass.getX() + 2.0f, y, glass.getRight() - 2.0f, y, 0.55f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.070f),
                                             glass.getCentreX(), glass.getY(),
                                             juce::Colours::transparentWhite,
                                             glass.getCentreX(), glass.getY() + glass.getHeight() * 0.44f, false));
    g.fillRoundedRectangle (glass.reduced (1.0f), 2.5f);

    const int index = processor.currentPresetIndex();
    const auto name = processor.currentPresetName().toUpperCase();
    const auto category = processor.currentPresetCategory();
    const auto blurb = processor.currentPresetBlurb();
    const auto categoryTint = categoryColour (category);

    auto inner = glass.reduced (12.0f, 6.0f);

    // Faint phosphor waveform behind the type.
    {
        juce::Path wave;
        const float y0 = inner.getCentreY() - 2.0f;
        for (int i = 0; i <= 80; ++i)
        {
            const float t = (float) i / 80.0f;
            const float x = inner.getX() + t * inner.getWidth();
            const float y = y0 - std::sin (t * 2.0f * juce::MathConstants<float>::pi * 5.0f)
                                 * inner.getHeight() * 0.20f * (0.4f + 0.6f * t);
            if (i == 0)
                wave.startNewSubPath (x, y);
            else
                wave.lineTo (x, y);
        }
        g.setColour (colour::amber.withAlpha (0.055f));
        g.strokePath (wave, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
    }

    drawTrackedText (g, "PRESET  A" + juce::String (index + 1).paddedLeft ('0', 2)
                            + "   /   " + juce::String (presetCount()).paddedLeft ('0', 2)
                            + "   ·   FACTORY BANK A   ·   SERIES 5",
                     { (int) inner.getX(), (int) inner.getY(), 380, 9 },
                     colour::amberDim.brighter (1.02f), juce::Justification::centredLeft,
                     trackedFont (5.9f, 1.5f, true), 0.3f);

    const juce::Rectangle<int> badge ((int) inner.getRight() - 76, (int) inner.getY() - 1, 74, 11);
    g.setColour (categoryTint.withAlpha (0.14f));
    g.fillRoundedRectangle (badge.toFloat(), 2.0f);
    g.setColour (categoryTint.withAlpha (0.68f));
    g.drawRoundedRectangle (badge.toFloat(), 2.0f, 0.7f);
    drawTrackedText (g, category, badge.reduced (2), categoryTint,
                     juce::Justification::centred, trackedFont (6.2f, 1.1f, true), 0.2f);

    const juce::Rectangle<int> nameArea ((int) inner.getX(), (int) inner.getY() + 9,
                                         (int) inner.getWidth() - 116, 31);

    g.setFont (monoFont (25.0f, true));
    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.drawText (name, nameArea.translated (0, 2), juce::Justification::centredLeft, true);
    g.setColour (colour::lcdText);
    g.drawText (name, nameArea, juce::Justification::centredLeft, true);

    g.setColour (colour::lcdText.withAlpha (0.18f));
    g.fillRect ((float) nameArea.getX(), (float) (nameArea.getY() + nameArea.getHeight() - 3),
                juce::jmin ((float) nameArea.getWidth(), 300.0f), 1.0f);

    drawTrackedText (g, blurb, { (int) inner.getX(), (int) inner.getY() + 40,
                                 (int) inner.getWidth() - 84, 11 },
                     colour::amberDim.brighter (0.88f), juce::Justification::centredLeft,
                     font (8.5f), 0.1f);

    if (processor.isPresetEdited())
    {
        g.setColour (colour::amber);
        g.fillEllipse (inner.getRight() - 8.0f, inner.getBottom() - 10.0f, 4.0f, 4.0f);
        drawGlow (g, { inner.getRight() - 6.0f, inner.getBottom() - 8.0f }, 9.0f, colour::amber, 0.4f);
        drawTrackedText (g, "EDIT", { (int) inner.getRight() - 40, (int) inner.getBottom() - 14, 28, 7 },
                         colour::amber, juce::Justification::centredRight, monoFont (5.2f, true), 0.2f);
    }

    drawTrackedText (g, "BROWSE LIBRARY", { (int) inner.getX(), (int) inner.getBottom() - 13, 108, 8 },
                     colour::amberDim.brighter (1.0f), juce::Justification::centredLeft,
                     monoFont (5.6f, true), 0.4f);

    juce::Path chevron;
    chevron.startNewSubPath (glass.getRight() - 19.0f, glass.getBottom() - 12.0f);
    chevron.lineTo (glass.getRight() - 15.0f, glass.getBottom() - 8.5f);
    chevron.lineTo (glass.getRight() - 11.0f, glass.getBottom() - 12.0f);
    g.setColour (colour::amberDim.brighter (0.65f));
    g.strokePath (chevron, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    const auto* modeParam = processor.apvts.getRawParameterValue (pid::layerMode);
    const int mode = modeParam != nullptr ? (int) std::round (modeParam->load()) : 1;
    const auto modeText = mode == 0 ? juce::String ("SINGLE")
                        : mode == 1 ? juce::String ("DUAL")
                                    : juce::String ("SPLIT");

    drawTrackedText (g, "MODE " + modeText + "  ·  CH I" + (mode == 0 ? "" : " + II"),
                     { (int) inner.getRight() - 226, (int) inner.getBottom() - 13, 216, 8 },
                     colour::amberDim.withAlpha (0.86f), juce::Justification::centredRight,
                     monoFont (5.3f), 0.2f);

    const auto lampY = inner.getBottom() - 15.0f;
    drawLed (g, { inner.getRight() - 248.0f, lampY, 4.0f, 4.0f }, true, colour::amber, 0.55f);

    if (mode != 0)
        drawLed (g, { inner.getRight() - 240.0f, lampY, 4.0f, 4.0f }, true, colour::aurora, 0.55f);

    // Corner screws.
    for (juce::Point<float> screw : { juce::Point<float> (8.0f, 8.0f),
                                      juce::Point<float> (bounds.getRight() - 8.0f, 8.0f),
                                      juce::Point<float> (8.0f, bounds.getBottom() - 8.0f),
                                      juce::Point<float> (bounds.getRight() - 8.0f, bounds.getBottom() - 8.0f) })
        drawScrew (g, screw, 2.4f);
}

//==============================================================================
PresetBrowser::PresetBrowser (Cs80AudioProcessor& processorToUse) : processor (processorToUse)
{
    setWantsKeyboardFocus (true);
    setInterceptsMouseClicks (true, true);
}

void PresetBrowser::resized() {}

std::vector<int> PresetBrowser::visiblePresets() const
{
    std::vector<int> indices;

    for (int i = 0; i < presetCount(); ++i)
    {
        const auto& preset = factoryPresets()[(size_t) i];

        if (filterCategory.isEmpty() || juce::String (preset.category) == filterCategory)
            indices.push_back (i);
    }

    return indices;
}

juce::Rectangle<float> PresetBrowser::rowBounds (int row) const
{
    const auto panel = browserPanelBounds (getLocalBounds().toFloat());
    const float margin = 30.0f;
    const float gap = 18.0f;
    const float columnWidth = (panel.getWidth() - margin * 2.0f - gap * (float) (columns - 1)) / (float) columns;
    const int column = row / rowsPerColumn;
    const int line = row % rowsPerColumn;

    return { panel.getX() + margin + (float) column * (columnWidth + gap),
             panel.getY() + rowsStart + (float) line * (rowHeight + rowGap),
             columnWidth, rowHeight };
}

int PresetBrowser::rowAt (juce::Point<float> position) const
{
    const auto visible = visiblePresets();

    for (int i = 0; i < (int) visible.size() && i < rowCount; ++i)
        if (rowBounds (i).contains (position))
            return visible[(size_t) i];

    return -1;
}

int PresetBrowser::closeButtonHit (juce::Point<float> position) const
{
    const auto panel = browserPanelBounds (getLocalBounds().toFloat());
    const juce::Rectangle<float> button (panel.getRight() - 50.0f, panel.getY() + 20.0f, 30.0f, 30.0f);
    return button.contains (position) ? 1 : 0;
}

juce::Rectangle<float> PresetBrowser::chipBounds (int index) const
{
    const auto panel = browserPanelBounds (getLocalBounds().toFloat());
    float x = panel.getX() + 30.0f;

    for (int i = 0; i < index; ++i)
        x += (i == 0 ? 70.0f : 118.0f) + chipGap;

    return { x, panel.getY() + chipsY, index == 0 ? 70.0f : 118.0f, chipsHeight };
}

int PresetBrowser::chipAt (juce::Point<float> position) const
{
    for (int i = 0; i < (int) (sizeof (categoryFilters) / sizeof (categoryFilters[0])); ++i)
        if (chipBounds (i).contains (position))
            return i;

    return -1;
}

void PresetBrowser::setFilter (const juce::String& category)
{
    filterCategory = category == "ALL" ? juce::String() : category;
    hoverRow = -1;
    repaint();
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.76f));
    g.fillRect (full);

    const auto panel = browserPanelBounds (full);

    // A cool backlight behind the panel, like a lit display in a dark room.
    drawGlow (g, panel.getCentre(), panel.getWidth() * 0.62f, colour::aurora, 0.10f);
    drawGlow (g, { panel.getCentreX(), panel.getY() + 40.0f },
              panel.getWidth() * 0.46f, colour::amber, 0.10f);

    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.fillRoundedRectangle (panel.translated (0.0f, 10.0f), 9.0f);

    drawPlate (g, panel, 9.0f);

    // Header wash.
    g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.13f),
                                             panel.getCentreX(), panel.getY(),
                                             juce::Colours::transparentBlack,
                                             panel.getCentreX(), panel.getY() + 100.0f, false));
    g.fillRoundedRectangle (panel.withHeight (100.0f).reduced (1.0f), 9.0f);

    drawTrackedText (g, "SOUND LIBRARY",
                     { (int) panel.getX() + 30, (int) panel.getY() + 18, 560, 30 },
                     colour::cream.brighter (0.05f), juce::Justification::centredLeft,
                     trackedFont (22.0f, 3.6f, true), 0.8f);

    drawTrackedText (g, "FACTORY BANK A   /   " + juce::String (presetCount())
                            + " PATCHES   /   CLICK TO AUDITION",
                     { (int) panel.getX() + 31, (int) panel.getY() + 49, 780, 12 },
                     colour::textDim.brighter (0.06f), juce::Justification::centredLeft,
                     monoFont (7.4f), 0.4f);

    // Bank plaque.
    const juce::Rectangle<float> plaque (panel.getRight() - 190.0f, panel.getY() + 20.0f, 118.0f, 36.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff161f26),
                                             plaque.getCentreX(), plaque.getY(),
                                             juce::Colour (0xff030608),
                                             plaque.getCentreX(), plaque.getBottom(), false));
    g.fillRoundedRectangle (plaque, 3.0f);
    g.setColour (colour::amber.withAlpha (0.30f));
    g.drawRoundedRectangle (plaque, 3.0f, 1.0f);
    drawTrackedText (g, "BANK A", plaque.toNearestInt(), colour::amberHot,
                     juce::Justification::centred, trackedFont (10.0f, 2.0f, true), 0.4f);

    // Close button.
    const juce::Rectangle<float> closeButton (panel.getRight() - 50.0f, panel.getY() + 20.0f, 30.0f, 30.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (closeButton, 3.0f);
    g.setColour (colour::amber.withAlpha (0.60f));
    g.drawRoundedRectangle (closeButton, 3.0f, 1.0f);

    juce::Path cross;
    cross.startNewSubPath (closeButton.getCentreX() - 5.0f, closeButton.getCentreY() - 5.0f);
    cross.lineTo (closeButton.getCentreX() + 5.0f, closeButton.getCentreY() + 5.0f);
    cross.startNewSubPath (closeButton.getCentreX() + 5.0f, closeButton.getCentreY() - 5.0f);
    cross.lineTo (closeButton.getCentreX() - 5.0f, closeButton.getCentreY() + 5.0f);
    g.setColour (colour::amberHot);
    g.strokePath (cross, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Category filter chips.
    for (int i = 0; i < (int) (sizeof (categoryFilters) / sizeof (categoryFilters[0])); ++i)
    {
        const auto chip = chipBounds (i);
        const auto filter = juce::String (categoryFilters[i]);
        const bool active = filter == "ALL" ? filterCategory.isEmpty() : filter == filterCategory;
        const auto tint = filter == "ALL" ? colour::amber : categoryColour (filter);

        g.setGradientFill (juce::ColourGradient (active ? tint.withAlpha (0.22f)
                                                        : juce::Colour (0xff101820),
                                                 chip.getCentreX(), chip.getY(),
                                                 active ? tint.withAlpha (0.08f)
                                                        : juce::Colour (0xff04070a),
                                                 chip.getCentreX(), chip.getBottom(), false));
        g.fillRoundedRectangle (chip, 3.0f);

        g.setColour ((active ? tint : colour::textFaint).withAlpha (active ? 0.74f : 0.28f));
        g.drawRoundedRectangle (chip.reduced (0.5f), 3.0f, 0.8f);

        if (active)
            drawGlow (g, chip.getCentre(), chip.getWidth() * 0.55f, tint, 0.18f);

        drawTrackedText (g, chipLabel (filter), chip.toNearestInt().reduced (2),
                         active ? tint.brighter (0.25f) : colour::textDim,
                         juce::Justification::centred, trackedFont (6.4f, 1.1f, true), 0.2f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.66f));
    g.drawLine (panel.getX() + 24.0f, panel.getY() + 126.0f,
                panel.getRight() - 24.0f, panel.getY() + 126.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (panel.getX() + 24.0f, panel.getY() + 127.0f,
                panel.getRight() - 24.0f, panel.getY() + 127.0f, 1.0f);

    // Column dividers.
    for (int column = 1; column < columns; ++column)
    {
        const float columnWidth = (panel.getWidth() - 60.0f - 18.0f * (float) (columns - 1)) / (float) columns;
        const float x = panel.getX() + 30.0f + (float) column * columnWidth
                      + (float) (2 * column - 1) * 9.0f;

        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.drawLine (x, panel.getY() + 140.0f, x, panel.getBottom() - footerHeight - 8.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawLine (x + 1.0f, panel.getY() + 140.0f, x + 1.0f, panel.getBottom() - footerHeight - 8.0f, 0.7f);
    }

    const int current = processor.currentPresetIndex();
    const auto visible = visiblePresets();

    for (int i = 0; i < (int) visible.size() && i < rowCount; ++i)
    {
        const int presetIndex = visible[(size_t) i];
        const auto& preset = factoryPresets()[(size_t) presetIndex];
        const auto row = rowBounds (i);
        const bool selected = presetIndex == current;
        const bool hovered = presetIndex == hoverRow;

        if (selected)
        {
            const auto tint = categoryColour (preset.category);
            g.setGradientFill (juce::ColourGradient (tint.withAlpha (0.16f),
                                                     row.getX(), row.getY(),
                                                     tint.withAlpha (0.035f),
                                                     row.getRight(), row.getBottom(), false));
            g.fillRoundedRectangle (row, 4.0f);
            g.setColour (tint.withAlpha (0.72f));
            g.drawRoundedRectangle (row.reduced (0.5f), 4.0f, 1.0f);
            drawGlow (g, { row.getX() + 3.0f, row.getCentreY() }, 24.0f, tint, 0.22f);
        }
        else if (hovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.045f));
            g.fillRoundedRectangle (row, 4.0f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawRoundedRectangle (row.reduced (0.5f), 4.0f, 0.8f);
        }

        if (selected || hovered)
        {
            const auto barTint = categoryColour (preset.category);
            g.setColour (barTint.withAlpha (selected ? 1.0f : 0.50f));
            g.fillRoundedRectangle (row.getX() + 1.0f, row.getY() + 8.0f, 2.4f, row.getHeight() - 16.0f, 1.2f);
        }

        drawTrackedText (g, juce::String (presetIndex + 1).paddedLeft ('0', 2),
                         { (int) row.getX() + 13, (int) row.getY() + 8, 28, 14 },
                         selected ? colour::amberHot : colour::textFaint,
                         juce::Justification::centredLeft, monoFont (9.0f, true), 0.2f);

        g.setFont (trackedFont (13.4f, 0.9f, true));
        g.setColour (selected ? colour::amberHot : (hovered ? colour::cream : colour::text));
        g.drawText (preset.name, row.withTrimmedLeft (46.0f).withTrimmedRight (78.0f).withHeight (29.0f),
                    juce::Justification::centredLeft, true);

        drawTrackedText (g, preset.blurb,
                         { (int) row.getX() + 47, (int) row.getY() + 29,
                           (int) row.getWidth() - 116, 12 },
                         selected ? colour::textDim : colour::textFaint.brighter (0.12f),
                         juce::Justification::centredLeft, font (8.3f), 0.05f);

        const auto badgeColour = categoryColour (preset.category);
        juce::Rectangle<float> badge (row.getRight() - 66.0f, row.getCentreY() - 7.0f, 55.0f, 14.0f);
        g.setColour (badgeColour.withAlpha (0.12f));
        g.fillRoundedRectangle (badge, 2.0f);
        g.setColour (badgeColour.withAlpha (0.58f));
        g.drawRoundedRectangle (badge, 2.0f, 0.7f);

        drawTrackedText (g, preset.category, badge.toNearestInt().reduced (2),
                         badgeColour, juce::Justification::centred,
                         trackedFont (6.0f, 1.0f, true), 0.2f);
    }

    if (visible.empty())
    {
        drawTrackedText (g, "NO PATCHES IN THIS CATEGORY",
                         { (int) panel.getX(), (int) panel.getCentreY() - 10, (int) panel.getWidth(), 20 },
                         colour::textFaint, juce::Justification::centred,
                         trackedFont (12.0f, 2.0f, true), 0.4f);
    }

    // Footer.
    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.drawLine (panel.getX() + 24.0f, panel.getBottom() - footerHeight,
                panel.getRight() - 24.0f, panel.getBottom() - footerHeight, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawLine (panel.getX() + 24.0f, panel.getBottom() - footerHeight + 1.0f,
                panel.getRight() - 24.0f, panel.getBottom() - footerHeight + 1.0f, 0.8f);

    drawTrackedText (g, "CLICK A PATCH TO AUDITION   /   FILTER BY CATEGORY   /   ESC TO CLOSE",
                     { (int) panel.getX() + 30, (int) panel.getBottom() - 33, 820, 14 },
                     colour::textDim.brighter (0.06f), juce::Justification::centredLeft,
                     monoFont (7.4f), 0.3f);

    drawTrackedText (g, "FREEBUFF AUDIO  ·  MODEL CS-80E  ·  SERIES 5",
                     { (int) panel.getRight() - 400, (int) panel.getBottom() - 33, 370, 14 },
                     colour::amberDim.brighter (0.40f), juce::Justification::centredRight,
                     trackedFont (6.4f, 1.5f, true), 0.3f);
}

void PresetBrowser::mouseMove (const juce::MouseEvent& event)
{
    const int row = rowAt (event.position);

    if (row != hoverRow)
    {
        hoverRow = row;
        repaint();
    }
}

void PresetBrowser::mouseExit (const juce::MouseEvent&)
{
    if (hoverRow != -1)
    {
        hoverRow = -1;
        repaint();
    }
}

void PresetBrowser::mouseDown (const juce::MouseEvent& event)
{
    if (closeButtonHit (event.position) == 1)
    {
        close();
        return;
    }

    const auto panel = browserPanelBounds (getLocalBounds().toFloat());

    if (! panel.contains (event.position))
    {
        close();
        return;
    }

    if (const int chip = chipAt (event.position); chip >= 0)
    {
        setFilter (juce::String (categoryFilters[chip]));
        return;
    }

    const int row = rowAt (event.position);

    if (row >= 0 && row < presetCount())
    {
        processor.loadPreset (row);

        if (onPresetLoaded)
            onPresetLoaded();

        repaint();
    }
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        close();
        return true;
    }

    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey)
    {
        const auto visible = visiblePresets();

        if (visible.empty())
            return true;

        const int current = processor.currentPresetIndex();
        int position = 0;

        for (int i = 0; i < (int) visible.size(); ++i)
            if (visible[(size_t) i] == current)
                position = i;

        const int step = key == juce::KeyPress::upKey ? -1 : 1;
        const int next = (position + step + (int) visible.size()) % (int) visible.size();
        processor.loadPreset (visible[(size_t) next]);

        if (onPresetLoaded)
            onPresetLoaded();

        repaint();
        return true;
    }

    return false;
}
} // namespace cs80::ui
