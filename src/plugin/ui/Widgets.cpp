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

    if (id == pid::bendRange)
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
        || id == pid::layerLevel || id == pid::layerRes || id == pid::layerPw)
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
    const float amp = area.getHeight() * 0.32f;
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

    g.setColour (colour.withAlpha (alpha));
    g.strokePath (path, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

void drawEnvelopePath (juce::Graphics& g, juce::Rectangle<float> area,
                       bool filterShape, juce::Colour colour, float alpha)
{
    const float x = area.getX();
    const float y = area.getY();
    const float w = area.getWidth();
    const float h = area.getHeight();
    const float base = y + h * 0.82f;

    juce::Path path;
    const float start = filterShape ? 0.42f : 0.0f;

    path.startNewSubPath (x, base - h * 0.55f * start);
    path.lineTo (x + w * 0.16f, y + h * 0.16f);
    path.lineTo (x + w * 0.44f, base - h * 0.26f);
    path.lineTo (x + w * 0.78f, base - h * 0.26f);
    path.lineTo (x + w * 0.95f, base);

    g.setColour (colour.withAlpha (alpha));
    g.strokePath (path, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    juce::Path fill (path);
    fill.lineTo (x + w * 0.95f, area.getBottom());
    fill.lineTo (x, area.getBottom());
    fill.closeSubPath();
    g.setColour (colour.withAlpha (alpha * 0.20f));
    g.fillPath (fill);
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

    drawPlate (g, bounds, 3.0f);

    // Accent rail across the top of the section.
    g.setColour (accent.withAlpha (0.70f));
    g.drawLine (8.0f, 1.2f, (float) getWidth() - 8.0f, 1.2f, 1.1f);
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.10f),
                                             (float) getWidth() * 0.5f, 1.0f,
                                             juce::Colours::transparentBlack,
                                             (float) getWidth() * 0.5f, 18.0f, false));
    g.fillRect (juce::Rectangle<float> (6.0f, 1.0f, (float) getWidth() - 12.0f, 18.0f));

    // Recessed header band, very lightly tinted with the section colour.
    auto header = bounds.removeFromTop ((float) titleHeight).reduced (2.0f, 1.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff070a0d),
                                             header.getCentreX(), header.getY(),
                                             juce::Colour (0xff141b22),
                                             header.getCentreX(), header.getBottom(), false));
    g.fillRoundedRectangle (header, 2.0f);

    g.setColour (accent.withAlpha (0.045f));
    g.fillRoundedRectangle (header, 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (header, 2.0f, 0.8f);

    // Accent chip + title.
    g.setColour (accent.withAlpha (0.95f));
    g.fillRoundedRectangle (13.0f, 10.0f, 2.5f, 12.0f, 1.2f);
    drawGlow (g, { 14.25f, 16.0f }, 18.0f, accent, 0.30f);

    drawEngravedText (g, title, { 21, 7, getWidth() - 50, 15 },
                      colour::cream.withAlpha (0.92f),
                      juce::Justification::centredLeft,
                      trackedFont (9.5f, 1.7f, true), 0.5f);

    if (subtitle.isNotEmpty())
        drawEngravedText (g, subtitle, { 21, 20, getWidth() - 50, 9 },
                          colour::textFaint.brighter (0.18f),
                          juce::Justification::centredLeft,
                          monoFont (6.4f), 0.3f);

    if (number > 0)
        drawTrackedText (g, juce::String (number).paddedLeft ('0', 2),
                         { getWidth() - 29, 8, 19, 10 },
                         accent.withAlpha (0.34f), juce::Justification::centredRight,
                         monoFont (6.5f, true), 0.4f);

    // Gold hairline under the header.
    g.setColour (accent.withAlpha (0.20f));
    g.drawLine (7.0f, (float) titleHeight - 0.5f, (float) getWidth() - 7.0f,
                (float) titleHeight - 0.5f, 0.8f);

    paintDecoration (g, getLocalBounds().toFloat().withTrimmedTop ((float) titleHeight + 2.0f));
}

void SectionPanel::paintDecoration (juce::Graphics& g, juce::Rectangle<float> body)
{
    if (decoration == Decoration::none || body.getHeight() < 30.0f)
        return;

    const auto area = body.reduced (14.0f, 20.0f);
    const float alpha = 0.105f;

    switch (decoration)
    {
        case Decoration::saw:
            drawWaveformPath (g, area.withHeight (area.getHeight() * 0.5f), 3, false, accent, alpha);
            break;

        case Decoration::pulse:
            drawWaveformPath (g, area.withHeight (area.getHeight() * 0.5f), 2, true, accent, alpha);
            break;

        case Decoration::ring:
        {
            const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.22f;
            const auto c = area.getCentre();
            g.setColour (accent.withAlpha (alpha));
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f)
                               .withCentre ({ c.x - r * 0.55f, c.y }), 1.2f);
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f)
                               .withCentre ({ c.x + r * 0.55f, c.y }), 1.2f);
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
            g.setColour (accent.withAlpha (alpha * 0.30f));
            g.fillPath (fill);

            g.setColour (accent.withAlpha (alpha));
            g.strokePath (curve, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            break;
        }

        case Decoration::filterEnv:
            drawEnvelopePath (g, area.withTrimmedTop (16.0f).withHeight (area.getHeight() * 0.6f),
                              true, accent, alpha);
            break;

        case Decoration::ampEnv:
            drawEnvelopePath (g, area.withTrimmedTop (16.0f).withHeight (area.getHeight() * 0.6f),
                              false, accent, alpha);
            break;

        case Decoration::lfo:
        {
            juce::Path wave;
            const float mid = area.getCentreY();
            const float amp = area.getHeight() * 0.16f;

            for (int i = 0; i <= 80; ++i)
            {
                const float t = (float) i / 80.0f;
                const float x = area.getX() + t * area.getWidth();
                const float y = mid - std::sin (t * 2.0f * kPi * 2.0f) * amp;

                if (i == 0)
                    wave.startNewSubPath (x, y);
                else
                    wave.lineTo (x, y);
            }

            g.setColour (accent.withAlpha (alpha));
            g.strokePath (wave, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
            break;
        }

        case Decoration::layer:
        {
            const auto upper = area.withTrimmedBottom (area.getHeight() * 0.42f);
            const auto lower = area.withTrimmedTop (area.getHeight() * 0.42f);
            drawWaveformPath (g, upper, 3, false, accent, alpha * 0.95f);
            drawWaveformPath (g, lower.translated (area.getWidth() * 0.08f, 0.0f),
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

            for (int i = 0; i < 5; ++i)
            {
                const float x = area.getX() + area.getWidth() * (0.1f + 0.2f * (float) i);
                const float y = area.getBottom() - 6.0f - (area.getHeight() - 16.0f) * (0.1f + 0.17f * (float) i);
                g.fillEllipse (x - 1.3f, y - 1.3f, 2.6f, 2.6f);
            }
            break;
        }

        case Decoration::output:
        {
            // A faint speaker grille in the corner.
            const juce::Rectangle<float> grille (area.getRight() - area.getWidth() * 0.30f,
                                                 area.getCentreY() - area.getHeight() * 0.26f,
                                                 area.getWidth() * 0.20f, area.getHeight() * 0.52f);
            g.setColour (accent.withAlpha (alpha * 0.75f));

            for (int i = 0; i < 7; ++i)
            {
                const float x = grille.getX() + grille.getWidth() * (float) i / 7.0f;
                g.drawLine (x, grille.getY(), x, grille.getBottom(), 1.0f);
            }
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

void ParamFader::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (parameter != nullptr)
    {
        const float value = slider.getValue();
        const auto valueText = formatParamValue (paramId, (float) value);
        const bool active = slider.isMouseOverOrDragging();

        drawTrackedText (g, valueText, bounds.removeFromTop (16),
                         active ? colour::amberHot : colour::cream.withAlpha (0.74f),
                         juce::Justification::centred, monoFont (8.6f, true), 0.35f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (19),
                     colour::textDim.brighter (0.10f), juce::Justification::centred,
                     trackedFont (7.6f, 0.9f, true), 0.3f);
}

void ParamFader::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (17).withTrimmedBottom (20).reduced (1, 1));
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

void ParamKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (parameter != nullptr)
    {
        const auto valueText = formatParamValue (paramId, (float) slider.getValue());

        drawTrackedText (g, valueText, bounds.removeFromBottom (12),
                         slider.isMouseOverOrDragging() ? colour::amberHot
                                                        : colour::cream.withAlpha (0.70f),
                         juce::Justification::centred, monoFont (8.0f, true), 0.25f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (12),
                     colour::textDim.brighter (0.08f), juce::Justification::centred,
                     trackedFont (7.4f, 0.7f, true), 0.2f);
}

void ParamKnob::resized()
{
    const int diameter = juce::jmin (knobDiameter, getHeight() - 26);
    slider.setBounds (juce::Rectangle<int> (diameter, diameter)
                          .withCentre ({ getWidth() / 2, juce::jmax (diameter / 2, getHeight() - 26 - diameter / 2) }));
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
    const auto selector = bounds.withTrimmedBottom (25).reduced (1, 6).toFloat();

    drawInsetWell (g, selector, 3.0f);

    if (choiceParameter != nullptr)
    {
        const auto text = choiceParameter->getCurrentValueAsText().toUpperCase();
        const auto textArea = selector.toNearestInt().withTrimmedLeft (7).withTrimmedRight (17);

        drawTrackedText (g, text, textArea, colour::amberHot, juce::Justification::centredLeft,
                         monoFont (8.2f, true), 0.35f);

        const float arrowX = selector.getRight() - 9.0f;
        const float arrowY = selector.getCentreY() - 1.0f;
        juce::Path arrow;
        arrow.startNewSubPath (arrowX - 3.5f, arrowY - 2.0f);
        arrow.lineTo (arrowX, arrowY + 2.5f);
        arrow.lineTo (arrowX + 3.5f, arrowY - 2.0f);
        g.setColour (colour::amber.withAlpha (0.85f));
        g.strokePath (arrow, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    drawTrackedText (g, label, bounds.removeFromBottom (14),
                     colour::textDim.brighter (0.08f), juce::Justification::centred,
                     trackedFont (7.4f, 0.7f, true), 0.2f);
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

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (120),
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
    setSize (30, 30);
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool lit = highlighted || down;

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 3.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222a32),
                                             0.0f, bounds.getY(),
                                             juce::Colour (0xff070a0d),
                                             0.0f, bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (accent.withAlpha (lit ? 0.85f : 0.35f));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    juce::Path path;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float s = bounds.getWidth() * 0.24f;

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
        drawGlow (g, { cx, cy }, bounds.getWidth() * 0.7f, accent, 0.32f);

    g.setColour (accent.withAlpha (lit ? 1.0f : 0.78f));
    g.strokePath (path, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

//==============================================================================
void BrandPlate::paint (juce::Graphics& g)
{
    const auto plate = juce::Rectangle<float> (0.0f, 0.0f, (float) getWidth(), 68.0f);

    // Inset brand plate with a fine brass edge.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff121920),
                                             plate.getCentreX(), plate.getY(),
                                             juce::Colour (0xff05070a),
                                             plate.getCentreX(), plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 4.0f);

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle (plate.reduced (0.5f), 4.0f, 1.2f);
    g.setColour (juce::Colour (0xffb3874f).withAlpha (0.34f));
    g.drawRoundedRectangle (plate.reduced (1.5f), 3.2f, 0.8f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f),
                                             plate.getCentreX(), plate.getY(),
                                             juce::Colours::transparentWhite,
                                             plate.getCentreX(), plate.getY() + 28.0f, false));
    g.fillRoundedRectangle (plate.reduced (2.0f), 3.0f);

    const auto logoArea = juce::Rectangle<int> (12, 0, getWidth() - 24, 58);
    const auto logoFont = displayFont (44.0f);

    // Build real glyph outlines so the badge can carry a chrome gradient.
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (logoFont, "CS-80E",
                          (float) logoArea.getX(),
                          (float) logoArea.getY() + 43.0f);

    juce::Path logo;
    glyphs.createPath (logo);

    juce::Path shadow (logo);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillPath (shadow, juce::AffineTransform::translation (0.0f, 2.2f));

    juce::ColourGradient chrome (juce::Colour (0xfffff4de),
                                 logoArea.getCentreX(), (float) logoArea.getY(),
                                 juce::Colour (0xffb58d55),
                                 logoArea.getCentreX(), (float) logoArea.getY() + 52.0f, false);
    chrome.addColour (0.40, juce::Colour (0xfff6e8cc));
    chrome.addColour (0.62, juce::Colour (0xffcbaa74));
    g.setGradientFill (chrome);
    g.fillPath (logo);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (logo, juce::PathStrokeType (0.6f));

    // Gold underline, broken into a long and a short segment.
    g.setColour (colour::amber.withAlpha (0.92f));
    g.fillRect (juce::Rectangle<float> (13.0f, 57.0f, 226.0f, 1.6f));
    g.setColour (colour::amberDim);
    g.fillRect (juce::Rectangle<float> (245.0f, 57.0f, 26.0f, 1.6f));

    // A tiny power jewel on the plate.
    drawLed (g, { (float) getWidth() - 20.0f, 14.0f, 5.0f, 5.0f }, true, colour::amber, 0.55f);

    drawTrackedText (g, "DUAL CHANNEL POLYPHONIC SYNTHESIZER", { 2, 72, getWidth(), 14 },
                     colour::cream.withAlpha (0.82f), juce::Justification::centredLeft,
                     trackedFont (7.8f, 2.2f, false), 0.7f);

    drawTrackedText (g, "CS-80 TRIBUTE   /   MODEL E   /   SERIES 3", { 2, 87, getWidth(), 11 },
                     colour::textFaint.brighter (0.30f), juce::Justification::centredLeft,
                     monoFont (6.8f), 0.2f);

    drawTrackedText (g, "CH I  ·  CH II", { getWidth() - 110, 70, 100, 10 },
                     colour::amber.withAlpha (0.72f), juce::Justification::centredRight,
                     trackedFont (6.2f, 1.2f, true), 0.3f);
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
    const float ledSize = 8.0f;
    const float slotW = 15.0f;
    const float totalWidth = count * slotW;
    const float x = ((float) getWidth() - totalWidth) * 0.5f;
    const float y = 2.0f;

    // Inset panel with a black-anodised face.
    g.setColour (juce::Colour (0xff050709));
    g.fillRoundedRectangle (juce::Rectangle<float> (x - 6.0f, y - 2.0f,
                                                    totalWidth + 12.0f, 26.0f), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (juce::Rectangle<float> (x - 6.0f, y - 2.0f,
                                                    totalWidth + 12.0f, 26.0f), 3.0f, 0.9f);

    for (int i = 0; i < count; ++i)
    {
        const float activity = engine.voiceActivity (i);
        const bool on = activity > 0.002f;
        const float intensity = juce::jlimit (0.25f, 1.0f, activity * 1.6f);

        const float cx = x + (float) i * slotW + slotW * 0.5f;

        // Faint socket ring.
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawEllipse (juce::Rectangle<float> (ledSize + 3.0f, ledSize + 3.0f)
                           .withCentre ({ cx, y + 8.0f }), 0.7f);

        drawLed (g, { cx - ledSize * 0.5f, y + 4.0f, ledSize, ledSize }, on,
                 colour::amber, intensity);

        drawTrackedText (g, juce::String (i + 1),
                         { (int) (cx - 6.0f), (int) (y + 15.0f), 12, 7 },
                         on ? colour::amberHot : colour::textFaint,
                         juce::Justification::centred, monoFont (5.6f, true), 0.2f);
    }
}
} // namespace cs80::ui
