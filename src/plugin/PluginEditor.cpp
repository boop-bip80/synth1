#include "PluginEditor.h"

#include "../dsp/Params.h"

#include <cmath>

namespace cs80
{
namespace
{
constexpr int headerDividerY = 154;
constexpr int mainRowY = 164;
constexpr int mainRowH = 392;
constexpr int lowerRowY = 568;
constexpr int lowerRowH = 144;

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
        const juce::Rectangle<float> face (40.0f, 10.0f, 1280.0f, 860.0f);

        g.fillAll (ui::colour::windowBg);
        ui::drawWoodFrame (g, full);

        // Shadow and metal face.
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (face.expanded (5.0f).translated (0.0f, 4.0f), 10.0f);

        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path faceClip;
            faceClip.addRoundedRectangle (face, 7.0f);
            g.reduceClipRegion (faceClip);

            ui::drawBrushedPanel (g, face.toNearestInt());

            // A brass trim cap along the top edge of the face.
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc79a5b),
                                                     face.getX(), face.getY(),
                                                     juce::Colour (0xff5a3c1b),
                                                     face.getRight(), face.getY(), false));
            g.fillRect (juce::Rectangle<float> (face.getX() + 6.0f, face.getY() + 0.5f,
                                                face.getWidth() - 12.0f, 2.0f));

            // Warm bloom behind the display and scope.
            ui::drawGlow (g, { 620.0f, 74.0f }, 270.0f, ui::colour::amber, 0.10f);
            ui::drawGlow (g, { 970.0f, 68.0f }, 140.0f, ui::colour::amber, 0.075f);

            // Side vignettes.
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.42f),
                                                     face.getX(), 0.0f,
                                                     juce::Colours::transparentBlack,
                                                     face.getX() + 46.0f, 0.0f, false));
            g.fillRect (face.withWidth (46.0f));

            g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                     face.getRight() - 46.0f, 0.0f,
                                                     juce::Colours::black.withAlpha (0.42f),
                                                     face.getRight(), 0.0f, false));
            g.fillRect (face.withTrimmedLeft (face.getWidth() - 46.0f));

            // Bottom bounce.
            g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                     face.getCentreX(), face.getBottom() - 90.0f,
                                                     juce::Colours::black.withAlpha (0.30f),
                                                     face.getCentreX(), face.getBottom(), false));
            g.fillRect (face.withTop (face.getBottom() - 90.0f));
        }

        // Bevels.
        g.setColour (juce::Colours::white.withAlpha (0.11f));
        g.drawRoundedRectangle (face.reduced (0.7f), 7.5f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawRoundedRectangle (face.reduced (1.8f), 7.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawRoundedRectangle (face.reduced (9.0f), 4.0f, 0.9f);

        // Brass corner brackets.
        {
            const float inset = 20.0f;
            const float len = 26.0f;
            g.setColour (juce::Colour (0xffb3874f).withAlpha (0.28f));

            for (int corner = 0; corner < 4; ++corner)
            {
                const bool right = (corner & 1) != 0;
                const bool bottom = corner >= 2;
                const float x = right ? face.getRight() - inset : face.getX() + inset;
                const float y = bottom ? face.getBottom() - inset : face.getY() + inset;
                const float dx = right ? -len : len;
                const float dy = bottom ? -len : len;

                g.drawLine (x, y, x + dx, y, 1.0f);
                g.drawLine (x, y, x, y + dy, 1.0f);
            }
        }

        // Corner screws.
        for (juce::Point<float> screw : { juce::Point<float> (26.0f, 28.0f),
                                          juce::Point<float> (1334.0f, 28.0f),
                                          juce::Point<float> (26.0f, 852.0f),
                                          juce::Point<float> (1334.0f, 852.0f),
                                          juce::Point<float> (26.0f, 440.0f),
                                          juce::Point<float> (1334.0f, 440.0f) })
            ui::drawScrew (g, screw, 5.4f);

        // Header rule and fine chrome dividers between the header clusters.
        g.setColour (juce::Colours::black.withAlpha (0.60f));
        g.drawLine (58.0f, (float) headerDividerY, 1302.0f, (float) headerDividerY, 1.0f);

        g.setColour (juce::Colour (0xffb3874f).withAlpha (0.16f));
        g.drawLine (382.0f, 30.0f, 382.0f, 140.0f, 0.8f);
        g.drawLine (872.0f, 30.0f, 872.0f, 140.0f, 0.8f);
        g.drawLine (1070.0f, 30.0f, 1070.0f, 140.0f, 0.8f);
        g.setColour (juce::Colour (0xff8a6a3f).withAlpha (0.26f));
        g.drawLine (58.0f, (float) headerDividerY + 1.0f, 1302.0f, (float) headerDividerY + 1.0f, 0.8f);

        // Header captions.
        ui::drawTrackedText (g, "VOICE ACTIVITY",
                             { 1082, 19, 138, 9 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.8f, 1.3f, true), 0.3f);

        ui::drawTrackedText (g, "OUTPUT MONITOR",
                             { 882, 107, 176, 8 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.4f, 1.2f, true), 0.3f);

        // Power jewel and MIDI lamp.
        ui::drawLed (g, { 1259.0f, 38.0f, 9.0f, 9.0f }, true, ui::colour::amber, 0.95f);
        ui::drawTrackedText (g, "POWER", { 1242, 50, 44, 8 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.4f, 1.0f, true), 0.2f);

        ui::drawLed (g, { 1259.0f, 72.0f, 7.0f, 7.0f }, true, ui::colour::green, 0.65f);
        ui::drawTrackedText (g, "MIDI", { 1248, 82, 32, 8 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.0f, 0.9f, true), 0.2f);

        // Footer engraving.
        ui::drawTrackedText (g, "CS-80E  /  EIGHT VOICE POLYPHONIC SYNTHESIZER",
                             { 60, 850, 400, 14 }, ui::colour::textFaint,
                             juce::Justification::centredLeft, ui::trackedFont (6.4f, 1.6f, true), 0.4f);

        ui::drawTrackedText (g, "DUAL VCO  ·  RING MOD  ·  HPF / LPF  ·  ENSEMBLE",
                             { 480, 850, 400, 14 }, ui::colour::textFaint.brighter (0.16f),
                             juce::Justification::centred, ui::monoFont (6.2f), 0.3f);

        const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
        const auto status = juce::String (sampleRate / 1000.0, 1) + " KHZ  /  8 VOICES  /  V2.0.0";

        ui::drawTrackedText (g, "SERIAL 00108  /  MODEL E  /  " + status,
                             { 900, 850, 400, 14 }, ui::colour::textFaint.brighter (0.16f),
                             juce::Justification::centredRight, ui::monoFont (6.0f), 0.2f);
    }

    void resized() override
    {
        brand.setBounds (60, 24, 320, 112);
        presetDisplay.setBounds (398, 30, 466, 96);
        scope.setBounds (882, 32, 176, 72);
        voiceLeds.setBounds (1082, 31, 138, 40);
        ribbon.setBounds (60, 722, 1240, 32);
        keyboard.setBounds (60, 764, 1240, 82);
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
        addSection ("VCO 1", "SAW / SUB / OCTAVE",
                    mainSection (60, 150), ui::colour::amber, ui::SectionPanel::Decoration::saw, 1);
        addSection ("VCO 2", "PULSE / PWM / OCTAVE",
                    mainSection (226, 164), ui::colour::amber, ui::SectionPanel::Decoration::pulse, 2);
        addSection ("RING", "MODULATOR / NOISE",
                    mainSection (406, 130), ui::colour::copper, ui::SectionPanel::Decoration::ring, 3);
        addSection ("FILTER", "HPF / LPF / TRACK / DRIVE",
                    mainSection (552, 292), ui::colour::copper, ui::SectionPanel::Decoration::filter, 4);
        addSection ("FILTER EG", "INITIAL / ATTACK LEVEL / ADSR",
                    mainSection (860, 250), ui::colour::sage, ui::SectionPanel::Decoration::filterEnv, 5);
        addSection ("AMP EG", "ADSR",
                    mainSection (1126, 174), ui::colour::sage, ui::SectionPanel::Decoration::ampEnv, 6);

        addSection ("LFO", "SPEED / WAVE / DELAY / DEPTH",
                    lowerSection (60, 440), ui::colour::slate, ui::SectionPanel::Decoration::lfo, 7);
        addSection ("PERFORMANCE", "TOUCH / BEND / GLIDE / VINTAGE",
                    lowerSection (516, 356), ui::colour::amberHot, ui::SectionPanel::Decoration::touch, 8);
        addSection ("OUTPUT", "MASTER / TUNE / FX / VU",
                    lowerSection (888, 412), ui::colour::cream, ui::SectionPanel::Decoration::output, 9);
    }

    void addSection (const juce::String& title, const juce::String& subtitle,
                     juce::Rectangle<int> bounds, juce::Colour accent,
                     ui::SectionPanel::Decoration decoration, int number)
    {
        auto section = std::make_unique<ui::SectionPanel> (title, subtitle, accent, decoration, number);
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
        fader (pid::vco1Level, { 70, 204, 44, 338 }, "LEVEL");
        fader (pid::vco1Sub, { 116, 204, 44, 338 }, "SUB");
        knob (pid::vco1Octave, { 166, 212, 44, 78 }, 32, "OCT");
        knob (pid::vco1Detune, { 166, 302, 44, 78 }, 32, "DETUNE");

        // VCO 2
        fader (pid::vco2Level, { 236, 204, 44, 338 }, "LEVEL");
        fader (pid::vco2Pw, { 282, 204, 44, 338 }, "PW");
        knob (pid::vco2Octave, { 338, 212, 44, 78 }, 32, "OCT");
        knob (pid::vco2Detune, { 338, 302, 44, 78 }, 32, "DETUNE");

        // Ring modulator / noise
        knob (pid::ring, { 426, 216, 90, 112 }, 70, "RING MOD");
        knob (pid::noise, { 434, 352, 74, 102 }, 54);

        // Filters
        fader (pid::hpfCutoff, { 566, 204, 54, 338 }, "HP CUTOFF");
        fader (pid::lpfCutoff, { 626, 204, 54, 338 }, "LP CUTOFF");
        knob (pid::hpfRes, { 694, 212, 60, 92 }, 44, "HP RES");
        knob (pid::lpfRes, { 758, 212, 60, 92 }, 44, "LP RES");
        knob (pid::drive, { 694, 320, 60, 92 }, 44, "DRIVE");
        knob (pid::filterTrack, { 758, 320, 60, 92 }, 44, "TRACK");

        // Filter envelope
        const char* filterIds[] = { pid::fIl, pid::fAl, pid::fA, pid::fD, pid::fS, pid::fR };
        const char* filterLabels[] = { "IL", "AL", "ATK", "DEC", "SUS", "REL" };

        for (int i = 0; i < 6; ++i)
            fader (filterIds[i], { 872 + i * 37, 204, 36, 338 }, filterLabels[i]);

        // Amp envelope
        const char* ampIds[] = { pid::aA, pid::aD, pid::aS, pid::aR };

        for (int i = 0; i < 4; ++i)
            fader (ampIds[i], { 1136 + i * 40, 204, 36, 338 });

        // LFO
        knob (pid::lfoRate, { 68, 606, 54, 96 }, 42, "SPEED");
        choice (pid::lfoWave, { 126, 610, 72, 92 });
        knob (pid::lfoDelay, { 200, 606, 54, 96 }, 42, "DELAY");
        knob (pid::lfoPitch, { 256, 606, 54, 96 }, 42, "PITCH");
        knob (pid::lfoPwm, { 310, 606, 54, 96 }, 42, "PWM");
        knob (pid::lfoFilter, { 364, 606, 54, 96 }, 42, "FILTER");
        knob (pid::lfoAmp, { 418, 606, 54, 96 }, 42, "TREM");

        // Performance
        knob (pid::aftertouch, { 526, 606, 54, 96 }, 42);
        knob (pid::bendRange, { 582, 606, 54, 96 }, 42, "BEND");
        knob (pid::spread, { 638, 606, 54, 96 }, 42);
        knob (pid::velocity, { 694, 606, 54, 96 }, 42);
        knob (pid::glide, { 750, 606, 54, 96 }, 42);
        knob (pid::vintage, { 806, 606, 54, 96 }, 42, "VINTAGE");

        // Output
        knob (pid::master, { 898, 606, 54, 96 }, 46, "MASTER");
        knob (pid::tune, { 954, 606, 54, 96 }, 42);
        knob (pid::brilliance, { 1010, 606, 54, 96 }, 42, "BRILL");
        knob (pid::ensemble, { 1066, 606, 54, 96 }, 42);
        knob (pid::space, { 1122, 606, 54, 96 }, 42);

        auto meter = std::make_unique<ui::MeterDisplay> (processor.getEngine());
        meter->setBounds (1210, 600, 48, 106);
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
    setResizeLimits (1020, 660, 2720, 1760);

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
