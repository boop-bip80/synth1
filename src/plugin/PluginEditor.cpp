#include "PluginEditor.h"

#include "../dsp/Params.h"

#include <cmath>

namespace cs80
{
namespace
{
constexpr int headerDividerY = 156;
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

        // Shadow and the lacquered metal face.
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.fillRoundedRectangle (face.expanded (6.0f).translated (0.0f, 5.0f), 11.0f);

        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path faceClip;
            faceClip.addRoundedRectangle (face, 7.0f);
            g.reduceClipRegion (faceClip);

            ui::drawBrushedPanel (g, face.toNearestInt());

            // Warm stage light across the top, cool bounce from below.
            ui::drawGlow (g, { 250.0f, 58.0f }, 330.0f, ui::colour::amber, 0.30f);
            ui::drawGlow (g, { 676.0f, 66.0f }, 380.0f, ui::colour::amber, 0.24f);
            ui::drawGlow (g, { 1096.0f, 62.0f }, 240.0f, ui::colour::aurora, 0.20f);
            ui::drawGlow (g, { 1300.0f, 300.0f }, 260.0f, ui::colour::aurora, 0.10f);
            ui::drawGlow (g, { 120.0f, 700.0f }, 280.0f, ui::colour::aurora, 0.09f);

            // A long, low specular sweep across the brushed face.
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.055f),
                                                     face.getX(), face.getY() + 210.0f,
                                                     juce::Colours::transparentWhite,
                                                     face.getRight(), face.getY() + 470.0f, false));
            g.fillRect (face);

            // Side vignettes.
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.46f),
                                                     face.getX(), 0.0f,
                                                     juce::Colours::transparentBlack,
                                                     face.getX() + 48.0f, 0.0f, false));
            g.fillRect (face.withWidth (48.0f));

            g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                     face.getRight() - 48.0f, 0.0f,
                                                     juce::Colours::black.withAlpha (0.46f),
                                                     face.getRight(), 0.0f, false));
            g.fillRect (face.withTrimmedLeft (face.getWidth() - 48.0f));

            // Bottom bounce.
            g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                     face.getCentreX(), face.getBottom() - 120.0f,
                                                     juce::Colours::black.withAlpha (0.34f),
                                                     face.getCentreX(), face.getBottom(), false));
            g.fillRect (face.withTop (face.getBottom() - 120.0f));

            // Brass rails on the faceplate edges.
            ui::drawBrassRail (g, { face.getX() + 6.0f, face.getY() + 1.0f,
                                    face.getWidth() - 12.0f, 2.4f }, false, 0.96f);
            ui::drawBrassRail (g, { face.getX() + 1.0f, face.getY() + 8.0f,
                                    2.4f, face.getHeight() - 16.0f }, true, 0.78f);
            ui::drawBrassRail (g, { face.getRight() - 3.4f, face.getY() + 8.0f,
                                    2.4f, face.getHeight() - 16.0f }, true, 0.78f);
        }

        // Bevels and inner hairlines.
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.drawRoundedRectangle (face.reduced (0.7f), 7.5f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.88f));
        g.drawRoundedRectangle (face.reduced (1.9f), 7.0f, 1.1f);
        g.setColour (juce::Colours::white.withAlpha (0.048f));
        g.drawRoundedRectangle (face.reduced (9.0f), 4.0f, 0.9f);
        g.setColour (juce::Colour (0xffd0a365).withAlpha (0.27f));
        g.drawRoundedRectangle (face.reduced (12.0f), 3.4f, 0.9f);

        // Brass corner brackets.
        {
            const float inset = 20.0f;
            const float len = 28.0f;
            g.setColour (juce::Colour (0xffc39a62).withAlpha (0.36f));

            for (int corner = 0; corner < 4; ++corner)
            {
                const bool right = (corner & 1) != 0;
                const bool bottom = corner >= 2;
                const float x = right ? face.getRight() - inset : face.getX() + inset;
                const float y = bottom ? face.getBottom() - inset : face.getY() + inset;
                const float dx = right ? -len : len;
                const float dy = bottom ? -len : len;

                g.drawLine (x, y, x + dx, y, 1.1f);
                g.drawLine (x, y, x, y + dy, 1.1f);
            }
        }

        // Corner screws and mid-rail fasteners.
        for (juce::Point<float> screw : { juce::Point<float> (26.0f, 28.0f),
                                          juce::Point<float> (1394.0f, 28.0f),
                                          juce::Point<float> (26.0f, 932.0f),
                                          juce::Point<float> (1394.0f, 932.0f),
                                          juce::Point<float> (26.0f, 480.0f),
                                          juce::Point<float> (1394.0f, 480.0f) })
            ui::drawScrew (g, screw, 5.4f);

        // Header rules.
        g.setColour (juce::Colours::black.withAlpha (0.66f));
        g.drawLine (58.0f, (float) headerDividerY, 1382.0f, (float) headerDividerY, 1.1f);
        g.setColour (juce::Colour (0xffb3874f).withAlpha (0.20f));
        g.drawLine (58.0f, (float) headerDividerY + 1.2f, 1382.0f, (float) headerDividerY + 1.2f, 0.8f);

        g.setColour (juce::Colour (0xffb3874f).withAlpha (0.20f));
        g.drawLine (382.0f, 30.0f, 382.0f, 142.0f, 0.8f);
        g.drawLine (968.0f, 30.0f, 968.0f, 142.0f, 0.8f);
        g.drawLine (1180.0f, 30.0f, 1180.0f, 142.0f, 0.8f);

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawLine (58.0f, (float) headerDividerY - 1.0f, 1382.0f, (float) headerDividerY - 1.0f, 0.8f);

        // Header captions.
        ui::drawTrackedText (g, "DUAL TRACE  /  L + R",
                             { 980, 14, 232, 10 }, ui::colour::textFaint.brighter (0.20f),
                             juce::Justification::centred, ui::trackedFont (6.5f, 1.4f, true), 0.4f);

        ui::drawTrackedText (g, "VOICE CARDS  ·  I / II",
                             { 1229, 14, 148, 10 }, ui::colour::textFaint.brighter (0.20f),
                             juce::Justification::centred, ui::trackedFont (6.2f, 1.4f, true), 0.4f);

        // Channel plate names over the two control rows.
        ui::drawTrackedText (g, "CHANNEL I  /  VCO  ·  RING  ·  FILTER  ·  ENVELOPES",
                             { 62, 157, 700, 9 }, ui::colour::amberDim.brighter (0.92f),
                             juce::Justification::centredLeft, ui::monoFont (6.4f), 0.25f);

        ui::drawTrackedText (g, "CHANNEL II  /  LAYER  ·  SPLIT  ·  MODULATION  ·  OUTPUT",
                             { 62, 571, 760, 9 }, ui::colour::amberDim.brighter (0.92f),
                             juce::Justification::centredLeft, ui::monoFont (6.4f), 0.25f);

        // Footer engraving.
        g.setColour (juce::Colour (0xffd9b276).withAlpha (0.34f));
        g.drawLine (62.0f, 921.0f, 1380.0f, 921.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (62.0f, 922.2f, 1380.0f, 922.2f, 0.8f);

        ui::drawTrackedText (g, "CS-80E  /  DUAL CHANNEL  /  EIGHT VOICE",
                             { 62, 926, 440, 13 }, ui::colour::textFaint.brighter (0.10f),
                             juce::Justification::centredLeft, ui::trackedFont (7.2f, 1.7f, true), 0.4f);

        ui::drawTrackedText (g, "CH I + CH II  ·  61 KEY  ·  POLY TOUCH  ·  BRONZE RIBBON",
                             { 500, 926, 500, 13 }, ui::colour::textFaint.brighter (0.30f),
                             juce::Justification::centred, ui::monoFont (6.8f), 0.3f);

        const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
        const auto status = juce::String (sampleRate / 1000.0, 1) + " KHZ  /  8 VOICES  /  V5.2.0";

        ui::drawTrackedText (g, "SERIAL 00108  /  MODEL E  /  " + status,
                             { 900, 926, 480, 13 }, ui::colour::textFaint.brighter (0.30f),
                             juce::Justification::centredRight, ui::monoFont (6.5f), 0.2f);

        ui::drawTrackedText (g, "SERIES 5  ·  ANALOG SOUL / MODERN BUILD",
                             { 500, 938, 500, 8 }, ui::colour::amber.withAlpha (0.50f),
                             juce::Justification::centred, ui::trackedFont (5.4f, 1.5f, true), 0.2f);
    }

    void resized() override
    {
        brand.setBounds (60, 20, 322, 124);
        presetDisplay.setBounds (398, 24, 566, 112);
        scope.setBounds (976, 26, 240, 90);
        voiceLeds.setBounds (1228, 26, 150, 46);
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
                    mainSection (258, 190), ui::colour::tangerine, ui::SectionPanel::Decoration::pulse, 2);
        addSection ("RING", "CARRIER / BELL / NOISE",
                    mainSection (456, 150), ui::colour::copper, ui::SectionPanel::Decoration::ring, 3);
        addSection ("FILTER", "CHANNEL I  /  HPF / LPF / DRIVE",
                    mainSection (614, 320), ui::colour::emerald, ui::SectionPanel::Decoration::filter, 4);
        addSection ("FILTER EG", "INITIAL · ATTACK · ADSR · DEPTH",
                    mainSection (942, 250), ui::colour::sage, ui::SectionPanel::Decoration::filterEnv, 5);
        addSection ("VCA", "SINE MIX / AMP ENVELOPE",
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
        knob (pid::vco1Octave, { 180, 216, 58, 82 }, 46, "OCT", ui::colour::amber);
        knob (pid::vco1Detune, { 180, 306, 58, 82 }, 46, "DETUNE", ui::colour::amber);

        // VCO 2
        fader (pid::vco2Level, { 270, 210, 46, 336 }, "LEVEL", ui::colour::tangerine);
        fader (pid::vco2Pw, { 324, 210, 46, 336 }, "PW", ui::colour::tangerine);
        knob (pid::vco2Octave, { 378, 216, 58, 82 }, 46, "OCT", ui::colour::tangerine);
        knob (pid::vco2Detune, { 378, 306, 58, 82 }, 46, "DETUNE", ui::colour::tangerine);

        // Ring modulator / noise
        knob (pid::ring, { 468, 212, 62, 104 }, 60, "RING MOD", ui::colour::copper);
        knob (pid::ringSpeed, { 534, 212, 62, 104 }, 60, "SPEED", ui::colour::copper);
        knob (pid::noise, { 501, 322, 62, 104 }, 60, "NOISE", ui::colour::copper);

        // Filters
        fader (pid::hpfCutoff, { 626, 210, 54, 336 }, "HP CUTOFF", ui::colour::emerald);
        fader (pid::lpfCutoff, { 688, 210, 54, 336 }, "LP CUTOFF", ui::colour::emerald);
        knob (pid::hpfRes, { 752, 210, 64, 94 }, 48, "HP RES", ui::colour::emerald);
        knob (pid::lpfRes, { 820, 210, 64, 94 }, 48, "LP RES", ui::colour::emerald);
        knob (pid::drive, { 752, 314, 64, 94 }, 48, "DRIVE", ui::colour::emerald);
        knob (pid::filterTrack, { 820, 314, 64, 94 }, 48, "TRACK", ui::colour::emerald);

        // Filter envelope, including the new sweep depth.
        const char* filterIds[] = { pid::fIl, pid::fAl, pid::fA, pid::fD, pid::fS,
                                    pid::fR, pid::filterEnvAmt };
        const char* filterLabels[] = { "IL", "AL", "ATK", "DEC", "SUS", "REL", "DEPTH" };

        for (int i = 0; i < 7; ++i)
            fader (filterIds[i], { 948 + i * 33, 210, 30, 336 }, filterLabels[i], ui::colour::sage);

        // Sine mix and VCA envelope
        fader (pid::sineLevel, { 1210, 210, 32, 336 }, "SINE", ui::colour::rose);

        const char* ampIds[] = { pid::aA, pid::aD, pid::aS, pid::aR };
        const char* ampLabels[] = { "ATK", "DEC", "SUS", "REL" };

        for (int i = 0; i < 4; ++i)
            fader (ampIds[i], { 1242 + i * 32, 210, 32, 336 }, ampLabels[i], ui::colour::rose);

        // Channel II
        const int chX[] = { 70, 148, 226, 304, 382 };

        knob (pid::layerLevel, { chX[0], 622, 76, 70 }, 38, nullptr, ui::colour::rose);
        knob (pid::layerDetune, { chX[1], 622, 76, 70 }, 38, nullptr, ui::colour::rose);
        knob (pid::layerOctave, { chX[2], 622, 76, 70 }, 38, "OCT", ui::colour::rose);
        knob (pid::layerPan, { chX[3], 622, 76, 70 }, 38, nullptr, ui::colour::rose);
        choice (pid::layerMode, { chX[4], 622, 76, 70 }, ui::colour::rose);
        knob (pid::layerCutoff, { chX[0], 698, 76, 70 }, 36, "CUTOFF", ui::colour::rose);
        knob (pid::layerRes, { chX[1], 698, 76, 70 }, 36, "RES", ui::colour::rose);
        knob (pid::layerPw, { chX[2], 698, 76, 70 }, 36, "PW", ui::colour::rose);
        knob (pid::layerEnv, { chX[3], 698, 76, 70 }, 36, "ENV", ui::colour::rose);
        knob (pid::splitPoint, { chX[4], 698, 76, 70 }, 36, "SPLIT", ui::colour::rose);

        // LFO
        const int lfoX[] = { 480, 562, 644, 726 };
        knob (pid::lfoRate, { lfoX[0], 622, 78, 70 }, 36, "SPEED", ui::colour::slate);
        choice (pid::lfoWave, { lfoX[1], 622, 78, 70 }, ui::colour::slate);
        knob (pid::lfoDelay, { lfoX[2], 622, 78, 70 }, 36, "DELAY", ui::colour::slate);
        knob (pid::lfoPitch, { lfoX[3], 622, 78, 70 }, 36, "PITCH", ui::colour::slate);
        knob (pid::lfoPwm, { lfoX[0], 698, 78, 70 }, 36, "PWM", ui::colour::slate);
        knob (pid::lfoFilter, { lfoX[1], 698, 78, 70 }, 36, "FILTER", ui::colour::slate);
        knob (pid::lfoAmp, { lfoX[2], 698, 78, 70 }, 36, "TREM", ui::colour::slate);

        // Performance
        const int perfX[] = { 834, 902, 970, 1038 };
        knob (pid::aftertouch, { perfX[0], 622, 68, 70 }, 36, nullptr, ui::colour::sage);
        knob (pid::bendRange, { perfX[1], 622, 68, 70 }, 36, "BEND", ui::colour::sage);
        knob (pid::spread, { perfX[2], 622, 68, 70 }, 36, nullptr, ui::colour::sage);
        knob (pid::twang, { perfX[3], 622, 68, 70 }, 36, "TWANG", ui::colour::sage);
        knob (pid::velocity, { perfX[0], 698, 68, 70 }, 36, nullptr, ui::colour::sage);
        knob (pid::glide, { perfX[1], 698, 68, 70 }, 36, nullptr, ui::colour::sage);
        knob (pid::vintage, { perfX[2], 698, 68, 70 }, 36, "VINTAGE", ui::colour::sage);

        // Output
        knob (pid::master, { 1139, 622, 64, 70 }, 38, "MASTER", ui::colour::gold);
        knob (pid::tune, { 1205, 622, 64, 70 }, 38, nullptr, ui::colour::gold);
        knob (pid::brilliance, { 1271, 622, 64, 70 }, 38, "BRILL", ui::colour::gold);
        knob (pid::ensemble, { 1139, 698, 64, 70 }, 38, "ENSEMBLE", ui::colour::gold);
        choice (pid::ensembleMode, { 1205, 698, 64, 70 }, ui::colour::gold);
        knob (pid::space, { 1271, 698, 64, 70 }, 38, "SPACE", ui::colour::gold);

        auto meter = std::make_unique<ui::MeterDisplay> (processor.getEngine());
        meter->setBounds (1339, 624, 33, 144);
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

    tooltip = std::make_unique<juce::TooltipWindow> (this, 650);
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
    if (key == juce::KeyPress::escapeKey)
    {
        if (content != nullptr && content->getBrowser() != nullptr)
        {
            closePresetBrowser();
            return true;
        }

        return false;
    }

    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
    {
        const int count = (int) factoryPresets().size();

        if (count > 0)
        {
            const int step = key == juce::KeyPress::leftKey ? -1 : 1;
            processor.loadPreset ((processor.currentPresetIndex() + step + count) % count);

            if (content != nullptr)
                content->repaint();
        }

        return true;
    }

    const auto character = key.getTextCharacter();

    if (character == 'l' || character == 'L')
    {
        openPresetBrowser();
        return true;
    }

    return false;
}
} // namespace cs80
