#include "PresetUI.h"

#include "../PluginProcessor.h"

namespace cs80::ui
{
namespace
{
constexpr float panelWidth = 1040.0f;
constexpr float panelHeight = 666.0f;
constexpr int columns = 2;
constexpr int rowsPerColumn = 9;
constexpr float rowHeight = 49.0f;
constexpr float rowGap = 3.0f;
constexpr int rowCount = columns * rowsPerColumn;
constexpr float rowsStart = 98.0f;
constexpr float footerHeight = 48.0f;

juce::Rectangle<float> browserPanelBounds (juce::Rectangle<float> full)
{
    return juce::Rectangle<float> (panelWidth, panelHeight).withCentre (full.getCentre());
}

int presetCount()
{
    return (int) factoryPresets().size();
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

    startTimerHz (12);
}

void PresetDisplay::resized()
{
    const auto area = getLocalBounds();
    previous.setBounds (4, area.getCentreY() - 15, 30, 30);
    next.setBounds (area.getRight() - 34, area.getCentreY() - 15, 30, 30);
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

    drawPlate (g, bounds, 4.0f);

    auto glass = bounds.reduced (38.0f, 6.0f);
    drawInsetWell (g, glass, 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b1907),
                                             glass.getCentreX(), glass.getY(),
                                             juce::Colour (0xff0d0803),
                                             glass.getCentreX(), glass.getBottom(), false));
    g.fillRoundedRectangle (glass.reduced (1.0f), 2.5f);

    drawGlow (g, glass.getCentre(), glass.getWidth() * 0.62f, colour::amber, 0.24f);

    g.setColour (colour::amber.withAlpha (0.20f));
    g.drawRoundedRectangle (glass.reduced (0.8f), 2.5f, 0.8f);

    // Scanlines.
    g.setColour (juce::Colours::black.withAlpha (0.13f));
    for (float y = glass.getY() + 2.0f; y < glass.getBottom(); y += 3.0f)
        g.drawLine (glass.getX() + 2.0f, y, glass.getRight() - 2.0f, y, 0.55f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.065f),
                                             glass.getCentreX(), glass.getY(),
                                             juce::Colours::transparentWhite,
                                             glass.getCentreX(), glass.getY() + glass.getHeight() * 0.42f, false));
    g.fillRoundedRectangle (glass.reduced (1.0f), 2.5f);

    const int index = processor.currentPresetIndex();
    const auto name = processor.currentPresetName().toUpperCase();
    const auto category = processor.currentPresetCategory();
    const auto blurb = processor.currentPresetBlurb();
    const auto categoryTint = categoryColour (category);

    auto inner = glass.reduced (11.0f, 6.0f);

    drawTrackedText (g, "PRESET  A" + juce::String (index + 1).paddedLeft ('0', 2)
                            + "   /   " + juce::String ((int) factoryPresets().size()).paddedLeft ('0', 2)
                            + "   ·   FACTORY BANK",
                     { (int) inner.getX(), (int) inner.getY(), 270, 9 },
                     colour::amberDim.brighter (0.95f), juce::Justification::centredLeft,
                     trackedFont (5.9f, 1.4f, true), 0.3f);

    const juce::Rectangle<int> badge ((int) inner.getRight() - 74, (int) inner.getY() - 1, 72, 11);
    g.setColour (categoryTint.withAlpha (0.14f));
    g.fillRoundedRectangle (badge.toFloat(), 2.0f);
    g.setColour (categoryTint.withAlpha (0.65f));
    g.drawRoundedRectangle (badge.toFloat(), 2.0f, 0.7f);
    drawTrackedText (g, category, badge.reduced (2), categoryTint,
                     juce::Justification::centred, trackedFont (6.2f, 1.0f, true), 0.2f);

    const juce::Rectangle<int> nameArea ((int) inner.getX(), (int) inner.getY() + 11,
                                         (int) inner.getWidth() - 110, 30);

    g.setFont (monoFont (24.0f, true));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (name, nameArea.translated (0, 2), juce::Justification::centredLeft, true);
    g.setColour (colour::lcdText);
    g.drawText (name, nameArea, juce::Justification::centredLeft, true);

    drawTrackedText (g, blurb, { (int) inner.getX(), (int) inner.getY() + 41,
                                 (int) inner.getWidth() - 74, 11 },
                     colour::amberDim.brighter (0.78f), juce::Justification::centredLeft,
                     font (8.4f), 0.1f);

    // Edit dot.
    if (processor.isPresetEdited())
    {
        g.setColour (colour::amber);
        g.fillEllipse (inner.getRight() - 8.0f, inner.getBottom() - 10.0f, 4.0f, 4.0f);
        drawTrackedText (g, "EDIT", { (int) inner.getRight() - 38, (int) inner.getBottom() - 14, 26, 7 },
                         colour::amber, juce::Justification::centredRight, monoFont (5.2f, true), 0.2f);
    }

    // Glass reflection across the top left.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.055f),
                                             glass.getX(), glass.getY(),
                                             juce::Colours::transparentWhite,
                                             glass.getRight(), glass.getBottom(), false));
    g.fillRoundedRectangle (glass.reduced (1.0f), 2.5f);

    // Browse hint, with a small chevron.
    drawTrackedText (g, "BROWSE LIBRARY",
                     { (int) inner.getX(), (int) inner.getBottom() - 13, 100, 8 },
                     colour::amberDim.brighter (0.95f), juce::Justification::centredLeft,
                     monoFont (5.5f, true), 0.4f);

    drawTrackedText (g, "OSC · RING · HPF · LPF · VCA",
                     { (int) inner.getRight() - 175, (int) inner.getBottom() - 13, 165, 8 },
                     colour::amberDim.withAlpha (0.72f), juce::Justification::centredRight,
                     monoFont (5.2f), 0.2f);

    juce::Path chevron;
    chevron.startNewSubPath (glass.getRight() - 19.0f, glass.getBottom() - 12.0f);
    chevron.lineTo (glass.getRight() - 15.0f, glass.getBottom() - 8.5f);
    chevron.lineTo (glass.getRight() - 11.0f, glass.getBottom() - 12.0f);
    g.setColour (colour::amberDim.brighter (0.6f));
    g.strokePath (chevron, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

//==============================================================================
PresetBrowser::PresetBrowser (Cs80AudioProcessor& processorToUse) : processor (processorToUse)
{
    setWantsKeyboardFocus (true);
    setInterceptsMouseClicks (true, true);
}

void PresetBrowser::resized() {}

juce::Rectangle<float> PresetBrowser::rowBounds (int row) const
{
    const auto panel = browserPanelBounds (getLocalBounds().toFloat());
    const float gap = 18.0f;
    const float margin = 28.0f;
    const float columnWidth = (panel.getWidth() - margin * 2.0f - gap) * 0.5f;
    const int column = row / rowsPerColumn;
    const int line = row % rowsPerColumn;

    return { panel.getX() + margin + (float) column * (columnWidth + gap),
             panel.getY() + rowsStart + (float) line * (rowHeight + rowGap),
             columnWidth, rowHeight };
}

int PresetBrowser::rowAt (juce::Point<float> position) const
{
    for (int i = 0; i < presetCount() && i < rowCount; ++i)
        if (rowBounds (i).contains (position))
            return i;

    return -1;
}

int PresetBrowser::closeButtonHit (juce::Point<float> position) const
{
    const auto panel = browserPanelBounds (getLocalBounds().toFloat());
    const juce::Rectangle<float> button (panel.getRight() - 48.0f, panel.getY() + 18.0f, 28.0f, 28.0f);
    return button.contains (position) ? 1 : 0;
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.74f));
    g.fillRect (full);

    const auto panel = browserPanelBounds (full);

    g.setColour (juce::Colours::black.withAlpha (0.60f));
    g.fillRoundedRectangle (panel.translated (0.0f, 9.0f), 8.0f);

    drawPlate (g, panel, 8.0f);

    // Header.
    g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.10f),
                                             panel.getCentreX(), panel.getY(),
                                             juce::Colours::transparentBlack,
                                             panel.getCentreX(), panel.getY() + 80.0f, false));
    g.fillRoundedRectangle (panel.withHeight (82.0f).reduced (1.0f), 8.0f);

    drawTrackedText (g, "PRESET LIBRARY",
                     { (int) panel.getX() + 30, (int) panel.getY() + 18, 520, 28 },
                     colour::cream.brighter (0.05f), juce::Justification::centredLeft,
                     trackedFont (21.0f, 3.2f, true), 0.8f);

    drawTrackedText (g, "FACTORY BANK A  /  " + juce::String (presetCount())
                            + " PATCHES  /  CLICK TO AUDITION",
                     { (int) panel.getX() + 31, (int) panel.getY() + 48, 620, 12 },
                     colour::textDim.brighter (0.05f), juce::Justification::centredLeft,
                     monoFont (7.8f), 0.4f);

    // A little "A" plaque on the right of the header.
    const juce::Rectangle<float> plaque (panel.getRight() - 126.0f, panel.getY() + 22.0f, 46.0f, 34.0f);
    g.setColour (juce::Colour (0xff080b0e));
    g.fillRoundedRectangle (plaque, 3.0f);
    g.setColour (colour::amber.withAlpha (0.35f));
    g.drawRoundedRectangle (plaque, 3.0f, 1.0f);
    drawTrackedText (g, "A", plaque.toNearestInt().withTrimmedBottom (12), colour::amberHot,
                     juce::Justification::centred, trackedFont (17.0f, 1.0f, true), 0.2f);
    drawTrackedText (g, "BANK", plaque.toNearestInt().withTrimmedTop (23), colour::textFaint,
                     juce::Justification::centred, monoFont (5.6f), 0.3f);

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawLine (panel.getX() + 24.0f, panel.getY() + 84.0f,
                panel.getRight() - 24.0f, panel.getY() + 84.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (panel.getX() + 24.0f, panel.getY() + 85.0f,
                panel.getRight() - 24.0f, panel.getY() + 85.0f, 1.0f);

    // Close button.
    const juce::Rectangle<float> closeButton (panel.getRight() - 48.0f, panel.getY() + 18.0f, 28.0f, 28.0f);
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.fillRoundedRectangle (closeButton, 3.0f);
    g.setColour (colour::amber.withAlpha (0.55f));
    g.drawRoundedRectangle (closeButton, 3.0f, 1.0f);

    juce::Path cross;
    cross.startNewSubPath (closeButton.getCentreX() - 5.0f, closeButton.getCentreY() - 5.0f);
    cross.lineTo (closeButton.getCentreX() + 5.0f, closeButton.getCentreY() + 5.0f);
    cross.startNewSubPath (closeButton.getCentreX() + 5.0f, closeButton.getCentreY() - 5.0f);
    cross.lineTo (closeButton.getCentreX() - 5.0f, closeButton.getCentreY() + 5.0f);
    g.setColour (colour::amberHot);
    g.strokePath (cross, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Column divider.
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (panel.getCentreX(), panel.getY() + 98.0f,
                panel.getCentreX(), panel.getBottom() - footerHeight - 10.0f, 1.0f);

    const int current = processor.currentPresetIndex();

    for (int i = 0; i < presetCount() && i < rowCount; ++i)
    {
        const auto& preset = factoryPresets()[(size_t) i];
        const auto row = rowBounds (i);
        const bool selected = i == current;
        const bool hovered = i == hoverRow;

        if (selected)
        {
            g.setColour (colour::amber.withAlpha (0.10f));
            g.fillRoundedRectangle (row, 3.0f);
            g.setColour (colour::amber.withAlpha (0.62f));
            g.drawRoundedRectangle (row.reduced (0.5f), 3.0f, 1.0f);
        }
        else if (hovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.042f));
            g.fillRoundedRectangle (row, 3.0f);
        }

        if (selected || hovered)
        {
            g.setColour (colour::amber.withAlpha (selected ? 0.92f : 0.40f));
            g.fillRoundedRectangle (row.getX() + 1.0f, row.getY() + 8.0f, 2.0f, row.getHeight() - 16.0f, 1.0f);
        }

        drawTrackedText (g, juce::String (i + 1).paddedLeft ('0', 2),
                         { (int) row.getX() + 12, (int) row.getY() + 7, 28, 14 },
                         selected ? colour::amberHot : colour::textFaint,
                         juce::Justification::centredLeft, monoFont (8.6f, true), 0.2f);

        g.setFont (trackedFont (12.8f, 0.9f, true));
        g.setColour (selected ? colour::amberHot : (hovered ? colour::cream : colour::text));
        g.drawText (preset.name, row.withTrimmedLeft (44.0f).withTrimmedRight (78.0f).withHeight (26.0f),
                    juce::Justification::centredLeft, true);

        drawTrackedText (g, preset.blurb,
                         { (int) row.getX() + 45, (int) row.getY() + 27,
                           (int) row.getWidth() - 118, 12 },
                         selected ? colour::textDim : colour::textFaint.brighter (0.10f),
                         juce::Justification::centredLeft, font (8.4f), 0.05f);

        const auto badgeColour = categoryColour (preset.category);
        juce::Rectangle<float> badge (row.getRight() - 66.0f, row.getCentreY() - 7.0f, 55.0f, 14.0f);
        g.setColour (badgeColour.withAlpha (0.12f));
        g.fillRoundedRectangle (badge, 2.0f);
        g.setColour (badgeColour.withAlpha (0.55f));
        g.drawRoundedRectangle (badge, 2.0f, 0.7f);

        drawTrackedText (g, preset.category, badge.toNearestInt().reduced (2),
                         badgeColour, juce::Justification::centred,
                         trackedFont (6.0f, 1.0f, true), 0.2f);
    }

    // Footer.
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawLine (panel.getX() + 24.0f, panel.getBottom() - footerHeight,
                panel.getRight() - 24.0f, panel.getBottom() - footerHeight, 1.0f);

    drawTrackedText (g, "CLICK A PATCH TO AUDITION  /  ESC TO CLOSE  /  UP / DOWN STEP THROUGH",
                     { (int) panel.getX() + 30, (int) panel.getBottom() - 32, 680, 14 },
                     colour::textDim.brighter (0.05f), juce::Justification::centredLeft,
                     monoFont (7.4f), 0.3f);

    drawTrackedText (g, "FREEBUFF AUDIO  ·  MODEL CS-80E  ·  TRIBUTE",
                     { (int) panel.getRight() - 330, (int) panel.getBottom() - 32, 300, 14 },
                     colour::amberDim.brighter (0.36f), juce::Justification::centredRight,
                     trackedFont (6.4f, 1.4f, true), 0.3f);
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
        const int count = (int) factoryPresets().size();
        const int step = key == juce::KeyPress::upKey ? -1 : 1;
        processor.loadPreset ((processor.currentPresetIndex() + step + count) % count);

        if (onPresetLoaded)
            onPresetLoaded();

        repaint();
        return true;
    }

    return false;
}
} // namespace cs80::ui
