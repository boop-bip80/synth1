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

    drawPlate (g, bounds, 4.5f);

    // A soft section-coloured wash behind the controls.
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds.reduced (1.0f), 4.0f);
        g.reduceClipRegion (clip);

        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.105f),
                                                 bounds.getCentreX(), bounds.getY(),
                                                 accent.withAlpha (0.012f),
                                                 bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.72f, false));
        g.fillRect (bounds);

        drawGlow (g, { bounds.getX() + 30.0f, bounds.getY() + 20.0f },
                  bounds.getWidth() * 0.52f, accent, 0.16f);
    }

    // Lit rail across the top, brightest at the left.
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.94f),
                                             10.0f, 1.6f,
                                             accent.withAlpha (0.08f),
                                             (float) getWidth() - 10.0f, 1.6f, false));
    g.fillRect (juce::Rectangle<float> (10.0f, 1.4f, (float) getWidth() - 20.0f, 2.0f));
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.14f),
                                             (float) getWidth() * 0.5f, 1.0f,
                                             juce::Colours::transparentBlack,
                                             (float) getWidth() * 0.5f, 26.0f, false));
    g.fillRect (juce::Rectangle<float> (7.0f, 1.0f, (float) getWidth() - 14.0f, 26.0f));

    // Recessed, tinted header band.
    auto header = bounds.removeFromTop ((float) titleHeight).reduced (2.2f, 1.6f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff04070a),
                                             header.getCentreX(), header.getY(),
                                             juce::Colour (0xff111a21),
                                             header.getCentreX(), header.getBottom(), false));
    g.fillRoundedRectangle (header, 2.4f);

    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.22f),
                                             header.getX(), header.getCentreY(),
                                             juce::Colours::transparentBlack,
                                             header.getX() + 92.0f, header.getCentreY(), false));
    g.fillRoundedRectangle (header, 2.4f);

    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.drawRoundedRectangle (header, 2.4f, 0.9f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (header.getX() + 3.0f, header.getY() + 0.8f,
                header.getRight() - 3.0f, header.getY() + 0.8f, 0.7f);

    // Accent jewel at the left edge.
    g.setColour (accent.withAlpha (0.99f));
    g.fillRoundedRectangle (12.0f, 9.0f, 3.2f, 16.0f, 1.6f);
    drawGlow (g, { 13.6f, 17.0f }, 22.0f, accent, 0.38f);

    drawEngravedText (g, title, { 22, 5, getWidth() - 58, 17 },
                      colour::cream.withAlpha (0.98f),
                      juce::Justification::centredLeft,
                      trackedFont (10.2f, 1.9f, true), 0.5f);

    if (subtitle.isNotEmpty())
        drawEngravedText (g, subtitle, { 22, 20, getWidth() - 58, 9 },
                          colour::textFaint.brighter (0.28f),
                          juce::Justification::centredLeft,
                          monoFont (6.1f), 0.3f);

    // Numbered card plate at the right, like a slot stamp.
    if (number > 0)
    {
        const juce::Rectangle<float> plate ((float) getWidth() - 31.0f, 8.0f, 20.0f, 18.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b252d),
                                                 plate.getCentreX(), plate.getY(),
                                                 juce::Colour (0xff030608),
                                                 plate.getCentreX(), plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 2.0f);

        g.setColour (accent.withAlpha (0.40f));
        g.drawRoundedRectangle (plate.reduced (0.5f), 2.0f, 0.7f);

        drawTrackedText (g, juce::String (number).paddedLeft ('0', 2),
                         plate.toNearestInt(), accent.withAlpha (0.78f),
                         juce::Justification::centred,
                         monoFont (6.6f, true), 0.4f);
    }

    g.setColour (accent.withAlpha (0.26f));
    g.drawLine (7.0f, (float) titleHeight - 0.5f, (float) getWidth() - 7.0f,
                (float) titleHeight - 0.5f, 0.8f);

    // Fine corner marks, a small detail that makes the card feel machined.
    const float len = 7.0f;
    const float inset = 5.0f;
    g.setColour (accent.withAlpha (0.26f));
    g.drawLine (inset, inset, inset + len, inset, 0.7f);
    g.drawLine (inset, inset, inset, inset + len, 0.7f);
    g.drawLine ((float) getWidth() - inset, (float) getHeight() - inset,
                (float) getWidth() - inset - len, (float) getHeight() - inset, 0.7f);
    g.drawLine ((float) getWidth() - inset, (float) getHeight() - inset,
                (float) getWidth() - inset, (float) getHeight() - inset - len, 0.7f);

    paintDecoration (g, getLocalBounds().toFloat().withTrimmedTop ((float) titleHeight + 2.0f));
}

void SectionPanel::paintDecoration (juce::Graphics& g, juce::Rectangle<float> body)
{
    if (decoration == Decoration::none || body.getHeight() < 30.0f)
        return;

    const auto area = body.reduced (18.0f, 28.0f);
    const float alpha = 0.135f;

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
            g.setColour (accent.withAlpha (0.16f));
            g.fillRoundedRectangle (textArea.reduced (2, 1).toFloat(), 2.0f);
        }

        drawTrackedText (g, valueText, textArea,
                         active ? accent.brighter (0.38f) : colour::cream.withAlpha (0.82f),
                         juce::Justification::centred,
                         monoFont (narrow ? 7.0f : 8.5f, true), 0.2f);

        if (active)
        {
            g.setColour (accent.withAlpha (0.82f));
            g.fillRect ((float) textArea.getCentreX() - 7.0f, (float) textArea.getBottom() - 1.0f,
                        14.0f, 1.0f);
        }
    }

    drawTrackedText (g, label, bounds.removeFromBottom (19),
                     colour::textDim.brighter (0.16f), juce::Justification::centred,
                     trackedFont (7.5f, 0.9f, true), 0.3f);
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
                         active ? accent.brighter (0.34f) : colour::cream.withAlpha (0.78f),
                         juce::Justification::centred, monoFont (8.0f, true), 0.2f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (13),
                     colour::textDim.brighter (0.14f), juce::Justification::centred,
                     trackedFont (7.2f, 0.7f, true), 0.2f);
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

        drawTrackedText (g, text, textArea, accent.brighter (0.40f),
                         juce::Justification::centredLeft, monoFont (8.2f, true), 0.35f);

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
                     trackedFont (7.2f, 0.7f, true), 0.2f);
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
    const auto plate = juce::Rectangle<float> (0.0f, 0.0f, (float) getWidth(), 92.0f);

    g.setColour (juce::Colours::black.withAlpha (0.66f));
    g.fillRoundedRectangle (plate.expanded (2.0f).translated (0.0f, 2.0f), 6.0f);

    juce::ColourGradient lacquer (juce::Colour (0xff202a31),
                                  plate.getCentreX(), plate.getY(),
                                  juce::Colour (0xff030507),
                                  plate.getCentreX(), plate.getBottom(), false);
    lacquer.addColour (0.36, juce::Colour (0xff11181e));
    g.setGradientFill (lacquer);
    g.fillRoundedRectangle (plate, 5.0f);

    g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.10f),
                                             plate.getCentreX(), plate.getY(),
                                             juce::Colours::transparentBlack,
                                             plate.getCentreX(), plate.getY() + 60.0f, false));
    g.fillRoundedRectangle (plate, 5.0f);

    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.drawRoundedRectangle (plate.reduced (0.5f), 5.0f, 1.2f);

    g.setColour (juce::Colour (0xffd6ad76).withAlpha (0.52f));
    g.drawRoundedRectangle (plate.reduced (2.0f), 4.0f, 0.9f);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawLine (plate.getX() + 6.0f, plate.getY() + 1.2f,
                plate.getRight() - 6.0f, plate.getY() + 1.2f, 0.8f);

    // Soft bloom behind the chrome.
    drawGlow (g, { plate.getCentreX(), plate.getY() + 36.0f }, 120.0f, colour::amber, 0.18f);

    drawChromeText (g, "CS-80E", { 12, 4, getWidth() - 24, 58 },
                    displayFont (47.0f),
                    juce::Colour (0xfffffaf0),
                    juce::Colour (0xffb7884d), 1.4f);

    // Series tag, in brass.
    const juce::Rectangle<float> tag ((float) getWidth() - 83.0f, 12.0f, 66.0f, 17.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2d39c),
                                             tag.getCentreX(), tag.getY(),
                                             juce::Colour (0xff7a5224),
                                             tag.getCentreX(), tag.getBottom(), false));
    g.fillRoundedRectangle (tag, 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (tag, 2.0f, 0.8f);
    drawTrackedText (g, "SERIES 5", tag.toNearestInt(), juce::Colour (0xff251607),
                     juce::Justification::centred, trackedFont (6.0f, 1.1f, true), 0.3f);

    // Warm rule and cool counter-rule.
    g.setColour (colour::amber.withAlpha (0.98f));
    g.fillRect (juce::Rectangle<float> (12.0f, 72.0f, 222.0f, 1.8f));
    g.setColour (colour::amberDim);
    g.fillRect (juce::Rectangle<float> (240.0f, 72.0f, 42.0f, 1.8f));
    g.setColour (colour::aurora.withAlpha (0.50f));
    g.fillRect (juce::Rectangle<float> (12.0f, 75.0f, 38.0f, 1.0f));

    drawLed (g, { (float) getWidth() - 14.0f, 40.0f, 4.4f, 4.4f }, true, colour::amber, 0.55f);

    drawTrackedText (g, "DUAL CHANNEL POLYPHONIC SYNTHESIZER",
                     { 12, 79, getWidth() - 24, 10 },
                     colour::cream.withAlpha (0.90f), juce::Justification::centredLeft,
                     trackedFont (7.2f, 2.0f, false), 0.6f);

    drawTrackedText (g, "EIGHT VOICE  ·  DUAL CARD  ·  61 KEY",
                     { 12, 92, getWidth() - 24, 9 },
                     colour::textFaint.brighter (0.32f), juce::Justification::centredLeft,
                     monoFont (6.1f), 0.3f);

    drawTrackedText (g, "ANALOG SOUL / MODERN BUILD",
                     { 12, 102, getWidth() - 24, 8 },
                     colour::amber.withAlpha (0.66f), juce::Justification::centredLeft,
                     trackedFont (5.2f, 1.3f, true), 0.2f);

    // A tiny engraved waveform emblem at the right of the plaque.
    juce::Path emblem;
    const float ex = plate.getRight() - 54.0f;
    const float ey = 54.0f;
    for (int i = 0; i <= 24; ++i)
    {
        const float t = (float) i / 24.0f;
        const float x = ex + t * 40.0f;
        const float y = ey - std::sin (t * 2.0f * kPi * 2.0f) * 7.0f * (0.35f + t * 0.65f);
        if (i == 0)
            emblem.startNewSubPath (x, y);
        else
            emblem.lineTo (x, y);
    }
    g.setColour (colour::amber.withAlpha (0.42f));
    g.strokePath (emblem, juce::PathStrokeType (0.9f, juce::PathStrokeType::curved,
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
