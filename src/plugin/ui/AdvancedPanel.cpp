#include "AdvancedPanel.h"

#include <algorithm>
#include <iterator>

namespace cs80::ui
{
namespace
{
const juce::Colour background { 0xff16130f };
const juce::Colour panel { 0xff272018 };
const juce::Colour accent { 0xffd5a86b };
const juce::Colour text { 0xfff4e6d3 };
const juce::Colour muted { 0xffb5a18b };

void styleButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, panel);
    button.setColour (juce::TextButton::buttonOnColourId, accent.darker (0.4f));
    button.setColour (juce::TextButton::textColourOffId, text);
    button.setColour (juce::TextButton::textColourOnId, text);
}

void styleLabel (juce::Label& label, float size = 15.0f)
{
    label.setColour (juce::Label::textColourId, text);
    label.setFont (juce::Font (size));
}
} // namespace

AdvancedPanel::AdvancedPanel (Cs80AudioProcessor& owner)
    : processor (owner)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    for (auto* button : { &closeButton, &modTab, &tuningTab, &presetTab,
                          &loadScaleButton, &loadMappingButton, &resetTuningButton,
                          &saveButton, &loadButton, &renameButton, &deleteButton,
                          &importButton, &exportButton })
    {
        addAndMakeVisible (*button);
        styleButton (*button);
    }

    closeButton.onClick = [this] { if (onClose) onClose(); };
    modTab.onClick = [this] { setPage (Page::modulation); };
    tuningTab.onClick = [this] { setPage (Page::tuning); };
    presetTab.onClick = [this] { setPage (Page::presets); };

    addAndMakeVisible (status);
    styleLabel (status, 15.0f);
    status.setJustificationType (juce::Justification::centredLeft);

    static const char* sourceNames[] = {
        "Off", "LFO", "Velocity", "Channel Pressure", "Poly Pressure", "Mod Wheel",
        "Key Track", "Filter Env", "Amp Env", "Random"
    };
    static const char* destinationNames[] = {
        "Off", "Pitch", "Pulse Width", "Filter Cutoff", "Resonance",
        "Amplitude", "Pan", "Drive", "Layer Level"
    };

    for (int i = 0; i < pid::kModSlots; ++i)
    {
        auto& route = routes[(size_t) i];
        addAndMakeVisible (route.number);
        addAndMakeVisible (route.source);
        addAndMakeVisible (route.destination);
        addAndMakeVisible (route.amount);

        route.number.setText (juce::String (i + 1).paddedLeft ('0', 2), juce::dontSendNotification);
        styleLabel (route.number, 18.0f);

        for (int index = 0; index < (int) std::size (sourceNames); ++index)
            route.source.addItem (sourceNames[index], index + 1);
        for (int index = 0; index < (int) std::size (destinationNames); ++index)
            route.destination.addItem (destinationNames[index], index + 1);

        route.source.setTooltip ("Modulation source for route " + juce::String (i + 1));
        route.destination.setTooltip ("Modulation destination for route " + juce::String (i + 1));
        route.amount.setTooltip ("Bipolar amount from -100% to +100%");
        route.amount.setSliderStyle (juce::Slider::LinearHorizontal);
        route.amount.setTextBoxStyle (juce::Slider::TextBoxRight, false, 74, 25);

        route.sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, pid::modSourceIds[i], route.source);
        route.destinationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, pid::modDestIds[i], route.destination);
        route.amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, pid::modAmountIds[i], route.amount);
    }

    addAndMakeVisible (tuningName);
    styleLabel (tuningName, 20.0f);
    tuningName.setJustificationType (juce::Justification::centredLeft);
    loadScaleButton.onClick = [this] { chooseTuningFile (false); };
    loadMappingButton.onClick = [this] { chooseTuningFile (true); };
    resetTuningButton.onClick = [this]
    {
        processor.resetMicrotuning();
        tuningName.setText ("Current: " + processor.tuningDescription(), juce::dontSendNotification);
        setStatus ("Standard tuning restored.");
    };

    for (auto* label : { &searchCaption, &nameCaption })
    {
        addAndMakeVisible (*label);
        styleLabel (*label);
    }

    searchCaption.setText ("SEARCH BANK", juce::dontSendNotification);
    nameCaption.setText ("PRESET NAME", juce::dontSendNotification);
    addAndMakeVisible (searchBox);
    addAndMakeVisible (nameBox);
    searchBox.setTextToShowWhenEmpty ("Filter saved patches", muted);
    nameBox.setTextToShowWhenEmpty ("New patch name", muted);
    searchBox.onTextChange = [this] { refreshPresetFiles(); };

    addAndMakeVisible (userPresetList);
    userPresetList.setModel (this);
    userPresetList.setRowHeight (38);
    userPresetList.setColour (juce::ListBox::backgroundColourId, panel);
    userPresetList.setColour (juce::ListBox::outlineColourId, accent.withAlpha (0.34f));

    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPreset(); };
    renameButton.onClick = [this] { renamePreset(); };
    deleteButton.onClick = [this] { deletePreset(); };
    importButton.onClick = [this] { importPreset(); };
    exportButton.onClick = [this] { exportPreset(); };

    refreshPresetFiles();
    setPage (Page::modulation);
}

AdvancedPanel::~AdvancedPanel()
{
    chooser.reset();
    userPresetList.setModel (nullptr);
}

void AdvancedPanel::setPage (Page page)
{
    currentPage = page;

    for (auto& route : routes)
    {
        const bool visible = page == Page::modulation;
        route.number.setVisible (visible);
        route.source.setVisible (visible);
        route.destination.setVisible (visible);
        route.amount.setVisible (visible);
    }

    juce::Component* tuningComponents[] = {
        &tuningName, &loadScaleButton, &loadMappingButton, &resetTuningButton
    };
    for (auto* component : tuningComponents)
        component->setVisible (page == Page::tuning);

    juce::Component* presetComponents[] = {
        &searchCaption, &nameCaption, &searchBox, &nameBox, &userPresetList,
        &saveButton, &loadButton, &renameButton, &deleteButton, &importButton, &exportButton
    };
    for (auto* component : presetComponents)
        component->setVisible (page == Page::presets);

    modTab.setToggleState (page == Page::modulation, juce::dontSendNotification);
    tuningTab.setToggleState (page == Page::tuning, juce::dontSendNotification);
    presetTab.setToggleState (page == Page::presets, juce::dontSendNotification);
    tuningName.setText ("Current: " + processor.tuningDescription(), juce::dontSendNotification);
    setStatus (page == Page::modulation ? "All eight routes are host-automatable and saved with the session."
                 : page == Page::tuning ? "Scala scales and keyboard maps are saved with the plugin state."
                                           : "User patches live in your account's CS-80E/Presets folder.");
    repaint();
}

void AdvancedPanel::showPageForSnapshot (int page)
{
    setPage (page == 1 ? Page::tuning : page == 2 ? Page::presets : Page::modulation);
}

void AdvancedPanel::setStatus (const juce::String& message, bool error)
{
    status.setColour (juce::Label::textColourId,
                      error ? juce::Colour (0xffee9c8b) : muted);
    status.setText (message, juce::dontSendNotification);
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    g.fillAll (background);
    const auto panelBounds = getLocalBounds().reduced (56, 36).toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (panelBounds, 14.0f);
    g.setColour (accent.withAlpha (0.63f));
    g.drawRoundedRectangle (panelBounds.reduced (1.0f), 14.0f, 2.0f);

    g.setColour (text);
    g.setFont (juce::Font (28.0f));
    g.drawText ("CS-80E  /  ADVANCED", 90, 54, 550, 40, juce::Justification::centredLeft);
    g.setColour (accent.withAlpha (0.55f));
    g.drawLine (86.0f, 162.0f, (float) getWidth() - 86.0f, 162.0f, 1.2f);

    g.setColour (muted);
    g.setFont (juce::Font (17.0f));
    if (currentPage == Page::modulation)
    {
        g.drawText ("ROUTE", 115, 186, 130, 30, juce::Justification::centredLeft);
        g.drawText ("SOURCE", 240, 186, 300, 30, juce::Justification::centredLeft);
        g.drawText ("DESTINATION", 588, 186, 300, 30, juce::Justification::centredLeft);
        g.drawText ("BIPOLAR DEPTH", 946, 186, 280, 30, juce::Justification::centredLeft);
    }
    else if (currentPage == Page::tuning)
    {
        g.drawFittedText ("LOAD A SCALA SCALE, OPTIONALLY FOLLOWED BY ITS KEYBOARD MAP.",
                          100, 210, 1120, 32, juce::Justification::centredLeft, 1);
        g.drawFittedText ("The active tuning follows your DAW project and saved user patches."
                          " Factory patches remain unchanged.",
                          100, 460, 1120, 40, juce::Justification::centredLeft, 2);
    }
    else
    {
        g.drawText ("DOUBLE-CLICK A PATCH TO LOAD IT", 102, 230, 820, 30,
                    juce::Justification::centredLeft);
    }
}

void AdvancedPanel::resized()
{
    closeButton.setBounds (1250, 76, 92, 36);
    modTab.setBounds (90, 116, 160, 37);
    tuningTab.setBounds (258, 116, 170, 37);
    presetTab.setBounds (436, 116, 170, 37);
    status.setBounds (96, 846, 1200, 32);

    for (int i = 0; i < pid::kModSlots; ++i)
    {
        auto& route = routes[(size_t) i];
        const int y = 232 + i * 73;
        route.number.setBounds (112, y, 85, 43);
        route.source.setBounds (235, y, 303, 43);
        route.destination.setBounds (585, y, 303, 43);
        route.amount.setBounds (940, y, 300, 43);
    }

    tuningName.setBounds (105, 280, 1090, 44);
    loadScaleButton.setBounds (105, 364, 260, 54);
    loadMappingButton.setBounds (380, 364, 270, 54);
    resetTuningButton.setBounds (665, 364, 270, 54);

    searchCaption.setBounds (105, 180, 150, 30);
    searchBox.setBounds (252, 180, 390, 32);
    nameCaption.setBounds (674, 180, 160, 30);
    nameBox.setBounds (824, 180, 393, 32);
    userPresetList.setBounds (100, 268, 1130, 448);
    saveButton.setBounds (100, 740, 175, 47);
    loadButton.setBounds (285, 740, 175, 47);
    renameButton.setBounds (470, 740, 175, 47);
    deleteButton.setBounds (655, 740, 175, 47);
    importButton.setBounds (840, 740, 175, 47);
    exportButton.setBounds (1025, 740, 175, 47);
}

bool AdvancedPanel::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose)
            onClose();
        return true;
    }
    return false;
}

int AdvancedPanel::getNumRows()
{
    return (int) userPresetFiles.size();
}

void AdvancedPanel::paintListBoxItem (int row, juce::Graphics& g, int width,
                                      int height, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    g.fillAll (selected ? accent.withAlpha (0.26f) : (row & 1) ? panel : background);
    g.setColour (selected ? juce::Colours::white : text);
    g.setFont (juce::Font (17.0f));
    g.drawText (userPresetFiles[(size_t) row].getFileNameWithoutExtension(),
                18, 0, width - 36, height, juce::Justification::centredLeft);
}

void AdvancedPanel::selectedRowsChanged (int row)
{
    if (row >= 0 && row < getNumRows())
        nameBox.setText (userPresetFiles[(size_t) row].getFileNameWithoutExtension(), false);
}

void AdvancedPanel::listBoxItemDoubleClicked (int, const juce::MouseEvent&)
{
    loadPreset();
}

void AdvancedPanel::refreshPresetFiles()
{
    userPresetFiles.clear();
    juce::Array<juce::File> found;
    const auto directory = Cs80AudioProcessor::userPresetDirectory();
    directory.findChildFiles (found, juce::File::findFiles, false, "*.cs80e");

    for (const auto& file : found)
        if (file.getFileNameWithoutExtension().containsIgnoreCase (searchBox.getText()))
            userPresetFiles.push_back (file);

    std::sort (userPresetFiles.begin(), userPresetFiles.end(),
               [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });

    userPresetList.updateContent();
    userPresetList.repaint();
}

juce::File AdvancedPanel::selectedPreset() const
{
    const int selected = userPresetList.getSelectedRow();
    return selected >= 0 && selected < (int) userPresetFiles.size()
             ? userPresetFiles[(size_t) selected] : juce::File();
}

juce::File AdvancedPanel::namedPreset() const
{
    const auto name = juce::File::createLegalFileName (nameBox.getText().trim());
    if (name.isEmpty())
        return {};
    return Cs80AudioProcessor::userPresetDirectory().getChildFile (name + ".cs80e");
}

void AdvancedPanel::savePreset()
{
    const auto file = namedPreset();
    if (file == juce::File())
        return setStatus ("Enter a preset name before saving.", true);
    if (file.exists())
        return setStatus ("That name already exists. Choose a new name or rename the old patch.", true);

    if (auto result = file.getParentDirectory().createDirectory(); result.failed())
        return setStatus ("Could not create the user preset directory.", true);

    juce::String error;
    if (! processor.saveUserPreset (file, error))
        return setStatus (error, true);

    refreshPresetFiles();
    setStatus ("Saved " + file.getFileNameWithoutExtension() + ".");
}

void AdvancedPanel::loadPreset()
{
    const auto file = selectedPreset();
    if (! file.existsAsFile())
        return setStatus ("Select a saved preset first.", true);

    juce::String error;
    if (! processor.loadUserPreset (file, error))
        return setStatus (error, true);
    setStatus ("Loaded " + file.getFileNameWithoutExtension() + ".");
}

void AdvancedPanel::renamePreset()
{
    const auto original = selectedPreset();
    const auto renamed = namedPreset();
    if (! original.existsAsFile() || renamed == juce::File())
        return setStatus ("Select a preset and enter its new name.", true);
    if (original == renamed)
        return setStatus ("The preset already has that name.");
    if (renamed.exists())
        return setStatus ("A preset with that name already exists.", true);
    if (! original.moveFileTo (renamed))
        return setStatus ("Could not rename the preset file.", true);
    refreshPresetFiles();
    setStatus ("Renamed to " + renamed.getFileNameWithoutExtension() + ".");
}

void AdvancedPanel::deletePreset()
{
    const auto file = selectedPreset();
    if (! file.existsAsFile())
        return setStatus ("Select a preset to delete.", true);

    const juce::Component::SafePointer<AdvancedPanel> safeThis (this);
    const auto options = juce::MessageBoxOptions::makeOptionsOkCancel (
        juce::MessageBoxIconType::WarningIcon, "Delete user preset?",
        "Delete '" + file.getFileNameWithoutExtension() + "'? This cannot be undone.",
        "DELETE", "CANCEL", this);

    juce::AlertWindow::showAsync (options, [safeThis, file] (int result)
    {
        if (result != 1 || safeThis == nullptr)
            return;

        if (! file.deleteFile())
            safeThis->setStatus ("Could not delete the selected preset.", true);
        else
        {
            safeThis->refreshPresetFiles();
            safeThis->setStatus ("Deleted " + file.getFileNameWithoutExtension() + ".");
        }
    });
}

void AdvancedPanel::importPreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Import CS-80E preset", juce::File(), "*.cs80e");
    const juce::Component::SafePointer<AdvancedPanel> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis] (const juce::FileChooser& dialog)
    {
        if (safeThis == nullptr)
            return;
        const auto source = dialog.getResult();
        if (! source.existsAsFile())
            return;
        juce::String error;
        if (! safeThis->processor.loadUserPreset (source, error))
            return safeThis->setStatus (error, true);

        const auto directory = Cs80AudioProcessor::userPresetDirectory();
        if (directory.createDirectory().failed())
            return safeThis->setStatus ("Could not create the preset directory.", true);
        const auto destination = directory.getNonexistentChildFile (
            juce::File::createLegalFileName (source.getFileNameWithoutExtension()), ".cs80e");
        if (! source.copyFileTo (destination))
            return safeThis->setStatus ("Could not copy the imported preset.", true);
        safeThis->refreshPresetFiles();
        safeThis->setStatus ("Imported " + destination.getFileNameWithoutExtension() + ".");
    });
}

void AdvancedPanel::exportPreset()
{
    const auto source = selectedPreset();
    if (! source.existsAsFile())
        return setStatus ("Select a preset to export.", true);

    chooser = std::make_unique<juce::FileChooser> (
        "Export CS-80E preset", source, "*.cs80e");
    const juce::Component::SafePointer<AdvancedPanel> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                          | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safeThis, source] (const juce::FileChooser& dialog)
    {
        if (safeThis == nullptr)
            return;
        auto destination = dialog.getResult();
        if (destination == juce::File())
            return;
        destination = destination.withFileExtension (".cs80e");
        if (destination == source)
            return safeThis->setStatus ("This preset is already stored there.");
        safeThis->setStatus (source.copyFileTo (destination) ? "Preset exported."
                                    : "Could not export the preset.", ! destination.existsAsFile());
    });
}

void AdvancedPanel::chooseTuningFile (bool keyboardMap)
{
    chooser = std::make_unique<juce::FileChooser> (
        keyboardMap ? "Load Scala keyboard map" : "Load Scala scale",
        juce::File(), keyboardMap ? "*.kbm" : "*.scl");

    const juce::Component::SafePointer<AdvancedPanel> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis, keyboardMap] (const juce::FileChooser& dialog)
    {
        if (safeThis == nullptr)
            return;
        const auto file = dialog.getResult();
        if (! file.existsAsFile())
            return;
        if (file.getSize() > 128 * 1024)
            return safeThis->setStatus ("Scale/map files must be smaller than 128 KB.", true);

        juce::String error;
        const auto source = file.loadFileAsString();
        const bool ok = keyboardMap ? safeThis->processor.loadScalaMapping (source, error)
                                    : safeThis->processor.loadScalaScale (source, error);
        if (! ok)
            return safeThis->setStatus (error, true);
        safeThis->tuningName.setText ("Current: " + safeThis->processor.tuningDescription(),
                                      juce::dontSendNotification);
        safeThis->setStatus ("Loaded " + file.getFileName() + ".");
    });
}
} // namespace cs80::ui
