#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace cs80
{
namespace
{
juce::String hostParameterName (const ParamSpec& spec)
{
    return juce::String (spec.group) + " " + spec.name;
}

int choiceIndex (const ParamSpec& spec)
{
    return (int) std::round (spec.defaultValue);
}
} // namespace

//==============================================================================
Cs80AudioProcessor::Cs80AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CS80E_PARAMETERS", createLayout())
{
    for (const auto& spec : paramSpecs())
        rawParameters.push_back (apvts.getRawParameterValue (spec.id));

    presetParams = defaultParams();
}

juce::AudioProcessorValueTreeState::ParameterLayout Cs80AudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& spec : paramSpecs())
    {
        const juce::String name = hostParameterName (spec);

        if (spec.choices != nullptr)
        {
            juce::StringArray choices;
            choices.addTokens (spec.choices, "|", "");
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { spec.id, 1 }, name, choices, choiceIndex (spec)));
        }
        else
        {
            juce::NormalisableRange<float> range (spec.minValue, spec.maxValue, 0.0f, spec.skew);

            if (spec.steps > 0)
                range.interval = 1.0f;

            auto attributes = juce::AudioParameterFloatAttributes().withLabel (spec.unit);

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { spec.id, 1 }, name, range, spec.defaultValue, attributes));
        }
    }

    return layout;
}

//==============================================================================
void Cs80AudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setParams (snapshotParams());
    keyboardState.reset();

    // The voice path runs oversampled and returns through a linear-phase
    // half-band decimator. Report that delay to the host so offline bounces,
    // PDC and automation land on the same timeline as everything else. The
    // value is derived from the topology, not hard-coded here.
    setLatencySamples (Engine::latencySamples());
}

void Cs80AudioProcessor::releaseResources()
{
    engine.reset();
}

bool Cs80AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

Params Cs80AudioProcessor::snapshotParams() const
{
    Params p;
    const auto& specs = paramSpecs();

    for (size_t i = 0; i < specs.size() && i < rawParameters.size(); ++i)
        setParam (p, specs[i].id, rawParameters[i]->load (std::memory_order_relaxed));

    return p;
}

void Cs80AudioProcessor::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        engine.noteOn (message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        engine.noteOff (message.getNoteNumber());
    }
    else if (message.isPitchWheel())
    {
        engine.pitchBend ((float) (message.getPitchWheelValue() - 8192) / 8192.0f);
    }
    else if (message.isChannelPressure())
    {
        const float value = (float) message.getChannelPressureValue() / 127.0f;
        channelPressureValue.store (value, std::memory_order_relaxed);
        engine.setChannelPressure (value);
    }
    else if (message.isAftertouch())
    {
        engine.setPolyPressure (message.getNoteNumber(), (float) message.getAfterTouchValue() / 127.0f);
    }
    else if (message.isController())
    {
        const int number = message.getControllerNumber();
        const int value = message.getControllerValue();

        if (number == 1)
        {
            const float wheel = (float) value / 127.0f;
            modWheelValue.store (wheel, std::memory_order_relaxed);
            engine.setModWheel (wheel);
        }
        else if (number == 64)
        {
            engine.setSustainPedal (value >= 64);
        }
        else if (number == 120 || number == 123)
        {
            engine.allNotesOff (number == 120);
        }
    }
}

void Cs80AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    if (buffer.getNumChannels() < 2)
        return;

    engine.setParams (snapshotParams());
    engine.setModWheel (modWheelValue.load (std::memory_order_relaxed));
    engine.setRibbon (ribbon.load (std::memory_order_relaxed));

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    int position = 0;

    for (const auto metadata : midi)
    {
        const int eventPosition = juce::jlimit (0, numSamples, metadata.samplePosition);

        if (eventPosition > position)
        {
            engine.process (left + position, right + position, eventPosition - position);
            position = eventPosition;
        }

        handleMidiMessage (metadata.getMessage());
    }

    if (position < numSamples)
        engine.process (left + position, right + position, numSamples - position);
}

//==============================================================================
juce::AudioProcessorEditor* Cs80AudioProcessor::createEditor()
{
    return new Cs80Editor (*this);
}

int Cs80AudioProcessor::getNumPrograms()
{
    return (int) factoryPresets().size();
}

int Cs80AudioProcessor::getCurrentProgram()
{
    return currentPresetIndex();
}

void Cs80AudioProcessor::setCurrentProgram (int index)
{
    loadPreset (index);
}

const juce::String Cs80AudioProcessor::getProgramName (int index)
{
    const auto& presets = factoryPresets();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;

    return {};
}

void Cs80AudioProcessor::changeProgramName (int, const juce::String&) {}

void Cs80AudioProcessor::loadPreset (int index)
{
    const auto& presets = factoryPresets();

    if (presets.empty())
        return;

    index = juce::jlimit (0, (int) presets.size() - 1, index);

    Params p = defaultParams();
    applyPreset (p, presets[(size_t) index]);

    const auto& specs = paramSpecs();

    for (size_t i = 0; i < specs.size(); ++i)
    {
        if (auto* parameter = apvts.getParameter (specs[i].id))
        {
            const float value = getParam (p, specs[i].id);
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        }
    }

    presetParams = p;
    presetIndex.store (index, std::memory_order_relaxed);
    updateHostDisplay();
}

juce::String Cs80AudioProcessor::currentPresetName() const
{
    const auto& presets = factoryPresets();
    const int index = currentPresetIndex();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;

    return "INIT";
}

juce::String Cs80AudioProcessor::currentPresetCategory() const
{
    const auto& presets = factoryPresets();
    const int index = currentPresetIndex();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].category;

    return "";
}

juce::String Cs80AudioProcessor::currentPresetBlurb() const
{
    const auto& presets = factoryPresets();
    const int index = currentPresetIndex();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].blurb;

    return "";
}

bool Cs80AudioProcessor::isPresetEdited() const
{
    const Params current = snapshotParams();

    for (const auto& spec : paramSpecs())
    {
        const float a = getParam (current, spec.id);
        const float b = getParam (presetParams, spec.id);
        const float tolerance = std::max (0.0005f, std::abs (b) * 0.001f);

        if (std::abs (a - b) > tolerance)
            return true;
    }

    return false;
}

void Cs80AudioProcessor::setRibbon (float normalised)
{
    ribbon.store (juce::jlimit (-1.0f, 1.0f, normalised), std::memory_order_relaxed);
}

//==============================================================================
void Cs80AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", currentPresetIndex(), nullptr);
    state.setProperty ("stateVersion", 2, nullptr);

    // Store the baseline the preset was loaded from. Without it a project that
    // was saved with a tweaked factory patch would look unmodified when it was
    // reopened, because the only remaining reference was the live parameters.
    juce::ValueTree baseline ("CS80E_PRESET_BASELINE");

    for (const auto& spec : paramSpecs())
        baseline.setProperty (spec.id, getParam (presetParams, spec.id), nullptr);

    state.removeChild (state.getChildWithName (baseline.getType()), nullptr);
    state.addChild (baseline, -1, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Cs80AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    const int index = (int) state.getProperty ("presetIndex", 0);
    const auto& presets = factoryPresets();

    apvts.replaceState (state);
    presetIndex.store (presets.empty() ? 0 : juce::jlimit (0, (int) presets.size() - 1, index),
                       std::memory_order_relaxed);

    if (auto baseline = state.getChildWithName ("CS80E_PRESET_BASELINE"); baseline.isValid())
    {
        presetParams = defaultParams();

        for (const auto& spec : paramSpecs())
            setParam (presetParams, spec.id,
                      (float) (double) baseline.getProperty (spec.id,
                                                             getParam (presetParams, spec.id)));
    }
    else
    {
        // States written before the baseline existed: reconstruct the factory
        // patch for the saved program, which is the closest available truth.
        presetParams = defaultParams();

        const int savedIndex = currentPresetIndex();
        if (savedIndex >= 0 && savedIndex < (int) presets.size())
            applyPreset (presetParams, presets[(size_t) savedIndex]);
    }
}
} // namespace cs80
