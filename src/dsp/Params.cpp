#include "Params.h"

#include <algorithm>
#include <cmath>

namespace cs80
{
namespace
{
struct FieldRef
{
    const char* id;
    float Params::*member;
};

// Pointer-to-member keeps this table honest: the test suite round-trips every
// spec through it, so an unmapped parameter fails the build's checks.
const FieldRef kFields[] = {
    { pid::master,     &Params::masterDb },
    { pid::brilliance, &Params::brilliance },
    { pid::ensemble,   &Params::ensemble },
    { pid::space,      &Params::space },
    { pid::ring,       &Params::ring },
    { pid::noise,      &Params::noise },
    { pid::vco1Level,  &Params::vco1Level },
    { pid::vco1Octave, &Params::vco1Octave },
    { pid::vco1Detune, &Params::vco1Detune },
    { pid::vco2Level,  &Params::vco2Level },
    { pid::vco2Pw,     &Params::vco2Pw },
    { pid::vco2Octave, &Params::vco2Octave },
    { pid::vco2Detune, &Params::vco2Detune },
    { pid::hpfCutoff,  &Params::hpfCutoff },
    { pid::hpfRes,     &Params::hpfRes },
    { pid::lpfCutoff,  &Params::lpfCutoff },
    { pid::lpfRes,     &Params::lpfRes },
    { pid::fIl,        &Params::fIl },
    { pid::fAl,        &Params::fAl },
    { pid::fA,         &Params::fA },
    { pid::fD,         &Params::fD },
    { pid::fS,         &Params::fS },
    { pid::fR,         &Params::fR },
    { pid::aA,         &Params::aA },
    { pid::aD,         &Params::aD },
    { pid::aS,         &Params::aS },
    { pid::aR,         &Params::aR },
    { pid::lfoRate,    &Params::lfoRate },
    { pid::lfoWave,    &Params::lfoWave },
    { pid::lfoDelay,   &Params::lfoDelay },
    { pid::lfoPitch,   &Params::lfoPitch },
    { pid::lfoPwm,     &Params::lfoPwm },
    { pid::lfoFilter,  &Params::lfoFilter },
    { pid::lfoAmp,     &Params::lfoAmp },
    { pid::aftertouch, &Params::aftertouch },
    { pid::bendRange,  &Params::bendRange },
    { pid::spread,     &Params::spread },
    { pid::velocity,   &Params::velocity },
};
} // namespace

//==============================================================================
const std::vector<ParamSpec>& paramSpecs()
{
    static const std::vector<ParamSpec> specs = {
        { pid::master, "Master", "OUTPUT", -24.0f, 12.0f, -4.0f, 1.0f, 0, "dB" },
        { pid::brilliance, "Brilliance", "OUTPUT", -1.0f, 1.0f, 0.0f, 1.0f, 0, "" },
        { pid::ensemble, "Ensemble", "OUTPUT", 0.0f, 1.0f, 0.38f, 1.0f, 0, "" },
        { pid::space, "Space", "OUTPUT", 0.0f, 1.0f, 0.16f, 1.0f, 0, "" },

        { pid::ring, "Ring Mod", "RING", 0.0f, 1.0f, 0.05f, 1.0f, 0, "" },
        { pid::noise, "Noise", "RING", 0.0f, 1.0f, 0.015f, 1.0f, 0, "" },

        { pid::vco1Level, "VCO 1", "VCO 1", 0.0f, 1.0f, 0.78f, 1.0f, 0, "" },
        { pid::vco1Octave, "Octave", "VCO 1", -2.0f, 2.0f, 0.0f, 1.0f, 5, "oct" },
        { pid::vco1Detune, "Detune", "VCO 1", -25.0f, 25.0f, -6.0f, 1.0f, 0, "ct" },

        { pid::vco2Level, "VCO 2", "VCO 2", 0.0f, 1.0f, 0.62f, 1.0f, 0, "" },
        { pid::vco2Pw, "Pulse Width", "VCO 2", 0.05f, 0.95f, 0.42f, 1.0f, 0, "" },
        { pid::vco2Octave, "Octave", "VCO 2", -2.0f, 2.0f, 0.0f, 1.0f, 5, "oct" },
        { pid::vco2Detune, "Detune", "VCO 2", -25.0f, 25.0f, 7.0f, 1.0f, 0, "ct" },

        { pid::hpfCutoff, "Cutoff", "HIGH PASS", 20.0f, 4000.0f, 130.0f, 0.38f, 0, "Hz" },
        { pid::hpfRes, "Resonance", "HIGH PASS", 0.0f, 1.0f, 0.12f, 1.0f, 0, "" },
        { pid::lpfCutoff, "Cutoff", "LOW PASS", 40.0f, 16000.0f, 2400.0f, 0.3f, 0, "Hz" },
        { pid::lpfRes, "Resonance", "LOW PASS", 0.0f, 1.0f, 0.28f, 1.0f, 0, "" },

        { pid::fIl, "Initial", "FILTER EG", 0.0f, 1.0f, 0.12f, 1.0f, 0, "" },
        { pid::fAl, "Attack", "FILTER EG", 0.0f, 1.0f, 0.92f, 1.0f, 0, "" },
        { pid::fA, "Attack", "FILTER EG", 0.001f, 8.0f, 0.18f, 0.35f, 0, "s" },
        { pid::fD, "Decay", "FILTER EG", 0.001f, 12.0f, 1.4f, 0.35f, 0, "s" },
        { pid::fS, "Sustain", "FILTER EG", 0.0f, 1.0f, 0.42f, 1.0f, 0, "" },
        { pid::fR, "Release", "FILTER EG", 0.001f, 12.0f, 0.9f, 0.35f, 0, "s" },

        { pid::aA, "Attack", "AMP EG", 0.001f, 8.0f, 0.06f, 0.35f, 0, "s" },
        { pid::aD, "Decay", "AMP EG", 0.001f, 12.0f, 1.2f, 0.35f, 0, "s" },
        { pid::aS, "Sustain", "AMP EG", 0.0f, 1.0f, 0.85f, 1.0f, 0, "" },
        { pid::aR, "Release", "AMP EG", 0.001f, 12.0f, 1.1f, 0.35f, 0, "s" },

        { pid::lfoRate, "Speed", "LFO", 0.05f, 20.0f, 4.6f, 0.4f, 0, "Hz" },
        { pid::lfoWave, "Wave", "LFO", 0.0f, 3.0f, 0.0f, 1.0f, 4, "", "Triangle|Sine|Square|Saw" },
        { pid::lfoDelay, "Delay", "LFO", 0.0f, 5.0f, 0.35f, 1.0f, 0, "s" },
        { pid::lfoPitch, "Pitch", "LFO", 0.0f, 1.0f, 0.12f, 1.0f, 0, "" },
        { pid::lfoPwm, "PWM", "LFO", 0.0f, 1.0f, 0.18f, 1.0f, 0, "" },
        { pid::lfoFilter, "Filter", "LFO", 0.0f, 1.0f, 0.0f, 1.0f, 0, "" },
        { pid::lfoAmp, "Tremolo", "LFO", 0.0f, 1.0f, 0.0f, 1.0f, 0, "" },

        { pid::aftertouch, "Aftertouch", "PERFORMANCE", 0.0f, 1.0f, 0.55f, 1.0f, 0, "" },
        { pid::bendRange, "Bend Range", "PERFORMANCE", 0.0f, 24.0f, 2.0f, 1.0f, 25, "st" },
        { pid::spread, "Spread", "PERFORMANCE", 0.0f, 1.0f, 0.45f, 1.0f, 0, "" },
        { pid::velocity, "Velocity", "PERFORMANCE", 0.0f, 1.0f, 0.45f, 1.0f, 0, "" },
    };

    return specs;
}

const ParamSpec* findSpec (const std::string& id)
{
    for (const auto& s : paramSpecs())
        if (id == s.id)
            return &s;

    return nullptr;
}

int specIndex (const std::string& id)
{
    const auto& specs = paramSpecs();

    for (size_t i = 0; i < specs.size(); ++i)
        if (id == specs[i].id)
            return (int) i;

    return -1;
}

//==============================================================================
Params defaultParams()
{
    Params p {}; // start from the explicit field defaults above

    for (const auto& spec : paramSpecs())
        setParam (p, spec.id, spec.defaultValue);

    return p;
}

bool isKnownParam (const std::string& id)
{
    return findSpec (id) != nullptr;
}

float getParam (const Params& params, const std::string& id)
{
    for (const auto& f : kFields)
        if (id == f.id)
            return params.*(f.member);

    return 0.0f;
}

void setParam (Params& params, const std::string& id, float value)
{
    for (const auto& f : kFields)
    {
        if (id == f.id)
        {
            params.*(f.member) = clampToSpec (id, value);
            return;
        }
    }
}

float clampToSpec (const std::string& id, float value)
{
    if (const auto* spec = findSpec (id))
    {
        if (spec->steps > 0 && spec->id == pid::vco1Octave)
            value = std::round (value);
        else if (spec->steps > 0 && spec->id == pid::vco2Octave)
            value = std::round (value);

        return std::clamp (value, spec->minValue, spec->maxValue);
    }

    return value;
}
} // namespace cs80
