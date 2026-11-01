#include "Presets.h"

namespace cs80
{
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = {
        { "Blade Brass", "BRASS", "Wide, slow-blooming dual-channel brass with a warm ensemble.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.44f }, { pid::layerDetune, 8.0f },
            { pid::layerPan, 0.32f }, { pid::layerCutoff, 0.17f }, { pid::layerRes, 0.29f },
            { pid::layerEnv, 1.10f }, { pid::sineLevel, 0.24f }, { pid::vco1Sub, 0.18f },
            { pid::drive, 0.38f }, { pid::vintage, 0.44f }, { pid::filterTrack, 0.40f },
            { pid::fIl, 0.16f }, { pid::fAl, 0.96f }, { pid::fA, 0.20f }, { pid::fD, 1.70f },
            { pid::fS, 0.48f }, { pid::fR, 1.50f }, { pid::aA, 0.10f }, { pid::aD, 1.40f },
            { pid::aS, 0.88f }, { pid::aR, 1.60f }, { pid::twang, 0.30f }, { pid::aftertouch, 0.82f },
            { pid::ensemble, 0.55f }, { pid::space, 0.24f }, { pid::lfoPitch, 0.09f },
            { pid::lfoRate, 4.8f }, { pid::lfoDelay, 0.55f }, { pid::master, -7.0f },
        } },

        { "Vangelis Lead", "LEAD", "Bright, vocal lead with aftertouch vibrato and a touch of glide.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.28f }, { pid::layerDetune, 10.0f },
            { pid::layerPan, 0.18f }, { pid::layerCutoff, 0.22f }, { pid::layerRes, 0.34f },
            { pid::layerEnv, 1.10f }, { pid::sineLevel, 0.20f }, { pid::lpfCutoff, 5200.0f },
            { pid::lpfRes, 0.44f }, { pid::vco2Pw, 0.30f }, { pid::ensemble, 0.50f },
            { pid::space, 0.32f }, { pid::aftertouch, 0.96f }, { pid::fIl, 0.14f },
            { pid::fAl, 1.0f }, { pid::fA, 0.02f }, { pid::fD, 0.90f }, { pid::fS, 0.60f },
            { pid::aA, 0.02f }, { pid::aD, 1.10f }, { pid::aS, 0.90f }, { pid::aR, 1.30f },
            { pid::twang, 0.22f }, { pid::lfoPitch, 0.20f }, { pid::lfoRate, 5.2f },
            { pid::vco1Detune, -10.0f }, { pid::vco2Detune, 10.0f }, { pid::master, -7.5f },
            { pid::vco1Sub, 0.10f }, { pid::glide, 0.05f }, { pid::drive, 0.40f },
            { pid::filterTrack, 0.30f }, { pid::vintage, 0.30f },
        } },

        { "Neon Pads", "PAD", "Huge detuned pad with a very slow filter tide.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.58f }, { pid::layerDetune, 15.0f },
            { pid::layerPan, 0.62f }, { pid::layerCutoff, 0.36f }, { pid::layerRes, 0.30f },
            { pid::layerEnv, 1.45f }, { pid::layerPw, 0.55f }, { pid::sineLevel, 0.14f },
            { pid::aA, 1.00f }, { pid::aD, 2.50f }, { pid::aS, 0.80f }, { pid::aR, 3.60f },
            { pid::fA, 1.20f }, { pid::fD, 3.00f }, { pid::fS, 0.68f }, { pid::fR, 2.80f },
            { pid::ensemble, 0.78f }, { pid::space, 0.46f }, { pid::lpfCutoff, 1850.0f },
            { pid::spread, 0.85f }, { pid::lfoPwm, 0.40f }, { pid::lfoRate, 0.35f },
            { pid::vco1Detune, -14.0f }, { pid::vco2Detune, 14.0f }, { pid::master, -9.0f },
            { pid::vco1Sub, 0.18f }, { pid::vintage, 0.58f }, { pid::drive, 0.22f },
        } },

        { "Cinematic Strings", "STRINGS", "Soft string machine, high-passed, layered and airborne.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.48f }, { pid::layerDetune, 10.0f },
            { pid::layerPan, 0.58f }, { pid::layerCutoff, 0.22f }, { pid::layerRes, 0.24f },
            { pid::layerEnv, 1.30f }, { pid::sineLevel, 0.12f }, { pid::hpfCutoff, 260.0f },
            { pid::lpfCutoff, 3200.0f }, { pid::aA, 0.50f }, { pid::aD, 1.80f },
            { pid::aS, 0.90f }, { pid::aR, 2.90f }, { pid::fA, 1.00f }, { pid::fD, 2.50f },
            { pid::fS, 0.55f }, { pid::fR, 2.30f }, { pid::ensemble, 0.72f }, { pid::space, 0.52f },
            { pid::noise, 0.05f }, { pid::lfoPitch, 0.10f }, { pid::vintage, 0.46f },
            { pid::filterTrack, 0.48f }, { pid::twang, 0.12f }, { pid::master, -8.5f },
        } },

        { "Glass Choir", "KEY", "Narrow pulse and ring modulation, glassy and vocal.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.40f }, { pid::layerDetune, 5.0f },
            { pid::layerPan, 0.36f }, { pid::layerCutoff, 0.30f }, { pid::layerRes, 0.38f },
            { pid::vco2Pw, 0.12f }, { pid::sineLevel, 0.26f }, { pid::ring, 0.32f },
            { pid::ringSpeed, 260.0f }, { pid::hpfCutoff, 560.0f }, { pid::lpfCutoff, 4600.0f },
            { pid::lpfRes, 0.50f }, { pid::fS, 0.72f }, { pid::fA, 0.60f }, { pid::aA, 0.35f },
            { pid::aD, 1.80f }, { pid::aR, 2.30f }, { pid::space, 0.42f }, { pid::ensemble, 0.62f },
            { pid::vco2Level, 0.35f }, { pid::vco1Detune, -3.0f }, { pid::vco2Detune, 9.0f },
            { pid::vintage, 0.26f }, { pid::filterTrack, 0.52f }, { pid::master, -8.0f },
        } },

        { "Sub Ribbon", "BASS", "Round mono bass with a sub-octave and a short ribbon glide.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::vco1Octave, -1.0f },
            { pid::vco2Octave, -1.0f }, { pid::sineLevel, 0.32f }, { pid::lpfCutoff, 720.0f },
            { pid::lpfRes, 0.20f }, { pid::aA, 0.01f }, { pid::aD, 0.60f }, { pid::aS, 0.90f },
            { pid::aR, 0.40f }, { pid::fA, 0.02f }, { pid::fD, 0.70f }, { pid::fS, 0.20f },
            { pid::fIl, 0.10f }, { pid::fAl, 0.85f }, { pid::ensemble, 0.05f }, { pid::space, 0.05f },
            { pid::spread, 0.0f }, { pid::vco2Level, 0.40f }, { pid::master, -5.0f },
            { pid::vco1Sub, 0.70f }, { pid::glide, 0.11f }, { pid::drive, 0.48f },
            { pid::vintage, 0.22f }, { pid::filterTrack, 0.20f },
        } },

        { "Silver Flute", "LEAD", "Noise-flecked flute with a delayed, breathy vibrato.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::noise, 0.11f },
            { pid::sineLevel, 0.34f }, { pid::vco1Level, 0.46f }, { pid::vco2Level, 0.14f },
            { pid::vco2Pw, 0.50f }, { pid::hpfCutoff, 480.0f }, { pid::lpfCutoff, 5200.0f },
            { pid::lpfRes, 0.16f }, { pid::aA, 0.12f }, { pid::aD, 1.40f }, { pid::aS, 0.90f },
            { pid::aR, 1.20f }, { pid::fA, 0.25f }, { pid::fD, 1.20f }, { pid::fS, 0.60f },
            { pid::ensemble, 0.24f }, { pid::space, 0.38f }, { pid::lfoPitch, 0.20f },
            { pid::lfoDelay, 0.55f }, { pid::vintage, 0.32f }, { pid::glide, 0.04f },
            { pid::filterTrack, 0.38f }, { pid::twang, 0.10f }, { pid::master, -6.5f },
        } },

        { "Brass Stab", "BRASS", "Short, punchy dual-channel brass hit for chords and stabs.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.32f }, { pid::layerDetune, 6.0f },
            { pid::layerPan, 0.24f }, { pid::layerCutoff, 0.14f }, { pid::layerRes, 0.32f },
            { pid::sineLevel, 0.20f }, { pid::aA, 0.006f }, { pid::aD, 0.55f }, { pid::aS, 0.58f },
            { pid::aR, 0.38f }, { pid::fA, 0.02f }, { pid::fD, 0.62f }, { pid::fS, 0.34f },
            { pid::fAl, 1.0f }, { pid::lpfCutoff, 3100.0f }, { pid::ensemble, 0.20f },
            { pid::space, 0.14f }, { pid::master, -7.0f }, { pid::vco1Sub, 0.22f },
            { pid::drive, 0.52f }, { pid::twang, 0.42f }, { pid::vintage, 0.36f },
            { pid::filterTrack, 0.36f },
        } },

        { "Ribbon Sweep", "FX", "Resonant filter adventure with a very slow LFO.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.36f }, { pid::layerDetune, 13.0f },
            { pid::layerPan, 0.48f }, { pid::layerCutoff, 0.42f }, { pid::layerRes, 0.55f },
            { pid::layerEnv, 1.40f }, { pid::sineLevel, 0.08f }, { pid::lpfCutoff, 1200.0f },
            { pid::lpfRes, 0.70f }, { pid::fIl, 0.0f }, { pid::fAl, 1.0f }, { pid::fA, 2.00f },
            { pid::fD, 3.50f }, { pid::fS, 0.20f }, { pid::fR, 3.00f }, { pid::aA, 0.42f },
            { pid::aD, 2.00f }, { pid::aS, 0.80f }, { pid::aR, 3.00f }, { pid::lfoFilter, 0.70f },
            { pid::lfoRate, 0.16f }, { pid::ensemble, 0.70f }, { pid::space, 0.58f },
            { pid::noise, 0.08f }, { pid::vco2Pw, 0.20f }, { pid::drive, 0.48f },
            { pid::vintage, 0.52f }, { pid::master, -9.0f },
        } },

        { "Blade Runner", "BRASS", "The big one: slow, rain-soaked brass over a glowing ensemble.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.56f }, { pid::layerDetune, 11.0f },
            { pid::layerPan, 0.54f }, { pid::layerCutoff, 0.24f }, { pid::layerRes, 0.36f },
            { pid::layerEnv, 1.30f }, { pid::sineLevel, 0.22f }, { pid::lpfCutoff, 1400.0f },
            { pid::lpfRes, 0.40f }, { pid::hpfCutoff, 88.0f }, { pid::fIl, 0.22f },
            { pid::fAl, 0.98f }, { pid::fA, 1.10f }, { pid::fD, 3.20f }, { pid::fS, 0.58f },
            { pid::fR, 3.60f }, { pid::aA, 0.70f }, { pid::aD, 2.40f }, { pid::aS, 0.88f },
            { pid::aR, 3.40f }, { pid::ensemble, 0.80f }, { pid::space, 0.64f },
            { pid::spread, 0.90f }, { pid::vco1Sub, 0.22f }, { pid::vco1Detune, -9.0f },
            { pid::vco2Detune, 9.0f }, { pid::aftertouch, 0.95f }, { pid::lfoPitch, 0.12f },
            { pid::lfoRate, 4.2f }, { pid::lfoDelay, 0.70f }, { pid::vintage, 0.62f },
            { pid::drive, 0.38f }, { pid::filterTrack, 0.46f }, { pid::twang, 0.20f },
            { pid::master, -9.0f },
        } },

        { "Tears in Rain", "PAD", "Intimate, high-passed pad for slow chords and long goodbyes.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.48f }, { pid::layerDetune, 14.0f },
            { pid::layerPan, 0.62f }, { pid::layerCutoff, 0.32f }, { pid::layerRes, 0.34f },
            { pid::layerEnv, 1.55f }, { pid::sineLevel, 0.16f }, { pid::hpfCutoff, 600.0f },
            { pid::lpfCutoff, 4300.0f }, { pid::lpfRes, 0.44f }, { pid::aA, 0.55f },
            { pid::aD, 2.20f }, { pid::aS, 0.82f }, { pid::aR, 3.90f }, { pid::fA, 1.30f },
            { pid::fD, 2.80f }, { pid::fS, 0.58f }, { pid::fR, 3.20f }, { pid::ensemble, 0.70f },
            { pid::space, 0.68f }, { pid::vco1Sub, 0.10f }, { pid::lfoPitch, 0.18f },
            { pid::lfoDelay, 0.80f }, { pid::aftertouch, 0.88f }, { pid::glide, 0.16f },
            { pid::tune, -2.0f }, { pid::vintage, 0.44f }, { pid::master, -9.0f },
            { pid::spread, 0.72f },
        } },

        { "CS-80 Wind", "FX", "Breathing noise and ring modulation, a storm behind the strings.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.52f }, { pid::layerDetune, 18.0f },
            { pid::layerPan, 0.66f }, { pid::layerCutoff, 0.50f }, { pid::layerRes, 0.58f },
            { pid::layerEnv, 1.60f }, { pid::sineLevel, 0.05f }, { pid::noise, 0.34f },
            { pid::ring, 0.40f }, { pid::ringSpeed, 120.0f }, { pid::vco2Level, 0.42f },
            { pid::vco2Pw, 0.16f }, { pid::hpfCutoff, 300.0f }, { pid::lpfCutoff, 900.0f },
            { pid::lpfRes, 0.72f }, { pid::fIl, 0.05f }, { pid::fAl, 1.0f }, { pid::fA, 2.50f },
            { pid::fD, 4.00f }, { pid::fS, 0.16f }, { pid::fR, 4.00f }, { pid::aA, 1.50f },
            { pid::aD, 3.00f }, { pid::aS, 0.74f }, { pid::aR, 4.20f }, { pid::lfoFilter, 0.82f },
            { pid::lfoRate, 0.12f }, { pid::ensemble, 0.64f }, { pid::space, 0.74f },
            { pid::vintage, 0.68f }, { pid::master, -10.0f }, { pid::spread, 0.82f },
        } },

        { "Touch Bell", "KEY", "Ring-modulated bell that opens with velocity and aftertouch.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::ring, 0.55f },
            { pid::ringSpeed, 640.0f }, { pid::sineLevel, 0.12f }, { pid::vco2Level, 0.20f },
            { pid::vco2Pw, 0.12f }, { pid::hpfCutoff, 900.0f }, { pid::lpfCutoff, 6200.0f },
            { pid::lpfRes, 0.55f }, { pid::fIl, 0.0f }, { pid::fAl, 1.0f }, { pid::fA, 0.002f },
            { pid::fD, 0.50f }, { pid::fS, 0.0f }, { pid::aS, 0.0f }, { pid::aA, 0.002f },
            { pid::aD, 0.80f }, { pid::aR, 0.50f }, { pid::velocity, 0.90f },
            { pid::aftertouch, 0.70f }, { pid::space, 0.38f }, { pid::ensemble, 0.15f },
            { pid::vintage, 0.20f }, { pid::master, -7.0f }, { pid::filterTrack, 0.58f },
        } },

        { "Velvet Strings", "STRINGS", "Lush, soft-focus string ensemble with a gentle bloom.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.58f }, { pid::layerDetune, 10.0f },
            { pid::layerPan, 0.68f }, { pid::layerCutoff, 0.26f }, { pid::layerRes, 0.22f },
            { pid::layerEnv, 1.35f }, { pid::sineLevel, 0.12f }, { pid::hpfCutoff, 300.0f },
            { pid::lpfCutoff, 2600.0f }, { pid::lpfRes, 0.26f }, { pid::aA, 0.80f },
            { pid::aD, 2.80f }, { pid::aS, 0.92f }, { pid::aR, 3.80f }, { pid::fA, 1.40f },
            { pid::fD, 3.00f }, { pid::fS, 0.62f }, { pid::fR, 3.40f }, { pid::ensemble, 0.88f },
            { pid::space, 0.48f }, { pid::spread, 0.88f }, { pid::vco1Sub, 0.14f },
            { pid::vintage, 0.56f }, { pid::filterTrack, 0.50f }, { pid::master, -9.0f },
        } },

        { "Prog Organ", "ORGAN", "Pulse-organ stack through the ensemble, with a warm sub.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::vco1Level, 0.58f },
            { pid::vco2Level, 0.60f }, { pid::vco2Pw, 0.50f }, { pid::vco1Sub, 0.36f },
            { pid::sineLevel, 0.26f }, { pid::hpfCutoff, 180.0f }, { pid::lpfCutoff, 7000.0f },
            { pid::fS, 0.82f }, { pid::aA, 0.005f }, { pid::aD, 0.20f }, { pid::aS, 1.0f },
            { pid::aR, 0.14f }, { pid::ensemble, 0.38f }, { pid::space, 0.15f },
            { pid::spread, 0.35f }, { pid::drive, 0.30f }, { pid::vintage, 0.24f },
            { pid::master, -7.0f }, { pid::filterTrack, 0.24f },
        } },

        { "Analog Bass", "BASS", "Round, gliding sub-bass with a tight filter pluck.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::vco1Level, 0.56f },
            { pid::vco2Level, 0.20f }, { pid::vco1Sub, 0.80f }, { pid::sineLevel, 0.34f },
            { pid::vco1Octave, -1.0f }, { pid::vco2Octave, -1.0f }, { pid::hpfCutoff, 40.0f },
            { pid::lpfCutoff, 620.0f }, { pid::lpfRes, 0.34f }, { pid::fIl, 0.12f },
            { pid::fAl, 0.86f }, { pid::fA, 0.005f }, { pid::fD, 0.42f }, { pid::fS, 0.12f },
            { pid::aA, 0.004f }, { pid::aD, 0.50f }, { pid::aS, 0.92f }, { pid::aR, 0.24f },
            { pid::glide, 0.13f }, { pid::velocity, 0.30f }, { pid::ensemble, 0.04f },
            { pid::space, 0.02f }, { pid::spread, 0.0f }, { pid::drive, 0.48f },
            { pid::vintage, 0.18f }, { pid::filterTrack, 0.18f }, { pid::master, -5.0f },
        } },

        { "Neon Arp", "KEY", "Tight, resonant pluck for fast lines and octave patterns.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::vco2Pw, 0.25f },
            { pid::sineLevel, 0.10f }, { pid::ring, 0.16f }, { pid::ringSpeed, 420.0f },
            { pid::hpfCutoff, 420.0f }, { pid::lpfCutoff, 3600.0f }, { pid::lpfRes, 0.58f },
            { pid::fIl, 0.0f }, { pid::fAl, 1.0f }, { pid::fA, 0.002f }, { pid::fD, 0.24f },
            { pid::fS, 0.0f }, { pid::aA, 0.002f }, { pid::aD, 0.30f }, { pid::aS, 0.0f },
            { pid::aR, 0.20f }, { pid::ensemble, 0.54f }, { pid::space, 0.26f },
            { pid::lfoPwm, 0.42f }, { pid::lfoRate, 5.5f }, { pid::glide, 0.03f },
            { pid::spread, 0.62f }, { pid::drive, 0.42f }, { pid::vintage, 0.28f },
            { pid::filterTrack, 0.46f }, { pid::master, -8.0f },
        } },

        { "Soft Horn", "BRASS", "Mellow French-horn brass that swells under the fingers.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.36f }, { pid::layerDetune, 7.0f },
            { pid::layerPan, 0.30f }, { pid::layerCutoff, 0.10f }, { pid::layerRes, 0.28f },
            { pid::sineLevel, 0.24f }, { pid::hpfCutoff, 150.0f }, { pid::lpfCutoff, 1900.0f },
            { pid::lpfRes, 0.22f }, { pid::fIl, 0.30f }, { pid::fAl, 0.76f }, { pid::fA, 0.28f },
            { pid::fD, 1.60f }, { pid::fS, 0.50f }, { pid::fR, 1.80f }, { pid::aA, 0.16f },
            { pid::aD, 1.40f }, { pid::aS, 0.90f }, { pid::aR, 1.40f }, { pid::ensemble, 0.52f },
            { pid::space, 0.32f }, { pid::aftertouch, 0.84f }, { pid::twang, 0.28f },
            { pid::vintage, 0.46f }, { pid::filterTrack, 0.40f }, { pid::vco1Sub, 0.12f },
            { pid::master, -8.0f },
        } },

        { "Brass Ensemble", "BRASS", "A wide, slow CS-80 brass choir with both channel cards singing.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.62f }, { pid::layerDetune, 12.0f },
            { pid::layerPan, 0.58f }, { pid::layerCutoff, 0.28f }, { pid::layerRes, 0.34f },
            { pid::layerEnv, 1.30f }, { pid::sineLevel, 0.20f }, { pid::hpfCutoff, 100.0f },
            { pid::lpfCutoff, 2100.0f }, { pid::lpfRes, 0.34f }, { pid::fIl, 0.16f },
            { pid::fAl, 0.98f }, { pid::fA, 0.38f }, { pid::fD, 2.20f }, { pid::fS, 0.62f },
            { pid::fR, 2.40f }, { pid::aA, 0.18f }, { pid::aD, 1.80f }, { pid::aS, 0.92f },
            { pid::aR, 2.20f }, { pid::ensemble, 0.72f }, { pid::space, 0.46f },
            { pid::spread, 0.86f }, { pid::aftertouch, 0.92f }, { pid::vco1Sub, 0.14f },
            { pid::twang, 0.24f }, { pid::vintage, 0.54f }, { pid::drive, 0.32f },
            { pid::master, -8.5f }, { pid::filterTrack, 0.44f },
        } },

        { "Aftertouch Brass", "LEAD", "A solo brass voice built around a deep, singing pressure swell.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.40f }, { pid::layerDetune, 9.0f },
            { pid::layerPan, 0.40f }, { pid::layerCutoff, 0.36f }, { pid::layerRes, 0.40f },
            { pid::layerEnv, 1.20f }, { pid::sineLevel, 0.20f }, { pid::lpfCutoff, 2600.0f },
            { pid::lpfRes, 0.46f }, { pid::hpfCutoff, 140.0f }, { pid::fIl, 0.10f },
            { pid::fAl, 1.0f }, { pid::fA, 0.08f }, { pid::fD, 1.30f }, { pid::fS, 0.56f },
            { pid::fR, 1.50f }, { pid::aA, 0.04f }, { pid::aD, 1.10f }, { pid::aS, 0.88f },
            { pid::aR, 1.40f }, { pid::aftertouch, 1.0f }, { pid::lfoPitch, 0.20f },
            { pid::lfoRate, 5.0f }, { pid::lfoDelay, 0.30f }, { pid::ensemble, 0.48f },
            { pid::space, 0.40f }, { pid::vintage, 0.44f }, { pid::drive, 0.44f },
            { pid::twang, 0.25f }, { pid::master, -7.5f }, { pid::filterTrack, 0.38f },
        } },

        { "Split Brass / Bell", "SPLIT", "Channel I anchors the left hand; Channel II rings above C4.", {
            { pid::layerMode, 2.0f }, { pid::splitPoint, 60.0f }, { pid::layerLevel, 0.78f },
            { pid::layerDetune, 4.0f }, { pid::layerPan, 0.46f }, { pid::layerPw, 0.18f },
            { pid::layerCutoff, 0.72f }, { pid::layerRes, 0.52f }, { pid::layerEnv, 0.90f },
            { pid::sineLevel, 0.16f }, { pid::ring, 0.30f }, { pid::ringSpeed, 520.0f },
            { pid::lpfCutoff, 2400.0f }, { pid::lpfRes, 0.34f }, { pid::hpfCutoff, 110.0f },
            { pid::fIl, 0.24f }, { pid::fAl, 0.92f }, { pid::fA, 0.10f }, { pid::fD, 1.40f },
            { pid::fS, 0.48f }, { pid::fR, 1.80f }, { pid::aA, 0.10f }, { pid::aD, 1.30f },
            { pid::aS, 0.82f }, { pid::aR, 1.60f }, { pid::ensemble, 0.58f }, { pid::space, 0.46f },
            { pid::spread, 0.70f }, { pid::vintage, 0.50f }, { pid::twang, 0.20f },
            { pid::master, -8.5f }, { pid::filterTrack, 0.40f },
        } },

        { "Glass Cathedral", "PAD", "A luminous dual-layer pad suspended in a very large plate.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.66f }, { pid::layerDetune, 17.0f },
            { pid::layerPan, 0.72f }, { pid::layerCutoff, 0.54f }, { pid::layerRes, 0.44f },
            { pid::layerEnv, 1.70f }, { pid::layerPw, 0.24f }, { pid::sineLevel, 0.18f },
            { pid::hpfCutoff, 500.0f }, { pid::lpfCutoff, 3600.0f }, { pid::lpfRes, 0.42f },
            { pid::ring, 0.20f }, { pid::ringSpeed, 180.0f }, { pid::aA, 1.20f },
            { pid::aD, 3.20f }, { pid::aS, 0.86f }, { pid::aR, 4.80f }, { pid::fA, 1.80f },
            { pid::fD, 3.60f }, { pid::fS, 0.66f }, { pid::fR, 4.20f }, { pid::ensemble, 0.82f },
            { pid::space, 0.80f }, { pid::spread, 0.92f }, { pid::lfoPitch, 0.10f },
            { pid::lfoRate, 0.22f }, { pid::vintage, 0.58f }, { pid::master, -10.0f },
            { pid::filterTrack, 0.42f },
        } },

        { "Night Runner", "BRASS", "Dark, slow-moving brass for neon skylines and long credits.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.60f }, { pid::layerDetune, 15.0f },
            { pid::layerPan, 0.60f }, { pid::layerCutoff, 0.18f }, { pid::layerRes, 0.40f },
            { pid::layerEnv, 1.45f }, { pid::sineLevel, 0.20f }, { pid::lpfCutoff, 1050.0f },
            { pid::lpfRes, 0.40f }, { pid::hpfCutoff, 78.0f }, { pid::fIl, 0.24f },
            { pid::fAl, 0.94f }, { pid::fA, 1.50f }, { pid::fD, 3.80f }, { pid::fS, 0.46f },
            { pid::fR, 4.20f }, { pid::aA, 0.90f }, { pid::aD, 2.80f }, { pid::aS, 0.86f },
            { pid::aR, 4.00f }, { pid::ensemble, 0.76f }, { pid::space, 0.72f },
            { pid::spread, 0.90f }, { pid::vco1Sub, 0.20f }, { pid::aftertouch, 0.92f },
            { pid::lfoPitch, 0.10f }, { pid::lfoRate, 3.1f }, { pid::lfoDelay, 0.90f },
            { pid::vintage, 0.64f }, { pid::twang, 0.15f }, { pid::master, -9.5f },
            { pid::filterTrack, 0.44f },
        } },

        { "Split Brass / Bass", "SPLIT", "A CS-80 two-handed setup: round bass below C3, brass above.", {
            { pid::layerMode, 2.0f }, { pid::splitPoint, 48.0f }, { pid::layerLevel, 0.70f },
            { pid::layerDetune, 8.0f }, { pid::layerPan, 0.32f }, { pid::layerCutoff, 0.42f },
            { pid::layerRes, 0.30f }, { pid::layerEnv, 1.10f }, { pid::sineLevel, 0.18f },
            { pid::lpfCutoff, 1600.0f }, { pid::lpfRes, 0.32f }, { pid::hpfCutoff, 90.0f },
            { pid::fIl, 0.14f }, { pid::fAl, 0.92f }, { pid::fA, 0.12f }, { pid::fD, 1.20f },
            { pid::fS, 0.44f }, { pid::fR, 1.40f }, { pid::aA, 0.06f }, { pid::aD, 1.00f },
            { pid::aS, 0.86f }, { pid::aR, 1.50f }, { pid::ensemble, 0.42f }, { pid::space, 0.36f },
            { pid::spread, 0.58f }, { pid::vco1Sub, 0.20f }, { pid::twang, 0.18f },
            { pid::vintage, 0.42f }, { pid::master, -8.0f }, { pid::filterTrack, 0.38f },
        } },

        //==============================================================================
        // The 4.0 additions: slower, more vocal, and built around the new VCA
        // sine path and touch bend.
        { "Chariots Brass", "BRASS", "Ceremonial, wide-screen brass. Slow to bloom, enormous in the room.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.50f }, { pid::layerDetune, 9.0f },
            { pid::layerPan, 0.44f }, { pid::layerCutoff, 0.20f }, { pid::layerRes, 0.30f },
            { pid::layerEnv, 1.22f }, { pid::sineLevel, 0.26f }, { pid::vco1Sub, 0.20f },
            { pid::hpfCutoff, 85.0f }, { pid::lpfCutoff, 2000.0f }, { pid::lpfRes, 0.32f },
            { pid::fIl, 0.18f }, { pid::fAl, 0.98f }, { pid::fA, 0.55f }, { pid::fD, 2.40f },
            { pid::fS, 0.55f }, { pid::fR, 2.40f }, { pid::aA, 0.35f }, { pid::aD, 1.80f },
            { pid::aS, 0.90f }, { pid::aR, 2.60f }, { pid::twang, 0.34f }, { pid::aftertouch, 0.90f },
            { pid::ensemble, 0.66f }, { pid::space, 0.52f }, { pid::spread, 0.82f },
            { pid::drive, 0.34f }, { pid::vintage, 0.48f }, { pid::filterTrack, 0.42f },
            { pid::master, -9.0f },
        } },

        { "Love Theme", "LEAD", "A throaty, close-miked solo voice that leans into the pressure.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.34f }, { pid::layerDetune, 8.0f },
            { pid::layerPan, 0.22f }, { pid::layerCutoff, 0.28f }, { pid::layerRes, 0.36f },
            { pid::layerEnv, 1.16f }, { pid::sineLevel, 0.30f }, { pid::vco1Sub, 0.12f },
            { pid::vco2Pw, 0.36f }, { pid::hpfCutoff, 160.0f }, { pid::lpfCutoff, 3800.0f },
            { pid::lpfRes, 0.38f }, { pid::fIl, 0.18f }, { pid::fAl, 0.94f }, { pid::fA, 0.16f },
            { pid::fD, 1.50f }, { pid::fS, 0.52f }, { pid::fR, 1.80f }, { pid::aA, 0.08f },
            { pid::aD, 1.20f }, { pid::aS, 0.88f }, { pid::aR, 1.70f }, { pid::aftertouch, 1.0f },
            { pid::lfoPitch, 0.18f }, { pid::lfoRate, 4.7f }, { pid::lfoDelay, 0.42f },
            { pid::glide, 0.05f }, { pid::twang, 0.18f }, { pid::ensemble, 0.40f },
            { pid::space, 0.44f }, { pid::vintage, 0.34f }, { pid::master, -8.0f },
        } },

        { "Glass Harp", "KEY", "A bright, fragile plucked bell that hangs in the air.", {
            { pid::layerMode, 0.0f }, { pid::layerLevel, 0.0f }, { pid::vco1Level, 0.52f },
            { pid::vco2Level, 0.22f }, { pid::vco2Pw, 0.16f }, { pid::ring, 0.38f },
            { pid::ringSpeed, 780.0f }, { pid::sineLevel, 0.22f }, { pid::hpfCutoff, 760.0f },
            { pid::lpfCutoff, 5800.0f }, { pid::lpfRes, 0.54f }, { pid::fIl, 0.0f },
            { pid::fAl, 1.0f }, { pid::fA, 0.002f }, { pid::fD, 1.10f }, { pid::fS, 0.10f },
            { pid::aA, 0.002f }, { pid::aD, 1.30f }, { pid::aS, 0.0f }, { pid::aR, 1.60f },
            { pid::velocity, 0.78f }, { pid::aftertouch, 0.62f }, { pid::ensemble, 0.34f },
            { pid::space, 0.56f }, { pid::vintage, 0.24f }, { pid::filterTrack, 0.56f },
            { pid::master, -8.0f },
        } },

        { "Deep Choir", "PAD", "Dark vowel pad with a sine core and a slow, breathing ensemble.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.62f }, { pid::layerDetune, 13.0f },
            { pid::layerPan, 0.66f }, { pid::layerCutoff, 0.30f }, { pid::layerRes, 0.32f },
            { pid::layerEnv, 1.40f }, { pid::layerPw, 0.34f }, { pid::sineLevel, 0.28f },
            { pid::ring, 0.10f }, { pid::ringSpeed, 220.0f }, { pid::hpfCutoff, 360.0f },
            { pid::lpfCutoff, 2400.0f }, { pid::lpfRes, 0.30f }, { pid::fIl, 0.22f },
            { pid::fAl, 0.86f }, { pid::fA, 1.10f }, { pid::fD, 2.80f }, { pid::fS, 0.60f },
            { pid::fR, 3.40f }, { pid::aA, 1.10f }, { pid::aD, 2.60f }, { pid::aS, 0.86f },
            { pid::aR, 4.00f }, { pid::ensemble, 0.72f }, { pid::space, 0.64f },
            { pid::spread, 0.84f }, { pid::lfoPwm, 0.24f }, { pid::lfoRate, 0.28f },
            { pid::vintage, 0.50f }, { pid::master, -9.5f },
        } },

        { "Brass Section", "BRASS", "Tighter and brighter than the choir: four horns in a small room.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.40f }, { pid::layerDetune, 6.0f },
            { pid::layerPan, 0.28f }, { pid::layerCutoff, 0.16f }, { pid::layerRes, 0.28f },
            { pid::layerEnv, 1.05f }, { pid::sineLevel, 0.20f }, { pid::vco1Sub, 0.14f },
            { pid::lpfCutoff, 3200.0f }, { pid::lpfRes, 0.30f }, { pid::fIl, 0.12f },
            { pid::fAl, 0.98f }, { pid::fA, 0.06f }, { pid::fD, 0.90f }, { pid::fS, 0.44f },
            { pid::fR, 1.10f }, { pid::aA, 0.03f }, { pid::aD, 0.80f }, { pid::aS, 0.86f },
            { pid::aR, 1.00f }, { pid::twang, 0.40f }, { pid::aftertouch, 0.86f },
            { pid::ensemble, 0.34f }, { pid::space, 0.30f }, { pid::drive, 0.42f },
            { pid::vintage, 0.40f }, { pid::filterTrack, 0.38f }, { pid::master, -8.0f },
        } },

        { "Ribbon Glide", "FX", "A slow ribbon stretch through a resonant, breathing filter.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.46f }, { pid::layerDetune, 16.0f },
            { pid::layerPan, 0.60f }, { pid::layerCutoff, 0.50f }, { pid::layerRes, 0.56f },
            { pid::layerEnv, 1.50f }, { pid::sineLevel, 0.12f }, { pid::vco2Pw, 0.22f },
            { pid::hpfCutoff, 200.0f }, { pid::lpfCutoff, 1400.0f }, { pid::lpfRes, 0.62f },
            { pid::fIl, 0.08f }, { pid::fAl, 1.0f }, { pid::fA, 1.20f }, { pid::fD, 2.60f },
            { pid::fS, 0.36f }, { pid::fR, 3.40f }, { pid::aA, 0.30f }, { pid::aD, 1.80f },
            { pid::aS, 0.82f }, { pid::aR, 3.00f }, { pid::glide, 0.35f }, { pid::lfoFilter, 0.48f },
            { pid::lfoRate, 0.20f }, { pid::ensemble, 0.64f }, { pid::space, 0.60f },
            { pid::vintage, 0.56f }, { pid::master, -9.0f },
        } },

        { "Split Pad / Bell", "SPLIT", "A warm pad below the break and a luminous bell above it.", {
            { pid::layerMode, 2.0f }, { pid::splitPoint, 60.0f }, { pid::layerLevel, 0.72f },
            { pid::layerDetune, 6.0f }, { pid::layerPan, 0.50f }, { pid::layerPw, 0.18f },
            { pid::layerCutoff, 0.68f }, { pid::layerRes, 0.55f }, { pid::layerEnv, 1.30f },
            { pid::sineLevel, 0.24f }, { pid::ring, 0.28f }, { pid::ringSpeed, 460.0f },
            { pid::hpfCutoff, 180.0f }, { pid::lpfCutoff, 2600.0f }, { pid::lpfRes, 0.38f },
            { pid::fIl, 0.18f }, { pid::fAl, 0.92f }, { pid::fA, 0.60f }, { pid::fD, 2.40f },
            { pid::fS, 0.56f }, { pid::fR, 2.60f }, { pid::aA, 0.60f }, { pid::aD, 2.00f },
            { pid::aS, 0.84f }, { pid::aR, 3.00f }, { pid::ensemble, 0.68f }, { pid::space, 0.62f },
            { pid::spread, 0.80f }, { pid::vintage, 0.52f }, { pid::master, -9.5f },
            { pid::filterTrack, 0.44f },
        } },

        { "Aftertouch Strings", "STRINGS", "A full string ensemble that opens and sings under pressure.", {
            { pid::layerMode, 1.0f }, { pid::layerLevel, 0.60f }, { pid::layerDetune, 11.0f },
            { pid::layerPan, 0.64f }, { pid::layerCutoff, 0.28f }, { pid::layerRes, 0.30f },
            { pid::layerEnv, 1.40f }, { pid::sineLevel, 0.14f }, { pid::hpfCutoff, 320.0f },
            { pid::lpfCutoff, 3000.0f }, { pid::lpfRes, 0.34f }, { pid::fIl, 0.20f },
            { pid::fAl, 0.88f }, { pid::fA, 0.80f }, { pid::fD, 2.60f }, { pid::fS, 0.56f },
            { pid::fR, 3.20f }, { pid::aA, 0.70f }, { pid::aD, 2.20f }, { pid::aS, 0.90f },
            { pid::aR, 3.60f }, { pid::aftertouch, 1.0f }, { pid::lfoPitch, 0.15f },
            { pid::lfoDelay, 0.60f }, { pid::lfoRate, 4.4f }, { pid::ensemble, 0.80f },
            { pid::space, 0.58f }, { pid::spread, 0.88f }, { pid::vintage, 0.50f },
            { pid::master, -9.0f }, { pid::filterTrack, 0.46f },
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
