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

    {
        const juce::ScopedLock lock (presetBaselineLock);
        presetParams = p;
    }
    presetIndex.store (index, std::memory_order_relaxed);
    {
        const juce::ScopedLock lock (stateMetadataLock);
        userPresetName.clear();
    }
    presetRevision.fetch_add (1, std::memory_order_relaxed);
    updateHostDisplay();
}

juce::String Cs80AudioProcessor::currentPresetName() const
{
    {
        const juce::ScopedLock lock (stateMetadataLock);
        if (userPresetName.isNotEmpty())
            return userPresetName;
    }

    const auto& presets = factoryPresets();
    const int index = currentPresetIndex();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;

    return "INIT";
}

juce::String Cs80AudioProcessor::currentPresetCategory() const
{
    {
        const juce::ScopedLock lock (stateMetadataLock);
        if (userPresetName.isNotEmpty())
            return "USER";
    }

    const auto& presets = factoryPresets();
    const int index = currentPresetIndex();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].category;

    return "";
}

juce::String Cs80AudioProcessor::currentPresetBlurb() const
{
    {
        const juce::ScopedLock lock (stateMetadataLock);
        if (userPresetName.isNotEmpty())
            return "User patch  /  saved with full plugin state";
    }

    const auto& presets = factoryPresets();
    const int index = currentPresetIndex();

    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].blurb;

    return "";
}

bool Cs80AudioProcessor::isPresetEdited() const
{
    const Params current = snapshotParams();
    const juce::ScopedLock lock (presetBaselineLock);

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

//==============================================================================
juce::File Cs80AudioProcessor::userPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("CS-80E").getChildFile ("Presets");
}

bool Cs80AudioProcessor::saveUserPreset (const juce::File& destination, juce::String& error)
{
    const auto file = destination.withFileExtension (".cs80e");
    const auto name = file.getFileNameWithoutExtension().trim();
    if (name.isEmpty())
    {
        error = "Enter a preset name first.";
        return false;
    }

    juce::String previousName;
    Params previousBaseline;
    {
        const juce::ScopedLock lock (presetBaselineLock);
        previousBaseline = presetParams;
        presetParams = snapshotParams();
    }
    {
        const juce::ScopedLock lock (stateMetadataLock);
        previousName = userPresetName;
        userPresetName = name;
    }

    juce::MemoryBlock data;
    getStateInformation (data);

    if (data.isEmpty() || ! file.replaceWithData (data.getData(), data.getSize()))
    {
        const juce::ScopedLock lock (stateMetadataLock);
        userPresetName = previousName;
        {
            const juce::ScopedLock baselineLock (presetBaselineLock);
            presetParams = previousBaseline;
        }
        error = "Could not write the preset file.";
        return false;
    }

    presetRevision.fetch_add (1, std::memory_order_relaxed);
    error.clear();
    return true;
}

bool Cs80AudioProcessor::loadUserPreset (const juce::File& file, juce::String& error)
{
    if (! file.existsAsFile() || file.getSize() > 4 * 1024 * 1024)
    {
        error = "Preset is missing or exceeds the 4 MB size limit.";
        return false;
    }

    juce::MemoryBlock data;
    if (! file.loadFileAsData (data) || data.isEmpty())
    {
        error = "Could not read this preset file.";
        return false;
    }

    auto xml = getXmlFromBinary (data.getData(), (int) data.getSize());
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
    {
        error = "Not a valid CS-80E preset.";
        return false;
    }

    setStateInformation (data.getData(), (int) data.getSize());
    {
        const juce::ScopedLock lock (stateMetadataLock);
        userPresetName = file.getFileNameWithoutExtension();
    }
    presetRevision.fetch_add (1, std::memory_order_relaxed);
    updateHostDisplay();
    error.clear();
    return true;
}

bool Cs80AudioProcessor::loadScalaScale (const juce::String& text, juce::String& error)
{
    Tuning next;
    std::string reason;
    if (! next.loadScala (text.toStdString(), &reason))
    {
        error = juce::String (reason);
        return false;
    }

    engine.setTuning (next);
    {
        const juce::ScopedLock lock (stateMetadataLock);
        scalaScaleText = text;
        scalaMappingText.clear(); // a new scale invalidates the old mapping
    }
    error.clear();
    return true;
}

bool Cs80AudioProcessor::loadScalaMapping (const juce::String& text, juce::String& error)
{
    juce::String scale;
    {
        const juce::ScopedLock lock (stateMetadataLock);
        scale = scalaScaleText;
    }

    Tuning next;
    std::string reason;
    if (scale.isNotEmpty() && ! next.loadScala (scale.toStdString(), &reason))
    {
        error = juce::String (reason);
        return false;
    }

    if (! next.loadKeyboardMapping (text.toStdString(), &reason))
    {
        error = juce::String (reason);
        return false;
    }

    engine.setTuning (next);
    {
        const juce::ScopedLock lock (stateMetadataLock);
        scalaMappingText = text;
    }
    error.clear();
    return true;
}

void Cs80AudioProcessor::resetMicrotuning()
{
    engine.setTuning (Tuning {});
    const juce::ScopedLock lock (stateMetadataLock);
    scalaScaleText.clear();
    scalaMappingText.clear();
}

juce::String Cs80AudioProcessor::tuningDescription() const
{
    return juce::String (engine.tuning().name());
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
    state.setProperty ("stateVersion", 3, nullptr);

    {
        const juce::ScopedLock lock (stateMetadataLock);
        state.setProperty ("userPresetName", userPresetName, nullptr);
        state.setProperty ("scalaScale", scalaScaleText, nullptr);
        state.setProperty ("scalaMapping", scalaMappingText, nullptr);
    }

    // Store the baseline the preset was loaded from. Without it a project that
    // was saved with a tweaked factory patch would look unmodified when it was
    // reopened, because the only remaining reference was the live parameters.
    juce::ValueTree baseline ("CS80E_PRESET_BASELINE");

    Params baselineSnapshot;
    {
        const juce::ScopedLock lock (presetBaselineLock);
        baselineSnapshot = presetParams;
    }

    for (const auto& spec : paramSpecs())
        baseline.setProperty (spec.id, getParam (baselineSnapshot, spec.id), nullptr);

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

    // State v2 (and earlier) did not contain tuning metadata. Restore such
    // sessions to 12-TET and retain the previous factory-preset fallback.
    const juce::String scaleText = state.getProperty ("scalaScale").toString();
    const juce::String mappingText = state.getProperty ("scalaMapping").toString();
    Tuning restored;
    std::string tuningError;
    bool valid = scaleText.isEmpty() || restored.loadScala (scaleText.toStdString(), &tuningError);
    valid = valid && (mappingText.isEmpty()
                       || restored.loadKeyboardMapping (mappingText.toStdString(), &tuningError));

    engine.setTuning (valid ? restored : Tuning {});
    {
        const juce::ScopedLock lock (stateMetadataLock);
        userPresetName = state.getProperty ("userPresetName").toString();
        scalaScaleText = valid ? scaleText : juce::String();
        scalaMappingText = valid ? mappingText : juce::String();
    }
    presetRevision.fetch_add (1, std::memory_order_relaxed);

    Params restoredBaseline = defaultParams();
    if (auto baseline = state.getChildWithName ("CS80E_PRESET_BASELINE"); baseline.isValid())
    {
        for (const auto& spec : paramSpecs())
            setParam (restoredBaseline, spec.id,
                      (float) (double) baseline.getProperty (spec.id,
                                                             getParam (restoredBaseline, spec.id)));
    }
    else
    {
        // States written before the baseline existed: reconstruct the factory
        // patch for the saved program, which is the closest available truth.
        const int savedIndex = currentPresetIndex();
        if (savedIndex >= 0 && savedIndex < (int) presets.size())
            applyPreset (restoredBaseline, presets[(size_t) savedIndex]);
    }

    {
        const juce::ScopedLock lock (presetBaselineLock);
        presetParams = restoredBaseline;
    }
}
} // namespace cs80
