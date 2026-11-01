#pragma once

#include "Params.h"
#include "Tuning.h"

#include <memory>

namespace cs80
{
/** Eight-voice, dual-oscillator CS-80 style engine.

    The engine has no dependency on JUCE (or anything else) so it can be
    compiled and exercised headlessly by the test suite and the offline
    renderer. All UI/automation communication happens through Params. */
class Engine
{
public:
    static constexpr int kNumVoices = 8;
    static constexpr int kScopeSize = 1024;

    Engine();
    ~Engine();

    Engine (Engine&&) noexcept;
    Engine& operator= (Engine&&) noexcept;
    Engine (const Engine&) = delete;
    Engine& operator= (const Engine&) = delete;

    void prepare (double sampleRate);
    void reset();
    double sampleRate() const;
    bool isPrepared() const;

    /** Delay introduced by the oversampled voice path, in host samples. The
        voice runs at twice the host rate through a linear-phase half-band
        decimator, so the group delay is (taps - 1) / 4 host samples. */
    static int latencySamples() noexcept;

    /** Host-rate multiplier used inside each voice. */
    static double oversamplingFactor() noexcept;

    void setParams (const Params& newParams);
    Params params() const;

    /** Installs a microtuning. Safe to call from the message thread: the table
        is written into an inactive slot and published atomically, so the audio
        thread never sees a half-written scale. */
    void setTuning (const Tuning& newTuning);
    const Tuning& tuning() const;

    /** Note currently assigned to a voice slot, or -1 when the slot is idle.
        Exposed for the UI, tests and host diagnostics. */
    int voiceNote (int index) const;
    bool voiceIsActive (int index) const;

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff (bool immediately = false);
    void setSustainPedal (bool isDown);

    void pitchBend (float normalised);      // -1 .. 1
    void setRibbon (float normalised);      // -1 .. 1
    void setModWheel (float normalised);    // 0 .. 1
    void setChannelPressure (float normalised); // 0 .. 1
    void setPolyPressure (int midiNote, float normalised);

    void process (float* left, float* right, int numSamples);

    int activeVoiceCount() const;
    float voiceActivity (int index) const;
    float outputMeter (int channel) const;
    int readScope (float* destination, int maxSamples) const;
    int readScopeStereo (float* destinationL, float* destinationR, int maxSamples) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace cs80
