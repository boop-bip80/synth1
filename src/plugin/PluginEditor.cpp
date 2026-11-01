#include "PluginEditor.h"

#include "../dsp/Params.h"

#include <cmath>

namespace cs80
{
namespace
{
constexpr int headerDividerY = 158;
constexpr int mainRowY = 168;
constexpr int mainRowH = 404;
constexpr int lowerRowY = 582;
constexpr int lowerRowH = 196;

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
          keyboard (processorIn)
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
        const juce::Rectangle<float> face (40.0f, 10.0f, 1360.0f, 940.0f);

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
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0a365),
                                                     face.getX(), face.getY(),
                                                     juce::Colour (0xff5a3c1b),
                                                     face.getRight(), face.getY(), false));
            g.fillRect (juce::Rectangle<float> (face.getX() + 6.0f, face.getY() + 0.5f,
                                                face.getWidth() - 12.0f, 2.0f));

            // Warm bloom behind the display, scope and badge.
            ui::drawGlow (g, { 678.0f, 72.0f }, 310.0f, ui::colour::amber, 0.11f);
            ui::drawGlow (g, { 1070.0f, 68.0f }, 165.0f, ui::colour::amber, 0.08f);
            ui::drawGlow (g, { 246.0f, 74.0f }, 210.0f, ui::colour::amber, 0.055f);

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
                                                     face.getCentreX(), face.getBottom() - 110.0f,
                                                     juce::Colours::black.withAlpha (0.30f),
                                                     face.getCentreX(), face.getBottom(), false));
            g.fillRect (face.withTop (face.getBottom() - 110.0f));
        }

        // Bevels.
        g.setColour (juce::Colours::white.withAlpha (0.11f));
        g.drawRoundedRectangle (face.reduced (0.7f), 7.5f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawRoundedRectangle (face.reduced (1.8f), 7.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawRoundedRectangle (face.reduced (9.0f), 4.0f, 0.9f);
        g.setColour (juce::Colour (0xffd0a365).withAlpha (0.24f));
        g.drawRoundedRectangle (face.reduced (12.0f), 3.4f, 0.9f);

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
                                          juce::Point<float> (1394.0f, 28.0f),
                                          juce::Point<float> (26.0f, 932.0f),
                                          juce::Point<float> (1394.0f, 932.0f),
                                          juce::Point<float> (26.0f, 480.0f),
                                          juce::Point<float> (1394.0f, 480.0f) })
            ui::drawScrew (g, screw, 5.4f);

        // Header rule and fine chrome dividers between the header clusters.
        g.setColour (juce::Colours::black.withAlpha (0.60f));
        g.drawLine (58.0f, (float) headerDividerY, 1382.0f, (float) headerDividerY, 1.0f);

        g.setColour (juce::Colour (0xffb3874f).withAlpha (0.16f));
        g.drawLine (382.0f, 30.0f, 382.0f, 140.0f, 0.8f);
        g.drawLine (968.0f, 30.0f, 968.0f, 140.0f, 0.8f);
        g.drawLine (1180.0f, 30.0f, 1180.0f, 140.0f, 0.8f);
        g.setColour (juce::Colour (0xff8a6a3f).withAlpha (0.26f));
        g.drawLine (58.0f, (float) headerDividerY + 1.0f, 1382.0f, (float) headerDividerY + 1.0f, 0.8f);

        // Header captions.
        ui::drawTrackedText (g, "DUAL SCOPE  /  L + R",
                             { 976, 16, 196, 9 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.8f, 1.3f, true), 0.3f);

        ui::drawTrackedText (g, "VOICE CARDS  ·  I / II",
                             { 1186, 16, 180, 9 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.8f, 1.3f, true), 0.3f);

        // Channel plate names over the two control rows.
        ui::drawTrackedText (g, "CHANNEL I  /  VCO  ·  RING  ·  FILTER  ·  ENVELOPES",
                             { 62, 159, 640, 8 }, ui::colour::amberDim.brighter (0.55f),
                             juce::Justification::centredLeft, ui::monoFont (5.3f), 0.25f);

        ui::drawTrackedText (g, "CHANNEL II  /  LAYER  ·  SPLIT  ·  MODULATION  ·  OUTPUT",
                             { 62, 573, 760, 8 }, ui::colour::amberDim.brighter (0.55f),
                             juce::Justification::centredLeft, ui::monoFont (5.3f), 0.25f);

        // Power jewel and MIDI lamp.
        ui::drawLed (g, { 1364.0f, 38.0f, 9.0f, 9.0f }, true, ui::colour::amber, 0.95f);
        ui::drawTrackedText (g, "POWER", { 1340, 50, 44, 8 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.2f, 1.0f, true), 0.2f);

        ui::drawLed (g, { 1365.0f, 72.0f, 7.0f, 7.0f }, true, ui::colour::green, 0.65f);
        ui::drawTrackedText (g, "MIDI", { 1348, 82, 32, 8 }, ui::colour::textFaint,
                             juce::Justification::centred, ui::trackedFont (5.0f, 0.9f, true), 0.2f);

        // Footer engraving.
        ui::drawTrackedText (g, "CS-80E  /  DUAL CHANNEL  /  EIGHT VOICE",
                             { 62, 926, 420, 12 }, ui::colour::textFaint,
                             juce::Justification::centredLeft, ui::trackedFont (6.5f, 1.6f, true), 0.4f);

        ui::drawTrackedText (g, "CH I + CH II  ·  61 KEY  ·  POLY TOUCH  ·  BRONZE RIBBON",
                             { 500, 926, 500, 12 }, ui::colour::textFaint.brighter (0.16f),
                             juce::Justification::centred, ui::monoFont (6.2f), 0.3f);

        const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
        const auto status = juce::String (sampleRate / 1000.0, 1) + " KHZ  /  8 VOICES  /  V4.0.0";

        ui::drawTrackedText (g, "SERIAL 00108  /  MODEL E  /  " + status,
                             { 900, 926, 480, 12 }, ui::colour::textFaint.brighter (0.16f),
                             juce::Justification::centredRight, ui::monoFont (6.0f), 0.2f);
    }

    void resized() override
    {
        brand.setBounds (60, 24, 320, 112);
        presetDisplay.setBounds (398, 30, 560, 96);
        scope.setBounds (976, 32, 196, 72);
        voiceLeds.setBounds (1186, 31, 180, 40);
        ribbon.setBounds (60, 788, 1320, 32);
        keyboard.setBounds (60, 828, 1320, 88);
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
        addSection ("VCO 1", "CHANNEL I  /  SAW / SUB / OCTAVE",
                    mainSection (60, 190), ui::colour::amber, ui::SectionPanel::Decoration::saw, 1);
        addSection ("VCO 2", "CHANNEL I  /  PULSE / PWM / OCTAVE",
                    mainSection (258, 190), ui::colour::amberHot, ui::SectionPanel::Decoration::pulse, 2);
        addSection ("RING", "CARRIER / BELL / NOISE",
                    mainSection (456, 150), ui::colour::copper, ui::SectionPanel::Decoration::ring, 3);
        addSection ("FILTER", "CHANNEL I  /  HPF / LPF / DRIVE",
                    mainSection (614, 320), ui::colour::copper, ui::SectionPanel::Decoration::filter, 4);
        addSection ("FILTER EG", "INITIAL / ATTACK LEVEL / ADSR",
                    mainSection (942, 250), ui::colour::sage, ui::SectionPanel::Decoration::filterEnv, 5);
        addSection ("VCA", "SINE MIX / AMPLITUDE ENVELOPE",
                    mainSection (1200, 180), ui::colour::rose, ui::SectionPanel::Decoration::ampEnv, 6);

        addSection ("CHANNEL II", "LAYER / SPLIT / ANALOG OFFSET",
                    lowerSection (60, 404), ui::colour::rose, ui::SectionPanel::Decoration::layer, 7);
        addSection ("LFO", "SPEED / WAVE / DELAY / DEPTH",
                    lowerSection (472, 348), ui::colour::slate, ui::SectionPanel::Decoration::lfo, 8);
        addSection ("PERFORMANCE", "TOUCH / BEND / TWANG / VINTAGE",
                    lowerSection (828, 296), ui::colour::sage, ui::SectionPanel::Decoration::touch, 9);
        addSection ("OUTPUT", "MASTER / TUNE / FX / VU",
                    lowerSection (1132, 248), ui::colour::gold, ui::SectionPanel::Decoration::output, 10);
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

        auto fader = [this] (const char* id, B bounds, const char* labelOverride = nullptr,
                             juce::Colour accent = ui::colour::amber)
        {
            auto control = std::make_unique<ui::ParamFader> (processor.apvts, id);

            if (labelOverride != nullptr)
                control->setLabelOverride (labelOverride);

            control->setAccentColour (accent);
            control->setBounds (bounds);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        };

        auto knob = [this] (const char* id, B bounds, int diameter, const char* labelOverride = nullptr,
                            juce::Colour accent = ui::colour::amber)
        {
            auto control = std::make_unique<ui::ParamKnob> (processor.apvts, id, diameter);

            if (labelOverride != nullptr)
                control->setLabelOverride (labelOverride);

            control->setAccentColour (accent);
            control->setBounds (bounds);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        };

        auto choice = [this] (const char* id, B bounds, juce::Colour accent = ui::colour::amber)
        {
            auto control = std::make_unique<ui::ParamChoice> (processor.apvts, id);
            control->setAccentColour (accent);
            control->setBounds (bounds);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        };

        // VCO 1
        fader (pid::vco1Level, { 72, 210, 46, 336 }, "LEVEL", ui::colour::amber);
        fader (pid::vco1Sub, { 126, 210, 46, 336 }, "SUB", ui::colour::amber);
        knob (pid::vco1Octave, { 180, 216, 58, 82 }, 42, "OCT", ui::colour::amber);
        knob (pid::vco1Detune, { 180, 306, 58, 82 }, 42, "DETUNE", ui::colour::amber);

        // VCO 2
        fader (pid::vco2Level, { 270, 210, 46, 336 }, "LEVEL", ui::colour::amberHot);
        fader (pid::vco2Pw, { 324, 210, 46, 336 }, "PW", ui::colour::amberHot);
        knob (pid::vco2Octave, { 378, 216, 58, 82 }, 42, "OCT", ui::colour::amberHot);
        knob (pid::vco2Detune, { 378, 306, 58, 82 }, 42, "DETUNE", ui::colour::amberHot);

        // Ring modulator / noise
        knob (pid::ring, { 468, 214, 62, 100 }, 58, "RING MOD", ui::colour::copper);
        knob (pid::ringSpeed, { 534, 214, 62, 100 }, 58, "SPEED", ui::colour::copper);
        knob (pid::noise, { 501, 326, 62, 100 }, 58, "NOISE", ui::colour::copper);

        // Filters
        fader (pid::hpfCutoff, { 626, 210, 54, 336 }, "HP CUTOFF", ui::colour::copper);
        fader (pid::lpfCutoff, { 688, 210, 54, 336 }, "LP CUTOFF", ui::colour::copper);
        knob (pid::hpfRes, { 752, 214, 64, 88 }, 44, "HP RES", ui::colour::copper);
        knob (pid::lpfRes, { 820, 214, 64, 88 }, 44, "LP RES", ui::colour::copper);
        knob (pid::drive, { 752, 320, 64, 88 }, 44, "DRIVE", ui::colour::copper);
        knob (pid::filterTrack, { 820, 320, 64, 88 }, 44, "TRACK", ui::colour::copper);

        // Filter envelope
        const char* filterIds[] = { pid::fIl, pid::fAl, pid::fA, pid::fD, pid::fS, pid::fR };
        const char* filterLabels[] = { "IL", "AL", "ATK", "DEC", "SUS", "REL" };

        for (int i = 0; i < 6; ++i)
            fader (filterIds[i], { 954 + i * 39, 210, 34, 336 }, filterLabels[i], ui::colour::sage);

        // Sine mix and VCA envelope
        fader (pid::sineLevel, { 1210, 210, 32, 336 }, "SINE", ui::colour::rose);

        const char* ampIds[] = { pid::aA, pid::aD, pid::aS, pid::aR };
        const char* ampLabels[] = { "ATK", "DEC", "SUS", "REL" };

        for (int i = 0; i < 4; ++i)
            fader (ampIds[i], { 1242 + i * 32, 210, 32, 336 }, ampLabels[i], ui::colour::rose);

        // Channel II
        const int chX[] = { 70, 148, 226, 304, 382 };

        knob (pid::layerLevel, { chX[0], 616, 76, 74 }, 36, nullptr, ui::colour::rose);
        knob (pid::layerDetune, { chX[1], 616, 76, 74 }, 36, nullptr, ui::colour::rose);
        knob (pid::layerOctave, { chX[2], 616, 76, 74 }, 36, "OCT", ui::colour::rose);
        knob (pid::layerPan, { chX[3], 616, 76, 74 }, 36, nullptr, ui::colour::rose);
        choice (pid::layerMode, { chX[4], 616, 76, 74 }, ui::colour::rose);
        knob (pid::layerCutoff, { chX[0], 692, 76, 74 }, 36, "CUTOFF", ui::colour::rose);
        knob (pid::layerRes, { chX[1], 692, 76, 74 }, 36, "RES", ui::colour::rose);
        knob (pid::layerPw, { chX[2], 692, 76, 74 }, 36, "PW", ui::colour::rose);
        knob (pid::layerEnv, { chX[3], 692, 76, 74 }, 36, "ENV", ui::colour::rose);
        knob (pid::splitPoint, { chX[4], 692, 76, 74 }, 36, "SPLIT", ui::colour::rose);

        // LFO
        const int lfoX[] = { 480, 562, 644, 726 };
        knob (pid::lfoRate, { lfoX[0], 616, 78, 74 }, 36, "SPEED", ui::colour::slate);
        choice (pid::lfoWave, { lfoX[1], 616, 78, 74 }, ui::colour::slate);
        knob (pid::lfoDelay, { lfoX[2], 616, 78, 74 }, 36, "DELAY", ui::colour::slate);
        knob (pid::lfoPitch, { lfoX[3], 616, 78, 74 }, 36, "PITCH", ui::colour::slate);
        knob (pid::lfoPwm, { lfoX[0], 692, 78, 74 }, 36, "PWM", ui::colour::slate);
        knob (pid::lfoFilter, { lfoX[1], 692, 78, 74 }, 36, "FILTER", ui::colour::slate);
        knob (pid::lfoAmp, { lfoX[2], 692, 78, 74 }, 36, "TREM", ui::colour::slate);

        // Performance
        const int perfX[] = { 834, 902, 970, 1038 };
        knob (pid::aftertouch, { perfX[0], 616, 68, 74 }, 36, nullptr, ui::colour::sage);
        knob (pid::bendRange, { perfX[1], 616, 68, 74 }, 36, "BEND", ui::colour::sage);
        knob (pid::spread, { perfX[2], 616, 68, 74 }, 36, nullptr, ui::colour::sage);
        knob (pid::twang, { perfX[3], 616, 68, 74 }, 36, "TWANG", ui::colour::sage);
        knob (pid::velocity, { perfX[0], 692, 68, 74 }, 36, nullptr, ui::colour::sage);
        knob (pid::glide, { perfX[1], 692, 68, 74 }, 36, nullptr, ui::colour::sage);
        knob (pid::vintage, { perfX[2], 692, 68, 74 }, 36, "VINTAGE", ui::colour::sage);

        // Output
        knob (pid::master, { 1142, 616, 54, 74 }, 42, "MASTER", ui::colour::gold);
        knob (pid::tune, { 1196, 616, 54, 74 }, 42, nullptr, ui::colour::gold);
        knob (pid::brilliance, { 1250, 616, 54, 74 }, 42, "BRILL", ui::colour::gold);
        knob (pid::ensemble, { 1142, 692, 54, 74 }, 42, "ENSEMBLE", ui::colour::gold);
        knob (pid::space, { 1196, 692, 54, 74 }, 42, "SPACE", ui::colour::gold);

        auto meter = std::make_unique<ui::MeterDisplay> (processor.getEngine());
        meter->setBounds (1264, 618, 48, 146);
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
    setResizeLimits (1080, 720, 2880, 1920);

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
