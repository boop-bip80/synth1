#include "PluginEditor.h"

#include "../dsp/Params.h"

namespace cs80
{
namespace
{
constexpr int headerDividerY = 112;
constexpr int mainRowY = 118;
constexpr int mainRowH = 352;
constexpr int lowerRowY = 480;
constexpr int lowerRowH = 136;

juce::Rectangle<int> mainSection (int x, int width)
{
    return { x, mainRowY, width, mainRowH };
}

juce::Rectangle<int> lowerSection (int x, int width)
{
    return { x, lowerRowY, width, lowerRowH };
}
} // namespace

//==============================================================================
class Cs80Editor::Content : public juce::Component
{
public:
    Content (Cs80Editor& ownerIn, Cs80AudioProcessor& processorIn)
        : owner (ownerIn),
          processor (processorIn),
          presetDisplay (processorIn),
          scope (processorIn.getEngine()),
          voiceLeds (processorIn.getEngine()),
          ribbon (processorIn),
          keyboard (processorIn.keyboardState)
    {
        setOpaque (true);
        setSize (designWidth, designHeight);

        addAndMakeVisible (brand);
        addAndMakeVisible (presetDisplay);
        addAndMakeVisible (scope);
        addAndMakeVisible (voiceLeds);
        addAndMakeVisible (ribbon);
        addAndMakeVisible (keyboard);

        presetDisplay.onOpenBrowser = [this] { owner.openPresetBrowser(); };

        buildSections();
        buildControls();
    }

    void paint (juce::Graphics& g) override
    {
        const auto full = getLocalBounds();
        const juce::Rectangle<float> face (38.0f, 12.0f, 1204.0f, 756.0f);

        ui::drawWoodFrame (g, full);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (face.expanded (6.0f).translated (0.0f, 3.0f), 12.0f);

        ui::drawBrushedPanel (g, face.toNearestInt());

        // Warm light falling from the top edge.
        g.setGradientFill (juce::ColourGradient (ui::colour::amber.withAlpha (0.035f),
                                                 face.getCentreX(), face.getY(),
                                                 juce::Colours::transparentBlack,
                                                 face.getCentreX(), face.getY() + 320.0f, false));
        g.fillRoundedRectangle (face, 8.0f);

        // Bevel and inner hairline.
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (face.reduced (0.6f), 8.0f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.80f));
        g.drawRoundedRectangle (face.reduced (1.6f), 7.4f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawRoundedRectangle (face.reduced (8.0f), 5.0f, 1.0f);

        // Edge vignette.
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f),
                                                 0.0f, 0.0f,
                                                 juce::Colours::transparentBlack,
                                                 38.0f, 0.0f, false));
        g.fillRect (0, 0, 38, 780);
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                 1204.0f, 0.0f,
                                                 juce::Colours::black.withAlpha (0.35f),
                                                 1280.0f, 0.0f, false));
        g.fillRect (1204, 0, 76, 780);

        // Corner screws.
        for (juce::Point<float> screw : { juce::Point<float> (26.0f, 26.0f),
                                          juce::Point<float> (1254.0f, 26.0f),
                                          juce::Point<float> (26.0f, 754.0f),
                                          juce::Point<float> (1254.0f, 754.0f),
                                          juce::Point<float> (26.0f, 390.0f),
                                          juce::Point<float> (1254.0f, 390.0f) })
            ui::drawScrew (g, screw, 5.2f);

        // Header rule.
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (60.0f, (float) headerDividerY, 1220.0f, (float) headerDividerY, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawLine (60.0f, (float) headerDividerY + 1.0f, 1220.0f, (float) headerDividerY + 1.0f, 1.0f);

        // Warm bloom behind the display and scope.
        ui::drawGlow (g, { 591.0f, 62.0f }, 250.0f, ui::colour::amber, 0.10f);
        ui::drawGlow (g, { 903.0f, 62.0f }, 130.0f, ui::colour::amber, 0.08f);

        // Voice activity caption and power lamp.
        ui::drawTrackedText (g, "VOICE ACTIVITY",
                             { 994, 25, 136, 10 }, ui::colour::textDim,
                             juce::Justification::centred, ui::trackedFont (6.2f, 1.4f, true), 0.3f);

        ui::drawLed (g, { 1188.0f, 42.0f, 9.0f, 9.0f }, true, ui::colour::amber, 0.95f);
        ui::drawTrackedText (g, "POWER", { 1170, 55, 46, 9 }, ui::colour::textDim,
                             juce::Justification::centred, ui::trackedFont (5.8f, 1.0f, true), 0.2f);

        // Footer engraving.
        ui::drawTrackedText (g, "CS-80E  /  EIGHT VOICE POLYPHONIC SYNTHESIZER",
                             { 60, 746, 460, 16 }, ui::colour::textFaint,
                             juce::Justification::centredLeft, ui::trackedFont (7.0f, 1.8f, true), 0.4f);

        ui::drawTrackedText (g, "DUAL VCO  ·  RING MOD  ·  HPF / LPF  ·  ENSEMBLE",
                             { 520, 746, 360, 16 }, ui::colour::textFaint.brighter (0.18f),
                             juce::Justification::centred, ui::monoFont (6.6f), 0.3f);

        ui::drawTrackedText (g, "SERIAL NO. 00108  /  V1.0.0",
                             { 940, 746, 280, 16 }, ui::colour::textFaint.brighter (0.18f),
                             juce::Justification::centredRight, ui::monoFont (6.6f), 0.3f);
    }

    void resized() override
    {
        brand.setBounds (62, 30, 300, 92);
        presetDisplay.setBounds (376, 30, 430, 64);
        scope.setBounds (824, 32, 158, 58);
        voiceLeds.setBounds (996, 40, 132, 26);
        ribbon.setBounds (60, 620, 1160, 34);
        keyboard.setBounds (60, 660, 1160, 82);
    }

    void addBrowser (std::unique_ptr<ui::PresetBrowser> newBrowser)
    {
        browser = std::move (newBrowser);

        if (browser != nullptr)
        {
            addAndMakeVisible (*browser);
            browser->setBounds (getLocalBounds());
            browser->toFront (true);
        }
    }

    void removeBrowser()
    {
        if (browser != nullptr)
        {
            removeChildComponent (browser.get());
            browser.reset();
        }
    }

    ui::PresetBrowser* getBrowser() { return browser.get(); }

    void repaintAll()
    {
        repaint();
        repaintDynamicChildren();
    }

    void repaintDynamicChildren()
    {
        for (auto* child : getChildren())
            child->repaint();
    }

private:
    void buildSections()
    {
        addSection ("VCO 1", "SAW / OCTAVE / DETUNE", mainSection (60, 140));
        addSection ("VCO 2", "PULSE / PWM", mainSection (210, 155));
        addSection ("RING", "MODULATOR / NOISE", mainSection (375, 118));
        addSection ("FILTER", "12 DB / OCT / RESONANT", mainSection (503, 252));
        addSection ("FILTER EG", "INITIAL / ATTACK LEVEL", mainSection (765, 245));
        addSection ("AMP EG", "ADSR", mainSection (1020, 200));

        addSection ("LFO", "MODULATION", lowerSection (60, 430));
        addSection ("PERFORMANCE", "TOUCH / BEND / SPREAD", lowerSection (500, 330));
        addSection ("OUTPUT", "MASTER / FX / VU", lowerSection (840, 380));
    }

    void addSection (const juce::String& title, const juce::String& subtitle, juce::Rectangle<int> bounds)
    {
        auto section = std::make_unique<ui::SectionPanel> (title, subtitle);
        section->setBounds (bounds);
        addAndMakeVisible (*section);
        sections.push_back (std::move (section));
    }

    void buildControls()
    {
        using B = juce::Rectangle<int>;

        auto fader = [this] (const char* id, B bounds, const char* labelOverride = nullptr)
        {
            auto control = std::make_unique<ui::ParamFader> (processor.apvts, id);

            if (labelOverride != nullptr)
                control->setLabelOverride (labelOverride);

            control->setBounds (bounds);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        };

        auto knob = [this] (const char* id, B bounds, int diameter, const char* labelOverride = nullptr)
        {
            auto control = std::make_unique<ui::ParamKnob> (processor.apvts, id, diameter);

            if (labelOverride != nullptr)
                control->setLabelOverride (labelOverride);

            control->setBounds (bounds);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        };

        auto choice = [this] (const char* id, B bounds)
        {
            auto control = std::make_unique<ui::ParamChoice> (processor.apvts, id);
            control->setBounds (bounds);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        };

        // VCO 1
        fader (pid::vco1Level, { 68, 152, 56, 296 }, "LEVEL");
        knob (pid::vco1Octave, { 126, 166, 70, 82 }, 38);
        knob (pid::vco1Detune, { 126, 268, 70, 82 }, 38);

        // VCO 2
        fader (pid::vco2Level, { 218, 150, 50, 206 }, "LEVEL");
        fader (pid::vco2Pw, { 270, 150, 50, 206 }, "PW");
        knob (pid::vco2Octave, { 218, 372, 64, 78 }, 36, "OCT");
        knob (pid::vco2Detune, { 284, 372, 64, 78 }, 36);

        // Ring modulator / noise
        knob (pid::ring, { 385, 166, 98, 96 }, 52, "RING MOD");
        knob (pid::noise, { 385, 300, 98, 96 }, 52);

        // Filters
        fader (pid::hpfCutoff, { 511, 152, 54, 296 }, "HP CUTOFF");
        knob (pid::hpfRes, { 567, 158, 54, 84 }, 38, "HP RES");
        fader (pid::lpfCutoff, { 625, 152, 54, 296 }, "LP CUTOFF");
        knob (pid::lpfRes, { 681, 158, 54, 84 }, 38, "LP RES");

        // Filter envelope
        const char* filterIds[] = { pid::fIl, pid::fAl, pid::fA, pid::fD, pid::fS, pid::fR };
        const char* filterLabels[] = { "IL", "AL", "ATK", "DEC", "SUS", "REL" };

        for (int i = 0; i < 6; ++i)
            fader (filterIds[i], { 769 + i * 39, 152, 37, 296 }, filterLabels[i]);

        // Amp envelope
        const char* ampIds[] = { pid::aA, pid::aD, pid::aS, pid::aR };

        for (int i = 0; i < 4; ++i)
            fader (ampIds[i], { 1026 + i * 48, 152, 45, 296 });

        // LFO
        knob (pid::lfoRate, { 70, 508, 54, 92 }, 44, "SPEED");
        choice (pid::lfoWave, { 126, 508, 68, 92 });
        knob (pid::lfoDelay, { 196, 508, 54, 92 }, 44, "DELAY");
        knob (pid::lfoPitch, { 252, 508, 54, 92 }, 44, "PITCH");
        knob (pid::lfoPwm, { 308, 508, 54, 92 }, 44, "PWM");
        knob (pid::lfoFilter, { 364, 508, 54, 92 }, 44, "FILTER");
        knob (pid::lfoAmp, { 420, 508, 54, 92 }, 44, "TREM");

        // Performance
        knob (pid::aftertouch, { 510, 508, 72, 92 }, 44);
        knob (pid::bendRange, { 586, 508, 72, 92 }, 44, "BEND");
        knob (pid::spread, { 662, 508, 72, 92 }, 44);
        knob (pid::velocity, { 738, 508, 72, 92 }, 44);

        // Output
        knob (pid::master, { 852, 508, 68, 92 }, 46);
        knob (pid::brilliance, { 922, 508, 68, 92 }, 44, "BRILL");
        knob (pid::ensemble, { 992, 508, 68, 92 }, 44);
        knob (pid::space, { 1062, 508, 68, 92 }, 44);

        auto meter = std::make_unique<ui::MeterDisplay> (processor.getEngine());
        meter->setBounds (1140, 504, 30, 96);
        addAndMakeVisible (*meter);
        controls.push_back (std::move (meter));
    }

    Cs80Editor& owner;
    Cs80AudioProcessor& processor;

    ui::BrandPlate brand;
    ui::PresetDisplay presetDisplay;
    ui::ScopeDisplay scope;
    ui::VoiceLedStrip voiceLeds;
    ui::RibbonStrip ribbon;
    ui::Cs80Keyboard keyboard;

    std::vector<std::unique_ptr<ui::SectionPanel>> sections;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::unique_ptr<ui::PresetBrowser> browser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Content)
};

//==============================================================================
Cs80Editor::Cs80Editor (Cs80AudioProcessor& processorToEdit)
    : AudioProcessorEditor (&processorToEdit), processor (processorToEdit)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setWantsKeyboardFocus (true);

    content = std::make_unique<Content> (*this, processor);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (960, 585, 2560, 1560);

    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) designWidth / (double) designHeight);

    setSize (designWidth, designHeight);

    tooltip = std::make_unique<juce::TooltipWindow> (this, 700);
}

Cs80Editor::~Cs80Editor()
{
    tooltip.reset();
    closePresetBrowser();
    setLookAndFeel (nullptr);
}

void Cs80Editor::paint (juce::Graphics& g)
{
    g.fillAll (ui::colour::windowBg);
}

void Cs80Editor::resized()
{
    if (content == nullptr)
        return;

    const float scale = juce::jmin ((float) getWidth() / (float) designWidth,
                                    (float) getHeight() / (float) designHeight);
    const float offsetX = ((float) getWidth() - (float) designWidth * scale) * 0.5f;
    const float offsetY = ((float) getHeight() - (float) designHeight * scale) * 0.5f;

    content->setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));
}

void Cs80Editor::openPresetBrowser()
{
    if (content == nullptr)
        return;

    if (auto* existing = content->getBrowser())
    {
        existing->toFront (true);
        return;
    }

    auto newBrowser = std::make_unique<ui::PresetBrowser> (processor);
    auto* browserPtr = newBrowser.get();

    browserPtr->onClose = [this] { closePresetBrowser(); };
    browserPtr->onPresetLoaded = [this]
    {
        if (content != nullptr)
            content->repaint();
    };

    content->addBrowser (std::move (newBrowser));
    browserPtr->grabKeyboardFocus();
}

void Cs80Editor::closePresetBrowser()
{
    if (content == nullptr || content->getBrowser() == nullptr)
        return;

    // The close request usually originates inside the browser itself, so never
    // destroy it while it is still on the stack.
    juce::Component::SafePointer<Cs80Editor> safeThis (this);

    juce::MessageManager::callAsync ([safeThis]
    {
        if (safeThis != nullptr && safeThis->content != nullptr)
            safeThis->content->removeBrowser();
    });
}

bool Cs80Editor::isPresetBrowserOpen() const
{
    return content != nullptr && content->getBrowser() != nullptr;
}

void Cs80Editor::setPresetBrowserHoverForSnapshot (int row)
{
    if (content != nullptr)
        if (auto* browser = content->getBrowser())
            browser->setHoverRowForSnapshot (row);
}

void Cs80Editor::refreshForSnapshot()
{
    if (content != nullptr)
        content->repaintAll();
}

bool Cs80Editor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && content != nullptr && content->getBrowser() != nullptr)
    {
        closePresetBrowser();
        return true;
    }

    return false;
}
} // namespace cs80
