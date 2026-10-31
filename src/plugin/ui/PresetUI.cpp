#include "PresetUI.h"

#include "../PluginProcessor.h"

namespace cs80::ui
{
namespace
{
constexpr float panelWidth = 900.0f;
constexpr float panelHeight = 604.0f;
constexpr float rowHeight = 45.0f;
constexpr float rowGap = 3.0f;
constexpr float rowsStart = 92.0f;
constexpr float footerHeight = 54.0f;

juce::Rectangle<float> browserPanelBounds (juce::Rectangle<float> full)
{
    return juce::Rectangle<float> (panelWidth, panelHeight).withCentre (full.getCentre());
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
    previous.setBounds (area.getX() + 2, area.getCentreY() - 14, 28, 28);
    next.setBounds (area.getRight() - 30, area.getCentreY() - 14, 28, 28);
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

    auto lcd = bounds.reduced (34.0f, 6.0f);

    drawGlow (g, lcd.getCentre(), lcd.getWidth() * 0.62f, colour::amber, 0.22f);
    drawInsetWell (g, lcd, 3.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1a07),
                                             lcd.getCentreX(), lcd.getY(),
                                             juce::Colour (0xff0f0903),
                                             lcd.getCentreX(), lcd.getBottom(), false));
    g.fillRoundedRectangle (lcd.reduced (1.0f), 3.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f),
                                             lcd.getCentreX(), lcd.getY(),
                                             juce::Colours::transparentWhite,
                                             lcd.getCentreX(), lcd.getY() + 18.0f, false));
    g.fillRoundedRectangle (lcd.reduced (1.0f), 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    for (float y = lcd.getY() + 2.0f; y < lcd.getBottom(); y += 3.0f)
        g.drawLine (lcd.getX() + 2.0f, y, lcd.getRight() - 2.0f, y, 0.55f);

    const int index = processor.currentPresetIndex();
    const auto name = processor.currentPresetName().toUpperCase();
    const auto category = processor.currentPresetCategory();
    const auto categoryTint = categoryColour (category);

    drawTrackedText (g, "PRESET  /  A" + juce::String (index + 1).paddedLeft ('0', 2),
                     { (int) lcd.getX() + 12, (int) lcd.getY() + 5, 220, 9 },
                     colour::amberDim.brighter (0.75f), juce::Justification::centredLeft,
                     trackedFont (6.2f, 1.5f, true), 0.3f);

    const juce::Rectangle<int> badge ((int) lcd.getRight() - 76, (int) lcd.getY() + 5, 64, 13);
    g.setColour (categoryTint.withAlpha (0.16f));
    g.fillRoundedRectangle (badge.toFloat(), 2.0f);
    g.setColour (categoryTint.withAlpha (0.70f));
    g.drawRoundedRectangle (badge.toFloat(), 2.0f, 0.8f);
    drawTrackedText (g, category, badge.reduced (2), categoryTint,
                     juce::Justification::centred, trackedFont (6.6f, 1.1f, true), 0.25f);

    const juce::Rectangle<int> nameArea ((int) lcd.getX() + 12, (int) lcd.getY() + 17,
                                         (int) lcd.getWidth() - 30, 30);

    g.setFont (monoFont (22.0f, true));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawText (name, nameArea.translated (0, 2), juce::Justification::centredLeft, true);
    g.setColour (colour::lcdText);
    g.drawText (name, nameArea, juce::Justification::centredLeft, true);

    if (processor.isPresetEdited())
    {
        g.setColour (colour::amber);
        g.fillEllipse (lcd.getRight() - 15.0f, lcd.getBottom() - 16.0f, 4.5f, 4.5f);
        drawTrackedText (g, "EDIT", { (int) lcd.getRight() - 46, (int) lcd.getBottom() - 21, 28, 8 },
                         colour::amber, juce::Justification::centredRight, monoFont (5.8f, true), 0.2f);
    }

    g.setColour (colour::amber.withAlpha (0.22f));
    g.fillRect (lcd.getX() + 10.0f, lcd.getBottom() - 3.5f, lcd.getWidth() - 20.0f, 1.0f);

    drawTrackedText (g, "BROWSE",
                     { (int) lcd.getX() + 12, (int) lcd.getBottom() - 17, 60, 9 },
                     colour::amberDim.brighter (0.6f), juce::Justification::centredLeft,
                     monoFont (5.8f, true), 0.4f);

    juce::Path chevron;
    chevron.startNewSubPath (lcd.getX() + 55.0f, lcd.getBottom() - 13.0f);
    chevron.lineTo (lcd.getX() + 58.5f, lcd.getBottom() - 9.5f);
    chevron.lineTo (lcd.getX() + 62.0f, lcd.getBottom() - 13.0f);
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
    return { panel.getX() + 28.0f,
             panel.getY() + rowsStart + (float) row * (rowHeight + rowGap),
             panel.getWidth() - 56.0f, rowHeight };
}

int PresetBrowser::rowAt (juce::Point<float> position) const
{
    for (int i = 0; i < (int) factoryPresets().size(); ++i)
        if (rowBounds (i).contains (position))
            return i;

    return -1;
}

int PresetBrowser::closeButtonHit (juce::Point<float> position) const
{
    const auto panel = browserPanelBounds (getLocalBounds().toFloat());
    const juce::Rectangle<float> button (panel.getRight() - 46.0f, panel.getY() + 18.0f, 26.0f, 26.0f);
    return button.contains (position) ? 1 : 0;
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillRect (full);

    const auto panel = browserPanelBounds (full);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (panel.translated (0.0f, 8.0f), 8.0f);

    drawPlate (g, panel, 8.0f);

    // Header.
    g.setColour (colour::amber.withAlpha (0.12f));
    g.fillRoundedRectangle (panel.withHeight (72.0f).reduced (1.0f), 8.0f);

    drawTrackedText (g, "PRESET LIBRARY",
                     { (int) panel.getX() + 30, (int) panel.getY() + 18, 420, 26 },
                     colour::text.brighter (0.12f), juce::Justification::centredLeft,
                     trackedFont (20.0f, 3.0f, true), 0.8f);

    drawTrackedText (g, "FACTORY BANK A  /  " + juce::String ((int) factoryPresets().size()) + " PATCHES  /  CLICK TO AUDITION",
                     { (int) panel.getX() + 31, (int) panel.getY() + 47, 600, 12 },
                     colour::textDim.brighter (0.05f), juce::Justification::centredLeft,
                     monoFont (7.8f), 0.4f);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (panel.getX() + 24.0f, panel.getY() + 78.0f,
                panel.getRight() - 24.0f, panel.getY() + 78.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (panel.getX() + 24.0f, panel.getY() + 79.0f,
                panel.getRight() - 24.0f, panel.getY() + 79.0f, 1.0f);

    // Close button.
    const juce::Rectangle<float> closeButton (panel.getRight() - 46.0f, panel.getY() + 18.0f, 26.0f, 26.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (closeButton, 3.0f);
    g.setColour (colour::amber.withAlpha (0.6f));
    g.drawRoundedRectangle (closeButton, 3.0f, 1.0f);

    juce::Path cross;
    cross.startNewSubPath (closeButton.getCentreX() - 5.0f, closeButton.getCentreY() - 5.0f);
    cross.lineTo (closeButton.getCentreX() + 5.0f, closeButton.getCentreY() + 5.0f);
    cross.startNewSubPath (closeButton.getCentreX() + 5.0f, closeButton.getCentreY() - 5.0f);
    cross.lineTo (closeButton.getCentreX() - 5.0f, closeButton.getCentreY() + 5.0f);
    g.setColour (colour::amberHot);
    g.strokePath (cross, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const int current = processor.currentPresetIndex();

    for (int i = 0; i < (int) factoryPresets().size(); ++i)
    {
        const auto& preset = factoryPresets()[(size_t) i];
        const auto row = rowBounds (i);
        const bool selected = i == current;
        const bool hovered = i == hoverRow;

        if (selected)
        {
            g.setColour (colour::amber.withAlpha (0.10f));
            g.fillRoundedRectangle (row, 4.0f);
            g.setColour (colour::amber.withAlpha (0.65f));
            g.drawRoundedRectangle (row.reduced (0.5f), 4.0f, 1.0f);
        }
        else if (hovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.045f));
            g.fillRoundedRectangle (row, 4.0f);
        }

        if (selected || hovered)
        {
            g.setColour (colour::amber.withAlpha (selected ? 0.9f : 0.4f));
            g.fillRoundedRectangle (row.getX() + 1.0f, row.getY() + 7.0f, 2.0f, row.getHeight() - 14.0f, 1.0f);
        }

        drawTrackedText (g, juce::String (i + 1).paddedLeft ('0', 2),
                         { (int) row.getX() + 14, (int) row.getY(), 34, (int) row.getHeight() },
                         selected ? colour::amberHot : colour::textFaint,
                         juce::Justification::centredLeft, monoFont (9.0f, true), 0.3f);

        g.setFont (trackedFont (13.2f, 1.0f, true));
        g.setColour (selected ? colour::amberHot : (hovered ? colour::text.brighter (0.12f) : colour::text));
        g.drawText (preset.name, row.withTrimmedLeft (52.0f).withWidth (220.0f),
                    juce::Justification::centredLeft, true);

        const auto badgeColour = categoryColour (preset.category);
        g.setColour (badgeColour.withAlpha (0.13f));
        g.fillRoundedRectangle (row.getRight() - 330.0f, row.getCentreY() - 7.0f, 52.0f, 14.0f, 2.0f);
        g.setColour (badgeColour.withAlpha (0.55f));
        g.drawRoundedRectangle (row.getRight() - 330.0f, row.getCentreY() - 7.0f, 52.0f, 14.0f, 2.0f, 0.7f);

        drawTrackedText (g, preset.category,
                         { (int) row.getRight() - 329, (int) row.getCentreY() - 7, 50, 14 },
                         badgeColour, juce::Justification::centred,
                         trackedFont (6.2f, 1.1f, true), 0.25f);

        drawTrackedText (g, preset.blurb,
                         { (int) row.getX() + 290, (int) row.getY(), (int) row.getWidth() - 340,
                           (int) row.getHeight() },
                         selected ? colour::textDim : colour::textFaint.brighter (0.08f),
                         juce::Justification::centredLeft, font (10.0f), 0.1f);
    }

    // Footer.
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (panel.getX() + 24.0f, panel.getBottom() - footerHeight,
                panel.getRight() - 24.0f, panel.getBottom() - footerHeight, 1.0f);

    drawTrackedText (g, "CLICK A PATCH TO AUDITION  /  ESC TO CLOSE  /  ARROWS IN THE LCD STEP THROUGH",
                     { (int) panel.getX() + 30, (int) panel.getBottom() - 38, 600, 14 },
                     colour::textDim.brighter (0.05f), juce::Justification::centredLeft,
                     monoFont (7.6f), 0.3f);

    drawTrackedText (g, "FREEBUFF AUDIO  ·  MODEL CS-80E",
                     { (int) panel.getRight() - 290, (int) panel.getBottom() - 38, 260, 14 },
                     colour::amberDim.brighter (0.3f), juce::Justification::centredRight,
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

    if (row >= 0)
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
