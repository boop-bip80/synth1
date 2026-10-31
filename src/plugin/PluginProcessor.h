#pragma once

#include "../dsp/Engine.h"
#include "../dsp/Params.h"
#include "../dsp/Presets.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <vector>

namespace cs80
{
class Cs80AudioProcessor : public juce::AudioProcessor
{
public:
    Cs80AudioProcessor();
    ~Cs80AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CS-80E"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    void loadPreset (int index);
    int currentPresetIndex() const { return presetIndex.load (std::memory_order_relaxed); }
    juce::String currentPresetName() const;
    juce::String currentPresetCategory() const;
    juce::String currentPresetBlurb() const;
    bool isPresetEdited() const;

    void setRibbon (float normalised);
    float ribbonValue() const { return ribbon.load (std::memory_order_relaxed); }

    Engine& getEngine() { return engine; }
    const Engine& getEngine() const { return engine; }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    Params snapshotParams() const;
    void handleMidiMessage (const juce::MidiMessage& message);

    Engine engine;
    std::vector<std::atomic<float>*> rawParameters;

    Params presetParams;
    std::atomic<int> presetIndex { 0 };
    std::atomic<float> ribbon { 0.0f };
    std::atomic<float> modWheelValue { 0.0f };
    std::atomic<float> channelPressureValue { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cs80AudioProcessor)
};
} // namespace cs80
