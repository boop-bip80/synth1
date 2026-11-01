#include "Widgets.h"

#include "../../dsp/Dsp.h"
#include "../../dsp/Params.h"

#include <cmath>

namespace cs80::ui
{
namespace
{
juce::String noteNameFromMidi (float value)
{
    static const char* const names[] = { "C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B" };
    const int note = (int) std::round (value);
    const int pitchClass = ((note % 12) + 12) % 12;
    const int octave = note / 12 - 1;
    return juce::String (names[pitchClass]) + juce::String (octave);
}

juce::String formatParamValue (const juce::String& id, float value)
{
    if (id == pid::vco1Octave || id == pid::vco2Octave || id == pid::layerOctave)
    {
        const int octave = (int) std::round (value);
        return (octave > 0 ? "+" : "") + juce::String (octave) + " OCT";
    }

    if (id == pid::splitPoint)
        return noteNameFromMidi (value);

    if (id == pid::layerPan)
    {
        const int amount = (int) std::round (std::abs (value) * 100.0f);

        if (amount == 0)
            return "CENTER";

        return (value < 0.0f ? "L " : "R ") + juce::String (amount);
    }

    if (id == pid::layerCutoff)
        return (value >= 0.0f ? "+" : "") + juce::String (value, 2) + " OCT";

    if (id == pid::layerEnv)
        return juce::String (value, 2) + "X";

    if (id == pid::ringSpeed)
        return juce::String (value, 0) + " HZ";

    if (id == pid::bendRange || id == pid::twang)
        return juce::String ((int) std::round (value)) + " ST";

    if (id == pid::fA || id == pid::fD || id == pid::fR
        || id == pid::aA || id == pid::aD || id == pid::aR
        || id == pid::lfoDelay || id == pid::glide)
        return formatTime (value);

    if (id == pid::hpfCutoff || id == pid::lpfCutoff)
        return formatHz (value);

    if (id == pid::lfoRate)
        return juce::String (value, 2) + " HZ";

    if (id == pid::master)
        return juce::String (value, 1) + " DB";

    if (id == pid::tune)
        return (value > 0.0f ? "+" : "") + juce::String ((int) std::round (value)) + " CT";

    if (id == pid::vco1Detune || id == pid::vco2Detune || id == pid::layerDetune)
        return (value > 0.0f ? "+" : "") + juce::String ((int) std::round (value)) + " CT";

    if (id == pid::fIl || id == pid::fAl || id == pid::fS
        || id == pid::aS || id == pid::vco1Level || id == pid::vco1Sub
        || id == pid::vco2Level || id == pid::vco2Pw || id == pid::ring
        || id == pid::noise || id == pid::hpfRes || id == pid::lpfRes
        || id == pid::ensemble || id == pid::space || id == pid::lfoPwm
        || id == pid::lfoFilter || id == pid::lfoAmp || id == pid::lfoPitch
        || id == pid::aftertouch || id == pid::spread || id == pid::velocity
        || id == pid::vintage || id == pid::drive || id == pid::filterTrack
        || id == pid::filterEnvAmt
        || id == pid::layerLevel || id == pid::layerRes || id == pid::layerPw
        || id == pid::sineLevel)
        return juce::String (juce::roundToInt (value * 100.0f)) + "%";

    return juce::String (value, 2);
}

void drawWaveformPath (juce::Graphics& g, juce::Rectangle<float> area,
                       int cycles, bool pulse, juce::Colour colour, float alpha)
{
    juce::Path path;
    const float x0 = area.getX();
    const float x1 = area.getRight();
    const float mid = area.getCentreY();
    const float amp = area.getHeight() * 0.34f;
    const float width = (x1 - x0) / (float) cycles;

    path.startNewSubPath (x0, pulse ? mid + amp : mid);

    for (int c = 0; c < cycles; ++c)
    {
        const float cx = x0 + (float) c * width;

        if (pulse)
        {
            path.lineTo (cx + width * 0.18f, mid + amp);
            path.lineTo (cx + width * 0.18f, mid - amp);
            path.lineTo (cx + width * 0.68f, mid - amp);
            path.lineTo (cx + width * 0.68f, mid + amp);
            path.lineTo (cx + width, mid + amp);
        }
        else
        {
            path.lineTo (cx + width * 0.5f, mid - amp);
            path.lineTo (cx + width, mid);
        }
    }

    g.setColour (colour.withAlpha (alpha * 1.35f));
    g.strokePath (path, juce::PathStrokeType (1.25f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void drawEnvelopePath (juce::Graphics& g, juce::Rectangle<float> area,
                       bool filterShape, juce::Colour colour, float alpha)
{
    const float x = area.getX();
    const float y = area.getY();
    const float w = area.getWidth();
    const float h = area.getHeight();
    const float base = y + h * 0.84f;

    juce::Path path;
    const float start = filterShape ? 0.42f : 0.0f;

    path.startNewSubPath (x, base - h * 0.58f * start);
    path.lineTo (x + w * 0.14f, y + h * 0.12f);
    path.lineTo (x + w * 0.40f, base - h * 0.24f);
    path.lineTo (x + w * 0.76f, base - h * 0.24f);
    path.lineTo (x + w * 0.96f, base);

    g.setColour (colour.withAlpha (alpha * 1.35f));
    g.strokePath (path, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}
} // namespace

//==============================================================================
SectionPanel::SectionPanel (juce::String sectionTitle, juce::String sectionSubtitle,
                            juce::Colour accentColour, Decoration decorationToUse,
                            int sectionNumber)
    : title (std::move (sectionTitle)),
      subtitle (std::move (sectionSubtitle)),
      accent (accentColour),
      decoration (decorationToUse),
      number (sectionNumber)
{
    setInterceptsMouseClicks (false, false);
}

void SectionPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    drawPlate (g, bounds, 5.0f);

    // A warm wash of the section colour behind the controls. The strongest
    // part of the tint sits under the header, where it reads as a lit band.
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds.reduced (1.0f), 4.5f);
        g.reduceClipRegion (clip);

        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.30f),
                                                 bounds.getCentreX(), bounds.getY(),
                                                 accent.withAlpha (0.035f),
                                                 bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.86f, false));
        g.fillRect (bounds);

        drawGlow (g, { bounds.getX() + 24.0f, bounds.getY() + 12.0f },
                  bounds.getWidth() * 0.68f, accent, 0.38f);

        // Fine engraved rules at the top and bottom of the control field.
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (bounds.getX() + 9.0f, bounds.getBottom() - 14.0f,
                    bounds.getRight() - 9.0f, bounds.getBottom() - 14.0f, 0.7f);
        g.setColour (accent.withAlpha (0.16f));
        g.drawLine (bounds.getX() + 9.0f, bounds.getBottom() - 13.2f,
                    bounds.getRight() - 9.0f, bounds.getBottom() - 13.2f, 0.6f);

        const int ticks = juce::jmax (4, (int) (bounds.getWidth() / 26.0f));
        for (int i = 0; i <= ticks; ++i)
        {
            const float tx = bounds.getX() + 12.0f
                           + (bounds.getWidth() - 24.0f) * (float) i / (float) ticks;
            g.setColour (accent.withAlpha (0.22f));
            g.drawLine (tx, bounds.getBottom() - 20.0f, tx, bounds.getBottom() - 16.0f, 0.6f);
        }
    }

    // Saturated header band.
    const juce::Rectangle<float> header (bounds.getX() + 2.0f, bounds.getY() + 2.0f,
                                         bounds.getWidth() - 4.0f, (float) titleHeight - 4.0f);

    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path headerClip;
        headerClip.addRoundedRectangle (header, 3.4f);
        g.reduceClipRegion (headerClip);

        g.setGradientFill (juce::ColourGradient (colour::plateTop,
                                                 header.getCentreX(), header.getY(),
                                                 juce::Colour (0xff0a0806),
                                                 header.getCentreX(), header.getBottom(), false));
        g.fillRect (header);

        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.62f),
                                                 header.getX(), header.getY(),
                                                 accent.withAlpha (0.10f),
                                                 header.getRight(), header.getBottom(), false));
        g.fillRect (header);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f),
                                                 header.getX(), header.getY(),
                                                 juce::Colours::transparentWhite,
                                                 header.getRight(), header.getY() + 14.0f, false));
        g.fillRect (header.withHeight (14.0f));

        g.setColour (accent.withAlpha (0.96f));
        g.fillRect (header.getX(), header.getY(), header.getWidth(), 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.fillRect (header.getX(), header.getBottom() - 2.0f, header.getWidth(), 2.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.74f));
    g.drawRoundedRectangle (header.reduced (0.4f), 3.2f, 0.9f);

    // Jewelled accent capsule at the left edge.
    const juce::Rectangle<float> jewel (header.getX() + 8.0f, header.getCentreY() - 8.5f, 3.6f, 17.0f);
    g.setGradientFill (juce::ColourGradient (accent.brighter (0.55f), jewel.getX(), jewel.getY(),
                                             accent.darker (0.35f), jewel.getRight(), jewel.getBottom(), false));
    g.fillRoundedRectangle (jewel, 1.8f);
    drawGlow (g, jewel.getCentre(), 24.0f, accent, 0.55f);

    drawEngravedText (g, title, { 20, 4, getWidth() - 58, 19 },
                      colour::cream.withAlpha (0.99f),
                      juce::Justification::centredLeft,
                      trackedFont (11.6f, 1.8f, true), 0.5f);

    if (subtitle.isNotEmpty())
        drawEngravedText (g, subtitle, { 20, 22, getWidth() - 52, 10 },
                          colour::textDim.withAlpha (0.80f),
                          juce::Justification::centredLeft,
                          monoFont (6.9f), 0.3f);

    // Numbered card plate, stamped like a slot card.
    if (number > 0)
    {
        const juce::Rectangle<float> plate ((float) getWidth() - 32.0f, 9.0f, 21.0f, 17.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b241c),
                                                 plate.getCentreX(), plate.getY(),
                                                 juce::Colour (0xff050403),
                                                 plate.getCentreX(), plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 2.2f);

        g.setColour (accent.withAlpha (0.58f));
        g.drawRoundedRectangle (plate.reduced (0.5f), 2.2f, 0.7f);

        drawTrackedText (g, juce::String (number).paddedLeft ('0', 2),
                         plate.toNearestInt(), accent.brighter (0.18f).withAlpha (0.92f),
                         juce::Justification::centred,
                         monoFont (7.4f, true), 0.4f);
    }

    // Brass corner marks.
    const float len = 8.0f;
    const float inset = 5.0f;
    g.setColour (colour::brass.withAlpha (0.34f));
    g.drawLine (inset, inset, inset + len, inset, 0.8f);
    g.drawLine (inset, inset, inset, inset + len, 0.8f);
    g.drawLine ((float) getWidth() - inset, (float) getHeight() - inset,
                (float) getWidth() - inset - len, (float) getHeight() - inset, 0.8f);
    g.drawLine ((float) getWidth() - inset, (float) getHeight() - inset,
                (float) getWidth() - inset, (float) getHeight() - inset - len, 0.8f);

    // A lit spine down the left cheek and a pair of slot screws give each
    // module the look of a removable voice card.
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.62f),
                                             (float) bounds.getX() + 2.0f, bounds.getY(),
                                             accent.withAlpha (0.05f),
                                             (float) bounds.getX() + 2.0f, bounds.getBottom(), false));
    g.fillRect (juce::Rectangle<float> ((float) bounds.getX() + 1.2f, (float) bounds.getY() + 7.0f,
                                        1.8f, (float) bounds.getHeight() - 14.0f));

    drawScrew (g, { (float) bounds.getX() + 8.0f, (float) bounds.getY() + 8.0f }, 2.2f);
    drawScrew (g, { (float) bounds.getRight() - 8.0f, (float) bounds.getBottom() - 8.0f }, 2.2f);

    // A calibration rail fills the lower margin of the tall channel cards and
    // makes the empty panel area look deliberately laid out.
    if (getHeight() > 320)
    {
        const juce::Rectangle<float> rail ((float) bounds.getX() + 10.0f,
                                           (float) bounds.getBottom() - 19.0f,
                                           (float) bounds.getWidth() - 20.0f, 11.0f);

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillRoundedRectangle (rail, 2.0f);
        g.setColour (accent.withAlpha (0.42f));
        g.fillRect (rail.getX(), rail.getY(), rail.getWidth(), 0.9f);

        drawTrackedText (g, "VOICE CARD A   ·   CALIBRATED", rail.toNearestInt(),
                         colour::textFaint.withAlpha (0.62f), juce::Justification::centred,
                         monoFont (5.8f), 0.35f);

        g.setColour (accent.withAlpha (0.60f));
        g.fillEllipse (rail.getX() + 4.0f, rail.getCentreY() - 1.2f, 2.4f, 2.4f);
        g.fillEllipse (rail.getRight() - 6.4f, rail.getCentreY() - 1.2f, 2.4f, 2.4f);
    }

    paintDecoration (g, getLocalBounds().toFloat().withTrimmedTop ((float) titleHeight + 2.0f));
}

void SectionPanel::paintDecoration (juce::Graphics& g, juce::Rectangle<float> body)
{
    if (decoration == Decoration::none || body.getHeight() < 30.0f)
        return;

    const auto area = body.reduced (14.0f, 24.0f);
    const float alpha = 0.28f;

    switch (decoration)
    {
        case Decoration::saw:
            drawWaveformPath (g, area.withHeight (area.getHeight() * 0.46f), 3, false, accent, alpha);
            break;

        case Decoration::pulse:
            drawWaveformPath (g, area.withHeight (area.getHeight() * 0.46f), 2, true, accent, alpha);
            break;

        case Decoration::ring:
        {
            const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.22f;
            const auto c = area.getCentre();

            g.setColour (accent.withAlpha (alpha * 0.9f));
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f)
                               .withCentre ({ c.x - r * 0.55f, c.y }), 1.3f);
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f)
                               .withCentre ({ c.x + r * 0.55f, c.y }), 1.3f);
            g.drawLine (c.x - 4.0f, c.y, c.x + 4.0f, c.y, 1.2f);
            g.drawLine (c.x, c.y - 4.0f, c.x, c.y + 4.0f, 1.2f);
            break;
        }

        case Decoration::filter:
        {
            juce::Path curve;
            const float x0 = area.getX();
            const float x1 = area.getRight();
            const float base = area.getBottom() - 8.0f;
            curve.startNewSubPath (x0, base);
            curve.lineTo (x0 + area.getWidth() * 0.36f, area.getY() + 10.0f);
            curve.quadraticTo (x0 + area.getWidth() * 0.62f,
                               area.getY() + 4.0f,
                               x1, area.getBottom() - 4.0f);

            juce::Path fill (curve);
            fill.lineTo (x1, area.getBottom());
            fill.lineTo (x0, area.getBottom());
            fill.closeSubPath();
            g.setGradientFill (juce::ColourGradient (accent.withAlpha (alpha * 0.32f),
                                                     area.getCentreX(), area.getY(),
                                                     accent.withAlpha (0.01f),
                                                     area.getCentreX(), area.getBottom(), false));
            g.fillPath (fill);

            g.setColour (accent.withAlpha (alpha));
            g.strokePath (curve, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            break;
        }

        case Decoration::filterEnv:
            drawEnvelopePath (g, area.withTrimmedTop (12.0f).withHeight (area.getHeight() * 0.52f),
                              true, accent, alpha);
            break;

        case Decoration::ampEnv:
            drawEnvelopePath (g, area.withTrimmedTop (12.0f).withHeight (area.getHeight() * 0.52f),
                              false, accent, alpha);
            break;

        case Decoration::lfo:
        {
            juce::Path wave;
            const float mid = area.getCentreY();
            const float amp = area.getHeight() * 0.16f;

            for (int i = 0; i <= 96; ++i)
            {
                const float t = (float) i / 96.0f;
                const float x = area.getX() + t * area.getWidth();
                const float y = mid - std::sin (t * 2.0f * kPi * 2.0f) * amp
                                    * (1.0f - 0.35f * t);

                if (i == 0)
                    wave.startNewSubPath (x, y);
                else
                    wave.lineTo (x, y);
            }

            g.setColour (accent.withAlpha (alpha));
            g.strokePath (wave, juce::PathStrokeType (1.25f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            break;
        }

        case Decoration::layer:
        {
            const auto upper = area.withTrimmedBottom (area.getHeight() * 0.44f);
            const auto lower = area.withTrimmedTop (area.getHeight() * 0.44f);
            drawWaveformPath (g, upper, 3, false, accent, alpha * 0.95f);
            drawWaveformPath (g, lower.translated (area.getWidth() * 0.06f, 0.0f),
                              3, true, accent, alpha * 0.62f);
            break;
        }

        case Decoration::touch:
        {
            juce::Path ramp;
            ramp.startNewSubPath (area.getX(), area.getBottom() - 6.0f);
            ramp.quadraticTo (area.getCentreX(), area.getY() + 10.0f,
                              area.getRight(), area.getY() + 4.0f);
            g.setColour (accent.withAlpha (alpha));
            g.strokePath (ramp, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            for (int i = 0; i < 6; ++i)
            {
                const float x = area.getX() + area.getWidth() * (0.08f + 0.18f * (float) i);
                const float y = area.getBottom() - 6.0f
                              - (area.getHeight() - 16.0f) * (0.08f + 0.155f * (float) i);
                g.setColour (accent.withAlpha (alpha));
                g.fillEllipse (x - 1.4f, y - 1.4f, 2.8f, 2.8f);
            }
            break;
        }

        case Decoration::output:
        {
            const juce::Rectangle<float> grille (area.getRight() - area.getWidth() * 0.26f,
                                                 area.getCentreY() - area.getHeight() * 0.25f,
                                                 area.getWidth() * 0.16f, area.getHeight() * 0.50f);
            drawVentGrille (g, grille, accent, 0.55f);
            break;
        }

        case Decoration::none:
            break;
    }
}

//==============================================================================
ParamFader::ParamFader (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId)
    : paramId (parameterId)
{
    if (const auto* spec = findSpec (parameterId.toStdString()))
        label = juce::String (spec->name).toUpperCase();
    else
        label = parameterId.toUpperCase();

    parameter = state.getParameter (parameterId);

    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (260);

    if (parameter != nullptr)
    {
        slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
        slider.setTooltip (parameter->getName (48) + " - double-click to reset");
    }

    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, slider);
    slider.onValueChange = [this] { repaint(); };
}

ParamFader::~ParamFader() = default;

void ParamFader::setAccentColour (juce::Colour newAccent)
{
    accent = newAccent;
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    repaint();
}

void ParamFader::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (parameter != nullptr)
    {
        const auto valueText = formatParamValue (paramId, (float) slider.getValue());
        const bool active = slider.isMouseOverOrDragging();
        const auto textArea = bounds.removeFromTop (17);
        const bool narrow = getWidth() < 34;

        if (active)
        {
            g.setColour (accent.withAlpha (0.18f));
            g.fillRoundedRectangle (textArea.reduced (2, 1).toFloat(), 2.0f);
        }

        drawTrackedText (g, valueText, textArea,
                         active ? accent.brighter (0.42f) : colour::cream.withAlpha (0.90f),
                         juce::Justification::centred,
                         monoFont (narrow ? 7.3f : 9.1f, true), 0.2f);

        if (active)
        {
            g.setColour (accent.withAlpha (0.82f));
            g.fillRect ((float) textArea.getCentreX() - 7.0f, (float) textArea.getBottom() - 1.0f,
                        14.0f, 1.0f);
        }
    }

    const auto labelArea = bounds.removeFromBottom (19);
    drawTrackedText (g, label, labelArea,
                     colour::textDim.brighter (0.14f), juce::Justification::centred,
                     trackedFont (8.1f, 0.9f, true), 0.35f);

    g.setColour (accent.withAlpha (0.42f));
    g.fillRect ((float) labelArea.getCentreX() - 9.0f, (float) labelArea.getY() - 1.0f,
                18.0f, 1.0f);
}

void ParamFader::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (18).withTrimmedBottom (21).reduced (1, 1));
}

//==============================================================================
ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId,
                      int knobSize)
    : paramId (parameterId), knobDiameter (knobSize)
{
    if (const auto* spec = findSpec (parameterId.toStdString()))
        label = juce::String (spec->name).toUpperCase();
    else
        label = parameterId.toUpperCase();

    parameter = state.getParameter (parameterId);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (190);

    if (parameter != nullptr)
    {
        slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
        slider.setTooltip (parameter->getName (48) + " - double-click to reset");
    }

    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, slider);
    slider.onValueChange = [this] { repaint(); };
}

ParamKnob::~ParamKnob() = default;

void ParamKnob::setAccentColour (juce::Colour newAccent)
{
    accent = newAccent;
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    repaint();
}

void ParamKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (parameter != nullptr)
    {
        const auto valueText = formatParamValue (paramId, (float) slider.getValue());
        const bool active = slider.isMouseOverOrDragging();

        drawTrackedText (g, valueText, bounds.removeFromBottom (15),
                         active ? accent.brighter (0.40f) : colour::cream.withAlpha (0.88f),
                         juce::Justification::centred, monoFont (8.8f, true), 0.2f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (13),
                     colour::textDim.brighter (0.14f), juce::Justification::centred,
                     trackedFont (7.9f, 0.7f, true), 0.25f);
}

void ParamKnob::resized()
{
    const int diameter = juce::jmin (knobDiameter, getHeight() - 28);
    slider.setBounds (juce::Rectangle<int> (diameter, diameter)
                          .withCentre ({ getWidth() / 2,
                                        juce::jmax (diameter / 2, getHeight() - 28 - diameter / 2) }));
}

//==============================================================================
ParamChoice::ParamChoice (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId)
    : paramId (parameterId)
{
    if (const auto* spec = findSpec (parameterId.toStdString()))
        label = juce::String (spec->name).toUpperCase();
    else
        label = parameterId.toUpperCase();

    if (auto* parameter = state.getParameter (parameterId))
        choiceParameter = dynamic_cast<juce::AudioParameterChoice*> (parameter);

    if (choiceParameter != nullptr)
        lastIndex = choiceParameter->getIndex();

    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    startTimerHz (12);
}

void ParamChoice::timerCallback()
{
    const int index = choiceParameter != nullptr ? choiceParameter->getIndex() : -1;

    if (index != lastIndex)
    {
        lastIndex = index;
        repaint();
    }
}

void ParamChoice::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto selector = bounds.withTrimmedBottom (25).reduced (1, 7).toFloat();

    drawInsetWell (g, selector, 3.0f);

    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.20f),
                                             selector.getX(), selector.getY(),
                                             accent.withAlpha (0.035f),
                                             selector.getRight(), selector.getBottom(), false));
    g.fillRoundedRectangle (selector.reduced (1.0f), 2.5f);

    g.setColour (accent.withAlpha (0.30f));
    g.drawRoundedRectangle (selector.reduced (0.8f), 2.8f, 0.7f);

    if (choiceParameter != nullptr)
    {
        const auto text = choiceParameter->getCurrentValueAsText().toUpperCase();
        const auto textArea = selector.toNearestInt().withTrimmedLeft (8).withTrimmedRight (18);

        drawTrackedText (g, text, textArea, accent.brighter (0.46f),
                         juce::Justification::centredLeft, monoFont (9.0f, true), 0.35f);

        const float arrowX = selector.getRight() - 9.0f;
        const float arrowY = selector.getCentreY() - 1.0f;
        juce::Path arrow;
        arrow.startNewSubPath (arrowX - 3.5f, arrowY - 2.0f);
        arrow.lineTo (arrowX, arrowY + 2.5f);
        arrow.lineTo (arrowX + 3.5f, arrowY - 2.0f);
        g.setColour (accent.withAlpha (0.90f));
        g.strokePath (arrow, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        drawLed (g, { selector.getX() + 4.0f, selector.getCentreY() - 1.8f, 3.6f, 3.6f },
                 true, accent, 0.45f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (14),
                     colour::textDim.brighter (0.14f), juce::Justification::centred,
                     trackedFont (7.9f, 0.7f, true), 0.25f);
}

void ParamChoice::resized() {}

void ParamChoice::mouseDown (const juce::MouseEvent&)
{
    if (choiceParameter == nullptr)
        return;

    const auto choices = choiceParameter->getAllValueStrings();
    const int current = choiceParameter->getIndex();

    juce::PopupMenu menu;

    for (int i = 0; i < choices.size(); ++i)
        menu.addItem (i + 1, choices[i], true, i == current);

    juce::Component::SafePointer<ParamChoice> safeThis (this);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (140),
                        [safeThis] (int result)
                        {
                            if (safeThis != nullptr && result > 0 && safeThis->choiceParameter != nullptr)
                            {
                                auto* parameter = safeThis->choiceParameter;
                                parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) (result - 1)));
                                safeThis->repaint();
                            }
                        });
}

//==============================================================================
IconButton::IconButton (Icon iconToUse, juce::Colour accentColour)
    : juce::Button ({}), icon (iconToUse), accent (accentColour)
{
    setSize (32, 32);
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool lit = highlighted || down;

    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.4f), 4.0f);

    juce::ColourGradient metal (juce::Colour (0xff2b3841),
                                bounds.getCentreX(), bounds.getY(),
                                juce::Colour (0xff05080b),
                                bounds.getCentreX(), bounds.getBottom(), false);
    metal.addColour (0.44, juce::Colour (0xff131c23));
    g.setGradientFill (metal);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (accent.withAlpha (lit ? 0.92f : 0.34f));
    g.drawRoundedRectangle (bounds.reduced (0.4f), 4.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (bounds.getX() + 5.0f, bounds.getY() + 1.0f,
                bounds.getRight() - 5.0f, bounds.getY() + 1.0f, 0.7f);

    juce::Path path;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float s = bounds.getWidth() * 0.23f;

    switch (icon)
    {
        case Icon::prev:
            path.startNewSubPath (cx + s * 0.55f, cy - s);
            path.lineTo (cx - s * 0.55f, cy);
            path.lineTo (cx + s * 0.55f, cy + s);
            break;

        case Icon::next:
            path.startNewSubPath (cx - s * 0.55f, cy - s);
            path.lineTo (cx + s * 0.55f, cy);
            path.lineTo (cx - s * 0.55f, cy + s);
            break;

        case Icon::close:
            path.startNewSubPath (cx - s * 0.8f, cy - s * 0.8f);
            path.lineTo (cx + s * 0.8f, cy + s * 0.8f);
            path.startNewSubPath (cx + s * 0.8f, cy - s * 0.8f);
            path.lineTo (cx - s * 0.8f, cy + s * 0.8f);
            break;

        case Icon::menu:
            for (int i = -1; i <= 1; ++i)
            {
                path.startNewSubPath (cx - s, cy + (float) i * s * 0.65f);
                path.lineTo (cx + s, cy + (float) i * s * 0.65f);
            }
            break;

        case Icon::power:
        {
            path.addArc (cx - s, cy - s, s * 2.0f, s * 2.0f,
                         juce::MathConstants<float>::pi * 0.25f,
                         juce::MathConstants<float>::pi * 1.75f, true);
            path.startNewSubPath (cx, cy - s * 1.1f);
            path.lineTo (cx, cy - s * 0.15f);
            break;
        }
    }

    if (lit)
        drawGlow (g, { cx, cy }, bounds.getWidth() * 0.8f, accent, 0.34f);

    g.setColour (accent.withAlpha (lit ? 1.0f : 0.80f));
    g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

//==============================================================================
void BrandPlate::paint (juce::Graphics& g)
{
    const auto plate = getLocalBounds().toFloat();

    // Outer shadow, lacquered body and a proud brass frame.
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillRoundedRectangle (plate.expanded (2.4f).translated (0.0f, 3.0f), 7.0f);

    juce::ColourGradient lacquer (juce::Colour (0xff30271d),
                                  plate.getCentreX(), plate.getY(),
                                  juce::Colour (0xff050403),
                                  plate.getCentreX(), plate.getBottom(), false);
    lacquer.addColour (0.32, juce::Colour (0xff1a1510));
    lacquer.addColour (0.72, juce::Colour (0xff0b0907));
    g.setGradientFill (lacquer);
    g.fillRoundedRectangle (plate, 6.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0d59f).withAlpha (0.52f),
                                             plate.getX(), plate.getY(),
                                             juce::Colour (0xff6c4a24).withAlpha (0.24f),
                                             plate.getRight(), plate.getBottom(), false));
    g.drawRoundedRectangle (plate.reduced (1.1f), 5.2f, 1.3f);

    g.setColour (juce::Colours::black.withAlpha (0.94f));
    g.drawRoundedRectangle (plate.reduced (0.5f), 6.0f, 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawLine (plate.getX() + 8.0f, plate.getY() + 1.1f,
                plate.getRight() - 8.0f, plate.getY() + 1.1f, 0.9f);

    drawGlow (g, { plate.getCentreX(), plate.getY() + plate.getHeight() * 0.38f },
              plate.getWidth() * 0.52f, colour::amber, 0.28f);
    drawGlow (g, { plate.getX() + 22.0f, plate.getBottom() - 18.0f },
              plate.getWidth() * 0.44f, colour::aurora, 0.10f);

    // Small maker line.
    drawTrackedText (g, "FREEBUFF AUDIO  /  MODEL E", { 14, 7, 190, 9 },
                     colour::textFaint.withAlpha (0.78f),
                     juce::Justification::centredLeft, trackedFont (5.8f, 1.6f, true), 0.4f);

    // Big chrome wordmark.
    drawChromeText (g, "CS-80E", { 8, 11, getWidth() - 96, 58 },
                    displayFont (49.0f),
                    juce::Colour (0xfffffbf2),
                    juce::Colour (0xffb07d3e), 1.8f);

    // Brass series tag, pinned to the top right.
    const juce::Rectangle<float> tag ((float) getWidth() - 79.0f, 13.0f, 63.0f, 17.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffe4b2),
                                             tag.getCentreX(), tag.getY(),
                                             juce::Colour (0xff7d5627),
                                             tag.getCentreX(), tag.getBottom(), false));
    g.fillRoundedRectangle (tag, 2.2f);
    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.drawRoundedRectangle (tag, 2.2f, 0.8f);
    drawTrackedText (g, "SERIES 5", tag.toNearestInt(), juce::Colour (0xff241506),
                     juce::Justification::centred, trackedFont (6.6f, 1.2f, true), 0.3f);

    // Constitution line.
    drawTrackedText (g, "DUAL CHANNEL POLYPHONIC SYNTHESIZER",
                     { 14, 69, getWidth() - 28, 11 },
                     colour::cream.withAlpha (0.92f),
                     juce::Justification::centredLeft, trackedFont (7.6f, 1.8f), 0.6f);

    g.setColour (colour::amber.withAlpha (0.90f));
    g.fillRect (juce::Rectangle<float> (14.0f, 82.0f, 156.0f, 1.6f));
    g.setColour (colour::amberDim);
    g.fillRect (juce::Rectangle<float> (174.0f, 82.0f, 34.0f, 1.6f));
    g.setColour (colour::aurora.withAlpha (0.52f));
    g.fillRect (juce::Rectangle<float> (14.0f, 85.0f, 30.0f, 1.0f));

    // Feature chips.
    const char* const chips[] = { "8 VOICE", "POLY TOUCH", "RIB BEND" };
    float chipX = 14.0f;

    for (const char* chip : chips)
    {
        const float width = 7.0f * (float) juce::String (chip).length() + 14.0f;
        const juce::Rectangle<float> pill (chipX, 92.0f, width, 17.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff221c15),
                                                 pill.getCentreX(), pill.getY(),
                                                 juce::Colour (0xff070605),
                                                 pill.getCentreX(), pill.getBottom(), false));
        g.fillRoundedRectangle (pill, 2.4f);
        g.setColour (colour::brass.withAlpha (0.34f));
        g.drawRoundedRectangle (pill, 2.4f, 0.7f);

        drawTrackedText (g, chip, pill.toNearestInt(), colour::textDim.withAlpha (0.88f),
                         juce::Justification::centred, trackedFont (5.9f, 1.0f, true), 0.2f);

        chipX += width + 7.0f;
    }

    // Power jewel.
    drawLed (g, { (float) getWidth() - 34.0f, 106.0f, 6.4f, 6.4f }, true, colour::amber, 0.92f);
    drawTrackedText (g, "POWER", { getWidth() - 66, 114, 46, 8 },
                     colour::textFaint, juce::Justification::centredRight,
                     trackedFont (5.2f, 1.0f, true), 0.2f);

    // A tiny engraved saw emblem in the lower right.
    juce::Path emblem;
    const float ex = plate.getRight() - 92.0f;
    const float ey = plate.getBottom() - 16.0f;
    for (int i = 0; i <= 16; ++i)
    {
        const float t = (float) i / 16.0f;
        const float x = ex + t * 40.0f;
        const float y = ey - std::fmod (t * 3.0f, 1.0f) * 11.0f + 5.5f;
        if (i == 0)
            emblem.startNewSubPath (x, y);
        else
            emblem.lineTo (x, y);
    }
    g.setColour (colour::amber.withAlpha (0.40f));
    g.strokePath (emblem, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

//==============================================================================
VoiceLedStrip::VoiceLedStrip (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void VoiceLedStrip::timerCallback()
{
    repaint();
}

void VoiceLedStrip::paint (juce::Graphics& g)
{
    const int count = Engine::kNumVoices;
    const float slotW = 19.5f;
    const float totalWidth = count * slotW;
    const float x = ((float) getWidth() - totalWidth) * 0.5f;
    const float y = 3.0f;

    const juce::Rectangle<float> plate (x - 8.0f, y - 4.0f, totalWidth + 16.0f, 35.0f);
    g.setColour (juce::Colour (0xff030608));
    g.fillRoundedRectangle (plate, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.88f));
    g.drawRoundedRectangle (plate, 3.0f, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawLine (plate.getX() + 4.0f, plate.getY() + 0.8f, plate.getRight() - 4.0f, plate.getY() + 0.8f, 0.7f);

    for (int i = 0; i < count; ++i)
    {
        const float activity = engine.voiceActivity (i);
        const bool on = activity > 0.002f;
        const float intensity = juce::jlimit (0.25f, 1.0f, activity * 1.7f);

        const float cx = x + (float) i * slotW + slotW * 0.5f;

        drawTrackedText (g, juce::String (i + 1),
                         { (int) (cx - 7.0f), (int) (y + 22.0f), 14, 8 },
                         on ? colour::amberHot : colour::textFaint.withAlpha (0.72f),
                         juce::Justification::centred, monoFont (5.8f, true), 0.2f);

        // Two lamps per voice slot: channel I and channel II, the twin cards.
        drawLed (g, { cx - 4.8f, y + 3.0f, 3.6f, 3.6f }, on, colour::amber, intensity);

        const bool secondLit = on && engine.params().layerMode > 0.5f;
        drawLed (g, { cx + 1.2f, y + 3.0f, 3.6f, 3.6f }, secondLit,
                 secondLit ? colour::aurora : colour::amberDim, intensity * 0.9f);

        if (i < count - 1)
        {
            g.setColour (juce::Colours::white.withAlpha (0.045f));
            g.drawLine (x + (float) (i + 1) * slotW, y + 1.0f,
                        x + (float) (i + 1) * slotW, y + 18.0f, 0.6f);
        }
    }
}
} // namespace cs80::ui
