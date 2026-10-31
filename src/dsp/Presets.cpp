#include "Presets.h"

namespace cs80
{
const std::vector<Preset>& factoryPresets()
{

    static const std::vector<Preset> presets = {
        { "Blade Brass", "BRASS", "Wide, slow-blooming brass that leans on the ensemble.", {} },

        { "Vangelis Lead", "LEAD", "Bright, vocal lead with aftertouch vibrato.", {
            { pid::lpfCutoff, 5200.0f }, { pid::lpfRes, 0.45f }, { pid::vco2Pw, 0.30f },
            { pid::ensemble, 0.50f }, { pid::space, 0.30f }, { pid::aftertouch, 0.80f },
            { pid::aA, 0.02f }, { pid::fA, 0.02f }, { pid::fD, 0.90f }, { pid::fS, 0.60f },
            { pid::fAl, 1.0f }, { pid::lfoPitch, 0.25f }, { pid::lfoRate, 5.2f },
            { pid::vco1Detune, -12.0f }, { pid::vco2Detune, 12.0f }, { pid::master, -6.0f },
        } },

        { "Neon Pads", "PAD", "Huge detuned pad with a very slow filter tide.", {
            { pid::aA, 0.90f }, { pid::aD, 2.50f }, { pid::aS, 0.80f }, { pid::aR, 3.50f },
            { pid::fA, 1.20f }, { pid::fD, 3.00f }, { pid::fS, 0.70f }, { pid::fR, 2.50f },
            { pid::ensemble, 0.75f }, { pid::space, 0.42f }, { pid::lpfCutoff, 1800.0f },
            { pid::spread, 0.85f }, { pid::lfoPwm, 0.40f }, { pid::lfoRate, 0.35f },
            { pid::vco1Detune, -14.0f }, { pid::vco2Detune, 14.0f }, { pid::master, -7.0f },
        } },

        { "Cinematic Strings", "PAD", "Soft string machine, high-passed and airborne.", {
            { pid::hpfCutoff, 260.0f }, { pid::lpfCutoff, 3200.0f }, { pid::aA, 0.45f },
            { pid::aD, 1.80f }, { pid::aS, 0.90f }, { pid::aR, 2.80f }, { pid::fA, 1.00f },
            { pid::fD, 2.50f }, { pid::fS, 0.55f }, { pid::fR, 2.20f }, { pid::ensemble, 0.68f },
            { pid::space, 0.50f }, { pid::noise, 0.06f }, { pid::lfoPitch, 0.10f },
        } },

        { "Glass Choir", "KEY", "Narrow pulse and ring modulation, glassy and vocal.", {
            { pid::vco2Pw, 0.12f }, { pid::ring, 0.35f }, { pid::hpfCutoff, 600.0f },
            { pid::lpfCutoff, 4800.0f }, { pid::lpfRes, 0.50f }, { pid::fS, 0.75f },
            { pid::fA, 0.60f }, { pid::aA, 0.35f }, { pid::aR, 2.20f }, { pid::space, 0.40f },
            { pid::ensemble, 0.60f }, { pid::vco2Level, 0.35f }, { pid::vco1Detune, -3.0f },
            { pid::vco2Detune, 9.0f },
        } },

        { "Sub Ribbon", "BASS", "Round mono bass for the ribbon controller.", {
            { pid::vco1Octave, -1.0f }, { pid::vco2Octave, -1.0f }, { pid::lpfCutoff, 700.0f },
            { pid::lpfRes, 0.20f }, { pid::aA, 0.01f }, { pid::aD, 0.60f }, { pid::aS, 0.90f },
            { pid::aR, 0.40f }, { pid::fA, 0.02f }, { pid::fD, 0.70f }, { pid::fS, 0.20f },
            { pid::fIl, 0.10f }, { pid::fAl, 0.85f }, { pid::ensemble, 0.05f },
            { pid::space, 0.05f }, { pid::spread, 0.0f }, { pid::vco2Level, 0.40f },
            { pid::master, -3.0f },
        } },

        { "Silver Flute", "LEAD", "Noise-flecked flute with a delayed, breathy vibrato.", {
            { pid::noise, 0.12f }, { pid::vco1Level, 0.50f }, { pid::vco2Level, 0.15f },
            { pid::vco2Pw, 0.50f }, { pid::hpfCutoff, 500.0f }, { pid::lpfCutoff, 5200.0f },
            { pid::lpfRes, 0.15f }, { pid::aA, 0.12f }, { pid::aD, 1.40f }, { pid::aS, 0.90f },
            { pid::aR, 1.20f }, { pid::fA, 0.25f }, { pid::fD, 1.20f }, { pid::fS, 0.60f },
            { pid::ensemble, 0.25f }, { pid::space, 0.35f }, { pid::lfoPitch, 0.22f },
            { pid::lfoDelay, 0.50f },
        } },

        { "Brass Stab", "BRASS", "Short, punchy brass hit for chords and stabs.", {
            { pid::aA, 0.005f }, { pid::aD, 0.50f }, { pid::aS, 0.55f }, { pid::aR, 0.35f },
            { pid::fA, 0.02f }, { pid::fD, 0.60f }, { pid::fS, 0.35f }, { pid::fAl, 1.0f },
            { pid::lpfCutoff, 3000.0f }, { pid::ensemble, 0.20f }, { pid::space, 0.12f },
            { pid::master, -5.0f },
        } },

        { "Ribbon Sweep", "FX", "Resonant filter adventure with a very slow LFO.", {
            { pid::lpfCutoff, 1200.0f }, { pid::lpfRes, 0.70f }, { pid::fIl, 0.0f },
            { pid::fAl, 1.0f }, { pid::fA, 2.00f }, { pid::fD, 3.50f }, { pid::fS, 0.20f },
            { pid::fR, 3.00f }, { pid::aA, 0.40f }, { pid::aD, 2.00f }, { pid::aS, 0.80f },
            { pid::aR, 3.00f }, { pid::lfoFilter, 0.70f }, { pid::lfoRate, 0.16f },
            { pid::ensemble, 0.70f }, { pid::space, 0.55f }, { pid::noise, 0.08f },
            { pid::vco2Pw, 0.20f },
        } },
    };

    return presets;
}

void applyPreset (Params& params, const Preset& preset)
{
    params = defaultParams();

    for (const auto& override : preset.values)
        setParam (params, override.id, override.value);
}

const Preset* findPreset (const std::string& name)
{
    for (const auto& preset : factoryPresets())
        if (name == preset.name)
            return &preset;

    return nullptr;
}
} // namespace cs80
