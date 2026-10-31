#pragma once

#include "Params.h"

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

    void setParams (const Params& newParams);
    Params params() const;

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
