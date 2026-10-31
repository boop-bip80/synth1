#pragma once

#include "Params.h"

#include <string>
#include <vector>

namespace cs80
{
struct PresetOverride
{
    const char* id = "";
    float value = 0.0f;
};

struct Preset
{
    const char* name = "";
    const char* category = "";
    const char* blurb = "";
    std::vector<PresetOverride> values;
};

const std::vector<Preset>& factoryPresets();
void applyPreset (Params& params, const Preset& preset);
const Preset* findPreset (const std::string& name);
} // namespace cs80
