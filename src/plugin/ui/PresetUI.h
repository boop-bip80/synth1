#pragma once

#include "Widgets.h"

#include <functional>

namespace cs80
{
class Cs80AudioProcessor;
}

namespace cs80::ui
{
//==============================================================================
class PresetDisplay : public juce::Component, private juce::Timer
{
public:
    explicit PresetDisplay (Cs80AudioProcessor& processorToUse);
    ~PresetDisplay() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void()> onOpenBrowser;

private:
    void timerCallback() override;

    Cs80AudioProcessor& processor;
    IconButton previous { IconButton::Icon::prev };
    IconButton next { IconButton::Icon::next };
    int cachedIndex = -1;
    bool cachedEdited = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetDisplay)
};

//==============================================================================
/** Full-panel preset library. Clicking a patch auditions it immediately. */
class PresetBrowser : public juce::Component
{
public:
    explicit PresetBrowser (Cs80AudioProcessor& processorToUse);
    ~PresetBrowser() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<void()> onClose;
    std::function<void()> onPresetLoaded;

    void close() { if (onClose) onClose(); }
    void setHoverRowForSnapshot (int row) { hoverRow = row; repaint(); }

private:
    juce::Rectangle<float> rowBounds (int row) const;
    int rowAt (juce::Point<float> position) const;
    int closeButtonHit (juce::Point<float> position) const;

    Cs80AudioProcessor& processor;
    int hoverRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
} // namespace cs80::ui
