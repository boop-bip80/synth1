#pragma once

#include "LookAndFeel.h"

#include "../../dsp/Engine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace cs80::ui
{
//==============================================================================
class SectionPanel : public juce::Component
{
public:
    SectionPanel (juce::String sectionTitle, juce::String sectionSubtitle = {});

    void paint (juce::Graphics&) override;

    static constexpr int titleHeight = 30;

private:
    juce::String title, subtitle;
};

//==============================================================================
class ParamFader : public juce::Component
{
public:
    ParamFader (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId);
    ~ParamFader() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setLabelOverride (juce::String newLabel) { label = std::move (newLabel); repaint(); }

    juce::Slider& getSlider() { return slider; }

private:
    juce::String paramId;
    juce::String label;
    juce::RangedAudioParameter* parameter = nullptr;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamFader)
};

//==============================================================================
class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId,
               int knobSize = 44);
    ~ParamKnob() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setLabelOverride (juce::String newLabel) { label = std::move (newLabel); }

private:
    juce::String paramId;
    juce::String label;
    juce::RangedAudioParameter* parameter = nullptr;
    int knobDiameter = 44;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamKnob)
};

//==============================================================================
class ParamChoice : public juce::Component, private juce::Timer
{
public:
    ParamChoice (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::String paramId;
    juce::String label;
    juce::AudioParameterChoice* choiceParameter = nullptr;
    int lastIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamChoice)
};

//==============================================================================
class IconButton : public juce::Button
{
public:
    enum class Icon { prev, next, close, menu, power };

    IconButton (Icon iconToUse, juce::Colour accentColour = colour::amber);

    void setAccentColour (juce::Colour newColour) { accent = newColour; repaint(); }
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

    void setIcon (Icon newIcon) { icon = newIcon; repaint(); }

private:
    Icon icon;
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButton)
};

//==============================================================================
class BrandPlate : public juce::Component
{
public:
    BrandPlate() = default;
    void paint (juce::Graphics&) override;
};

//==============================================================================
/** Eight hardware-style voice LEDs driven by the engine's activity meters. */
class VoiceLedStrip : public juce::Component, private juce::Timer
{
public:
    explicit VoiceLedStrip (const Engine& engineToUse);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    const Engine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceLedStrip)
};
} // namespace cs80::ui
