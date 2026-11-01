#pragma once

#include "../PluginProcessor.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace cs80::ui
{
/** Keyboard-accessible secondary panel for modulation, Scala tuning and user
    preset management. All file dialogs and disk operations run on the message
    thread; audio is handled by the lock-free engine snapshot path. */
class AdvancedPanel final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit AdvancedPanel (Cs80AudioProcessor&);
    ~AdvancedPanel() override;

    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void showPageForSnapshot (int page);

private:
    enum class Page { modulation, tuning, presets };

    struct RouteControls
    {
        juce::Label number;
        juce::ComboBox source, destination;
        juce::Slider amount;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destinationAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    };

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int row) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;

    void setPage (Page);
    void refreshPresetFiles();
    juce::File selectedPreset() const;
    juce::File namedPreset() const;
    void savePreset();
    void loadPreset();
    void renamePreset();
    void deletePreset();
    void importPreset();
    void exportPreset();
    void chooseTuningFile (bool keyboardMap);
    void setStatus (const juce::String&, bool isError = false);

    Cs80AudioProcessor& processor;
    Page currentPage = Page::modulation;

    juce::TextButton closeButton { "CLOSE" };
    juce::TextButton modTab { "MOD MATRIX" }, tuningTab { "MICROTUNING" }, presetTab { "USER PRESETS" };
    juce::Label status;

    std::array<RouteControls, pid::kModSlots> routes;

    juce::Label tuningName;
    juce::TextButton loadScaleButton { "LOAD .SCL SCALE" };
    juce::TextButton loadMappingButton { "LOAD .KBM MAPPING" };
    juce::TextButton resetTuningButton { "RESET TO 12-TET" };

    juce::Label searchCaption, nameCaption;
    juce::TextEditor searchBox, nameBox;
    juce::ListBox userPresetList;
    juce::Label emptyBankMessage;
    juce::TextButton saveButton { "SAVE" }, loadButton { "LOAD" };
    juce::TextButton renameButton { "RENAME" }, deleteButton { "DELETE" };
    juce::TextButton importButton { "IMPORT" }, exportButton { "EXPORT" };
    std::vector<juce::File> userPresetFiles;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPanel)
};
} // namespace cs80::ui
