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
    int presetIndex;
    std::vector<int> notes;
    bool browserOpen;
    int browserHover;
    float scale;
    juce::Rectangle<int> crop;
};

void renderShot (const Shot& shot, const juce::File& directory)
{
    cs80::Cs80AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, (int) kSampleRate, 512);
    processor.prepareToPlay (kSampleRate, 512);
    processor.loadPreset (shot.presetIndex);

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
    processor.loadPreset (0); // Blade Brass

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

    const std::vector<Shot> shots = {
        { "01-blade-brass.png", 0, { 36, 48, 51, 55, 62 }, false, 0, 1.35f, {} },
        { "02-vangelis-lead.png", 1, { 60, 63, 67, 70 }, false, 0, 1.20f, {} },
        { "03-neon-pads.png", 2, { 41, 48, 55, 60, 63, 68 }, false, 0, 1.20f, {} },
        { "04-preset-library.png", 9, { 36, 48, 51, 55, 60, 63, 67 }, true, 13, 1.00f, {} },
        { "05-brass-stab.png", 7, { 48, 55, 58, 63 }, false, 0, 1.20f, {} },
        { "06-filter-closeup.png", 4, { 60, 64, 67 }, false, 0, 2.00f,
          juce::Rectangle<int> (552, 164, 558, 392) },
        { "07-ribbon-keybed.png", 0, { 48, 55, 60, 64 }, false, 0, 1.50f,
          juce::Rectangle<int> (40, 700, 1280, 170) },
        { "08-blade-runner.png", 9, { 36, 48, 51, 55, 60, 63 }, false, 0, 1.20f, {} },
    };

    for (const auto& shot : shots)
        renderShot (shot, screenshotDir);

    renderDemoAudio (audioDir.getChildFile ("cs80-demo.wav"));

    std::printf ("Done.\n");
    return 0;
}
