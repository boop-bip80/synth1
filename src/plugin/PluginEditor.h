#pragma once

#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/Performance.h"
#include "ui/PresetUI.h"
#include "ui/Widgets.h"

#include <memory>

namespace cs80
{
class Cs80Editor : public juce::AudioProcessorEditor
{
public:
    explicit Cs80Editor (Cs80AudioProcessor&);
    ~Cs80Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void openPresetBrowser();
    void closePresetBrowser();
    bool isPresetBrowserOpen() const;
    void setPresetBrowserHoverForSnapshot (int row);
    void refreshForSnapshot();

    static constexpr int designWidth = 1360;
    static constexpr int designHeight = 880;

private:
    class Content;

    Cs80AudioProcessor& processor;
    ui::Cs80LookAndFeel lookAndFeel;
    std::unique_ptr<Content> content;
    std::unique_ptr<juce::TooltipWindow> tooltip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cs80Editor)
};
} // namespace cs80
