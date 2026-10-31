#pragma once

#include <string>
#include <vector>

namespace cs80
{
//==============================================================================
/** Every automatable parameter id. Keeping these in one place stops the DSP,
    the parameter layout and the UI from drifting apart. */
namespace pid
{
inline constexpr const char* master     = "master";
inline constexpr const char* brilliance = "brilliance";
inline constexpr const char* ensemble   = "ensemble";
inline constexpr const char* space      = "space";

inline constexpr const char* ring       = "ring";
inline constexpr const char* noise      = "noise";

inline constexpr const char* vco1Level  = "vco1Level";
inline constexpr const char* vco1Octave = "vco1Octave";
inline constexpr const char* vco1Detune = "vco1Detune";

inline constexpr const char* vco2Level  = "vco2Level";
inline constexpr const char* vco2Pw     = "vco2Pw";
inline constexpr const char* vco2Octave = "vco2Octave";
inline constexpr const char* vco2Detune = "vco2Detune";

inline constexpr const char* hpfCutoff  = "hpfCutoff";
inline constexpr const char* hpfRes     = "hpfRes";
inline constexpr const char* lpfCutoff  = "lpfCutoff";
inline constexpr const char* lpfRes     = "lpfRes";

inline constexpr const char* fIl        = "fIl";
inline constexpr const char* fAl        = "fAl";
inline constexpr const char* fA         = "fA";
inline constexpr const char* fD         = "fD";
inline constexpr const char* fS         = "fS";
inline constexpr const char* fR         = "fR";

inline constexpr const char* aA         = "aA";
inline constexpr const char* aD         = "aD";
inline constexpr const char* aS         = "aS";
inline constexpr const char* aR         = "aR";

inline constexpr const char* lfoRate    = "lfoRate";
inline constexpr const char* lfoWave    = "lfoWave";
inline constexpr const char* lfoDelay   = "lfoDelay";
inline constexpr const char* lfoPitch   = "lfoPitch";
inline constexpr const char* lfoPwm     = "lfoPwm";
inline constexpr const char* lfoFilter  = "lfoFilter";
inline constexpr const char* lfoAmp     = "lfoAmp";

inline constexpr const char* aftertouch = "aftertouch";
inline constexpr const char* bendRange  = "bendRange";
inline constexpr const char* spread     = "spread";
inline constexpr const char* velocity   = "velocity";
} // namespace pid

//==============================================================================
struct ParamSpec
{
    const char* id = "";
    const char* name = "";
    const char* group = "";
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float skew = 1.0f;           // 1 = linear, < 1 gives the low end more travel
    int steps = 0;               // 0 = continuous
    const char* unit = "";
    const char* choices = nullptr; // pipe separated labels, when this is a choice
};

const std::vector<ParamSpec>& paramSpecs();
const ParamSpec* findSpec (const std::string& id);
int specIndex (const std::string& id);

//==============================================================================
/** A plain-old-data snapshot of the whole instrument. The audio engine only
    ever sees this struct, which keeps the DSP independent of JUCE and trivially
    unit-testable. */
struct Params
{
    float masterDb = -4.0f;
    float brilliance = 0.0f;
    float ensemble = 0.38f;
    float space = 0.16f;

    float ring = 0.05f;
    float noise = 0.015f;

    float vco1Level = 0.78f;
    float vco1Octave = 0.0f;
    float vco1Detune = -6.0f;

    float vco2Level = 0.62f;
    float vco2Pw = 0.42f;
    float vco2Octave = 0.0f;
    float vco2Detune = 7.0f;

    float hpfCutoff = 130.0f;
    float hpfRes = 0.12f;
    float lpfCutoff = 2400.0f;
    float lpfRes = 0.28f;

    float fIl = 0.12f;
    float fAl = 0.92f;
    float fA = 0.18f;
    float fD = 1.4f;
    float fS = 0.42f;
    float fR = 0.9f;

    float aA = 0.06f;
    float aD = 1.2f;
    float aS = 0.85f;
    float aR = 1.1f;

    float lfoRate = 4.6f;
    float lfoWave = 0.0f;   // triangle
    float lfoDelay = 0.35f;
    float lfoPitch = 0.12f;
    float lfoPwm = 0.18f;
    float lfoFilter = 0.0f;
    float lfoAmp = 0.0f;

    float aftertouch = 0.55f;
    float bendRange = 2.0f;
    float spread = 0.45f;
    float velocity = 0.45f;
};

Params defaultParams();
bool isKnownParam (const std::string& id);
float getParam (const Params& params, const std::string& id);
void setParam (Params& params, const std::string& id, float value);

float clampToSpec (const std::string& id, float value);
} // namespace cs80
