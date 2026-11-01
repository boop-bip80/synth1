// Offline renderer: instantiates the actual plugin editor and paints it into
// PNGs, then renders a demo WAV through the real processor. Runs on CI under
// xvfb (or fully headless, since JUCE only needs a display for windows).

#include "../../src/plugin/PluginEditor.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr double kSampleRate = 48000.0;

juce::File outputDirectory (int argc, char** argv)
{
    auto directory = juce::File::getCurrentWorkingDirectory().getChildFile ("docs");

    if (argc > 1)
        directory = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1]));

    return directory;
}

struct Shot
{
    const char* fileName;
    const char* presetName;
    std::vector<int> notes;
    bool browserOpen;
    int browserHover;
    float scale;
    juce::Rectangle<int> crop;
    int advancedPage = -1;
};

int presetIndexByName (const juce::String& name)
{
    const auto& presets = cs80::factoryPresets();

    for (size_t i = 0; i < presets.size(); ++i)
        if (juce::String (presets[i].name) == name)
            return (int) i;

    std::fprintf (stderr, "Preset not found: %s\n", name.toRawUTF8());
    return 0;
}

bool verifyPresetAndTuningRoundTrip()
{
    cs80::Cs80AudioProcessor original;
    original.loadPreset (0);

    juce::String error;
    const auto scale = juce::String ("CI pentatonic\n5\n200.0\n400.0\n700.0\n900.0\n2/1\n");
    const auto mapping = juce::String ("5\n0\n127\n60\n60\n261.6255653\n5\n0\n1\n2\n3\n4\n");

    if (! original.loadScalaScale (scale, error)
        || ! original.loadScalaMapping (mapping, error))
    {
        std::fprintf (stderr, "Tuning state setup failed: %s\n", error.toRawUTF8());
        return false;
    }

    auto* amount = original.apvts.getParameter (cs80::pid::modAmountIds[0]);
    if (amount == nullptr)
        return false;
    amount->setValueNotifyingHost (amount->convertTo0to1 (0.625f));

    const auto path = juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getNonexistentChildFile ("cs80e-ci-roundtrip", ".cs80e");
    const bool saved = original.saveUserPreset (path, error);

    cs80::Cs80AudioProcessor restored;
    const bool loaded = saved && restored.loadUserPreset (path, error);
    path.deleteFile();

    if (! loaded)
    {
        std::fprintf (stderr, "User preset round-trip failed: %s\n", error.toRawUTF8());
        return false;
    }

    const auto* restoredAmount = restored.apvts.getRawParameterValue (cs80::pid::modAmountIds[0]);
    const auto tuning = restored.getEngine().tuning();
    const bool verified = restoredAmount != nullptr
        && std::abs (restoredAmount->load() - 0.625f) < 0.0001f
        && std::abs (tuning.noteFrequency (61)
                     - 261.6255653 * std::exp2 (200.0 / 1200.0)) < 0.01
        && ! tuning.isEqualTemperament()
        && restored.currentPresetCategory() == "USER";

    std::printf ("  user preset / Scala / mod matrix round-trip: %s\n",
                 verified ? "PASS" : "FAIL");
    return verified;
}

void renderShot (const Shot& shot, const juce::File& directory)
{
    cs80::Cs80AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, (int) kSampleRate, 512);
    processor.prepareToPlay (kSampleRate, 512);
    processor.loadPreset (presetIndexByName (shot.presetName));

    for (int note : shot.notes)
        processor.keyboardState.noteOn (1, note, 0.8f);

    juce::AudioBuffer<float> buffer (2, 512);

    for (int block = 0; block < 40; ++block)
    {
        buffer.clear();
        juce::MidiBuffer empty;
        processor.processBlock (buffer, empty);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (cs80::Cs80Editor::designWidth, cs80::Cs80Editor::designHeight);
    editor->setVisible (true);

    auto* cs80Editor = dynamic_cast<cs80::Cs80Editor*> (editor.get());

    if (cs80Editor != nullptr)
    {
        if (shot.browserOpen)
        {
            cs80Editor->openPresetBrowser();
            cs80Editor->setPresetBrowserHoverForSnapshot (shot.browserHover);
        }

        if (shot.advancedPage >= 0)
        {
            cs80Editor->openAdvancedPanel();
            cs80Editor->setAdvancedPageForSnapshot (shot.advancedPage);
        }

        cs80Editor->refreshForSnapshot();
    }

    const auto area = shot.crop.isEmpty() ? editor->getLocalBounds() : shot.crop;
    auto image = editor->createComponentSnapshot (area, true, shot.scale);

    const auto file = directory.getChildFile (shot.fileName);
    file.deleteFile();

    if (auto stream = file.createOutputStream())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (image, *stream);
    }
    else
    {
        std::fprintf (stderr, "Could not write %s\n", file.getFullPathName().toRawUTF8());
    }

    std::printf ("  wrote %s (%d x %d)\n", shot.fileName, image.getWidth(), image.getHeight());
}

void renderDemoAudio (const juce::File& file)
{
    cs80::Cs80AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, (int) kSampleRate, 512);
    processor.prepareToPlay (kSampleRate, 512);
    processor.loadPreset (presetIndexByName ("Blade Brass"));

    struct NoteEvent
    {
        double time;
        bool on;
        int note;
        float velocity;
    };

    const std::vector<std::vector<int>> chords = {
        { 36, 48, 51, 55, 62 },
        { 44, 48, 51, 55, 60 },
        { 46, 50, 53, 57, 62 },
        { 41, 48, 53, 56, 60 },
        { 43, 50, 55, 59, 62 },
        { 36, 48, 51, 55, 63 },
        { 41, 53, 56, 60, 65 },
    };

    std::vector<NoteEvent> events;
    constexpr double chordLength = 2.9;

    for (size_t i = 0; i < chords.size(); ++i)
    {
        const double start = 0.4 + (double) i * chordLength;
        const float velocity = (i == chords.size() - 1) ? 0.82f : 0.70f;

        for (int note : chords[i])
        {
            events.push_back ({ start, true, note, velocity });
            events.push_back ({ start + chordLength - 0.48, false, note, 0.0f });
        }
    }

    std::sort (events.begin(), events.end(), [] (const NoteEvent& a, const NoteEvent& b)
    {
        return a.time < b.time;
    });

    const double totalSeconds = 0.4 + (double) chords.size() * chordLength + 2.6;
    const int totalSamples = (int) std::ceil (totalSeconds * kSampleRate);
    juce::AudioBuffer<float> output (2, totalSamples);
    output.clear();

    juce::AudioBuffer<float> block (2, 512);
    size_t nextEvent = 0;
    int written = 0;

    while (written < totalSamples)
    {
        const int samples = std::min (512, totalSamples - written);
        const double blockTime = (double) written / kSampleRate;

        while (nextEvent < events.size() && events[nextEvent].time <= blockTime)
        {
            const auto& event = events[nextEvent];

            if (event.on)
                processor.keyboardState.noteOn (1, event.note, event.velocity);
            else
                processor.keyboardState.noteOff (1, event.note, 0.0f);

            ++nextEvent;
        }

        // Slow aftertouch swells on the later chords, which opens the filter
        // and adds the CS-80's characteristic vocal pressure.
        const float swell1 = blockTime > 9.0 && blockTime < 12.6
                               ? (float) std::sin ((float) ((blockTime - 9.0) / 3.6) * juce::MathConstants<float>::pi)
                               : 0.0f;
        const float swell2 = blockTime > 15.6 && blockTime < 21.9
                               ? (float) std::sin ((float) ((blockTime - 15.6) / 6.3) * juce::MathConstants<float>::pi)
                               : 0.0f;
        processor.getEngine().setChannelPressure (juce::jlimit (0.0f, 1.0f,
                                                               std::max (swell1, swell2) * 0.85f));

        block.clear();
        juce::MidiBuffer empty;
        processor.processBlock (block, empty);

        for (int channel = 0; channel < 2; ++channel)
            output.copyFrom (channel, written, block, channel, 0, samples);

        written += samples;
    }

    // Leave a little headroom and remove the very first DC settle.
    output.applyGainRamp (0, std::min (64, totalSamples), 0.0f, 1.0f);

    const float peak = output.getMagnitude (0, output.getNumSamples());

    if (peak > 0.0f)
        output.applyGain (0.89f / peak);

    file.deleteFile();

    if (auto stream = file.createOutputStream())
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            format.createWriterFor (stream.release(), kSampleRate, 2, 16, {}, 0));

        if (writer != nullptr)
        {
            writer->writeFromAudioSampleBuffer (output, 0, output.getNumSamples());
            std::printf ("  wrote %s (%.1f s)\n", file.getFileName().toRawUTF8(), totalSeconds);
        }
    }
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto directory = outputDirectory (argc, argv);
    const auto screenshotDir = directory.getChildFile ("screenshots");
    const auto audioDir = directory.getChildFile ("audio");

    screenshotDir.createDirectory();
    audioDir.createDirectory();

    std::printf ("CS-80E offline renderer\n");

    if (! verifyPresetAndTuningRoundTrip())
        return 1;

    const std::vector<Shot> shots = {
        { "01-blade-brass.png", "Blade Brass", { 36, 48, 51, 55, 62 }, false, 0, 1.20f, {} },
        { "02-vangelis-lead.png", "Vangelis Lead", { 60, 63, 67, 70 }, false, 0, 1.10f, {} },
        { "03-neon-pads.png", "Neon Pads", { 41, 48, 55, 60, 63, 68 }, false, 0, 1.10f, {} },
        { "04-preset-library.png", "Blade Runner", { 36, 48, 51, 55, 60, 63, 67 }, true, 13, 1.00f, {} },
        { "05-brass-stab.png", "Brass Stab", { 48, 55, 58, 63 }, false, 0, 1.10f, {} },
        { "06-filter-closeup.png", "Glass Choir", { 60, 64, 67 }, false, 0, 1.75f,
          juce::Rectangle<int> (440, 166, 770, 418) },
        { "07-vca-sine.png", "Velvet Strings", { 52, 59, 64, 68 }, false, 0, 2.10f,
          juce::Rectangle<int> (1060, 166, 332, 418) },
        { "08-ribbon-keybed.png", "Blade Brass", { 48, 55, 60, 64 }, false, 0, 1.45f,
          juce::Rectangle<int> (40, 570, 1360, 382) },
        { "09-blade-runner.png", "Blade Runner", { 36, 48, 51, 55, 60, 63 }, false, 0, 1.10f, {} },
        { "10-channel-two.png", "Deep Choir", { 52, 59, 64, 68 }, false, 0, 2.00f,
          juce::Rectangle<int> (50, 570, 430, 218) },
        { "11-modulation-matrix.png", "Vangelis Lead", { 60, 64, 67 }, false, 0, 1.00f, {}, 0 },
        { "12-scala-tuning.png", "Glass Choir", { 60, 64, 67 }, false, 0, 1.00f, {}, 1 },
        { "13-user-presets.png", "Blade Brass", { 48, 55, 60 }, false, 0, 1.00f, {}, 2 },
    };

    for (const auto& shot : shots)
        renderShot (shot, screenshotDir);

    renderDemoAudio (audioDir.getChildFile ("cs80-demo.wav"));

    std::printf ("Done.\n");
    return 0;
}
