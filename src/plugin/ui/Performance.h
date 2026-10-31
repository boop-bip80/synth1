#pragma once

#include "Widgets.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace cs80
{
class Cs80AudioProcessor;
}

namespace cs80::ui
{
//==============================================================================
class ScopeDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ScopeDisplay (const Engine& engineToUse);

    void paint (juce::Graphics&) override;
    void refreshNow() { repaint(); }

private:
    void timerCallback() override { repaint(); }

    const Engine& engine;
};

//==============================================================================
class MeterDisplay : public juce::Component, private juce::Timer
{
public:
    explicit MeterDisplay (const Engine& engineToUse);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }

    const Engine& engine;
};

//==============================================================================
/** Spring-loaded pitch ribbon. Drag away from centre; release to snap back. */
class RibbonStrip : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit RibbonStrip (Cs80AudioProcessor& processorToUse);
    ~RibbonStrip() override = default;

    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void setFromMouse (float x);
    void pushToProcessor();

    Cs80AudioProcessor& processor;
    float value = 0.0f;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RibbonStrip)
};

//==============================================================================
/** Three-octave CS-80 style keybed with velocity from the vertical touch point. */
class Cs80Keyboard : public juce::Component, private juce::MidiKeyboardState::Listener,
                     private juce::AsyncUpdater
{
public:
    explicit Cs80Keyboard (juce::MidiKeyboardState& stateToUse);
    ~Cs80Keyboard() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    struct Key
    {
        int note = 60;
        juce::Rectangle<float> bounds;
        bool black = false;
    };

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleAsyncUpdate() override { repaint(); }

    int noteAt (juce::Point<float> position) const;
    float velocityAt (float y) const;
    void stopCurrentNote();

    juce::MidiKeyboardState& state;
    std::vector<Key> whiteKeys, blackKeys;
    int currentNote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cs80Keyboard)
};
} // namespace cs80::ui
