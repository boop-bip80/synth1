#include "Widgets.h"

#include "../../dsp/Params.h"

#include <cmath>

namespace cs80::ui
{
namespace
{
juce::String formatParamValue (const juce::String& id, float value)
{
    if (id == pid::vco1Octave || id == pid::vco2Octave)
    {
        const int octave = (int) std::round (value);
        return (octave > 0 ? "+" : "") + juce::String (octave) + " OCT";
    }

    if (id == pid::bendRange)
        return juce::String ((int) std::round (value)) + " ST";

    if (id == pid::fA || id == pid::fD || id == pid::fR
        || id == pid::aA || id == pid::aD || id == pid::aR
        || id == pid::lfoDelay)
        return formatTime (value);

    if (id == pid::hpfCutoff || id == pid::lpfCutoff)
        return formatHz (value);

    if (id == pid::lfoRate)
        return juce::String (value, 2) + " HZ";

    if (id == pid::master)
        return juce::String (value, 1) + " DB";

    if (id == pid::vco1Detune || id == pid::vco2Detune)
        return (value > 0.0f ? "+" : "") + juce::String ((int) std::round (value)) + " CT";

    if (id == pid::fIl || id == pid::fAl || id == pid::fS
        || id == pid::aS || id == pid::vco1Level || id == pid::vco2Level
        || id == pid::vco2Pw || id == pid::ring || id == pid::noise
        || id == pid::hpfRes || id == pid::lpfRes || id == pid::ensemble
        || id == pid::space || id == pid::lfoPwm || id == pid::lfoFilter
        || id == pid::lfoAmp || id == pid::lfoPitch || id == pid::aftertouch
        || id == pid::spread || id == pid::velocity)
        return juce::String (juce::roundToInt (value * 100.0f)) + "%";

    return juce::String (value, 2);
}
} // namespace

//==============================================================================
SectionPanel::SectionPanel (juce::String sectionTitle, juce::String sectionSubtitle)
    : title (std::move (sectionTitle)), subtitle (std::move (sectionSubtitle))
{
    setInterceptsMouseClicks (false, false);
}

void SectionPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    drawPlate (g, bounds, 4.0f);

    // A darker engraved header band.
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (bounds.removeFromTop (26.0f).reduced (2.0f, 1.0f), 3.0f);

    g.setColour (colour::amber.withAlpha (0.50f));
    g.fillRoundedRectangle (7.0f, 8.0f, 2.0f, 9.0f, 1.0f);

    drawEngravedText (g, title, { 13, 4, getWidth() - 22, 15 }, colour::textDim.brighter (0.18f),
                      juce::Justification::centredLeft, trackedFont (9.2f, 1.8f, true), 0.5f);

    if (subtitle.isNotEmpty())
        drawEngravedText (g, subtitle, { 13, 17, getWidth() - 22, 9 }, colour::textFaint.brighter (0.32f),
                          juce::Justification::centredLeft, monoFont (6.8f), 0.25f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (7.0f, 27.0f, (float) getWidth() - 7.0f, 27.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawLine (7.0f, 28.0f, (float) getWidth() - 7.0f, 28.0f, 1.0f);
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
    slider.setMouseDragSensitivity (240);

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

        drawTrackedText (g, valueText, bounds.removeFromTop (15),
                         active ? colour::amberHot : colour::textDim,
                         juce::Justification::centred, monoFont (8.8f, true), 0.45f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (17),
                     colour::textDim.brighter (0.12f), juce::Justification::centred,
                     trackedFont (7.9f, 1.0f, true), 0.3f);
}

void ParamFader::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (16).withTrimmedBottom (18).reduced (1, 2));
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
    slider.setMouseDragSensitivity (180);

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

        drawTrackedText (g, valueText, bounds.removeFromBottom (13),
                         slider.isMouseOverOrDragging() ? colour::amberHot : colour::textDim,
                         juce::Justification::centred, monoFont (8.2f, true), 0.3f);
    }

    drawTrackedText (g, label, bounds.removeFromBottom (13),
                     colour::textDim.brighter (0.12f), juce::Justification::centred,
                     trackedFont (7.8f, 0.9f, true), 0.25f);
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
    const auto selector = bounds.withTrimmedBottom (18).reduced (2, 7).toFloat();

    drawInsetWell (g, selector, 3.0f);

    if (choiceParameter != nullptr)
    {
        const auto text = choiceParameter->getCurrentValueAsText().toUpperCase();
        const auto textArea = selector.toNearestInt().withTrimmedLeft (7).withTrimmedRight (17);

        drawTrackedText (g, text, textArea, colour::amber, juce::Justification::centredLeft,
                         monoFont (8.4f, true), 0.45f);

        const float arrowX = selector.getRight() - 9.0f;
        const float arrowY = selector.getCentreY() - 1.0f;
        juce::Path arrow;
        arrow.startNewSubPath (arrowX - 3.5f, arrowY - 2.0f);
        arrow.lineTo (arrowX, arrowY + 2.5f);
        arrow.lineTo (arrowX + 3.5f, arrowY - 2.0f);
        g.setColour (colour::amber.withAlpha (0.8f));
        g.strokePath (arrow, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    drawTrackedText (g, label, bounds.removeFromBottom (13),
                     colour::textDim.brighter (0.12f), juce::Justification::centred,
                     trackedFont (7.8f, 0.9f, true), 0.25f);
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
    setSize (28, 28);
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool lit = highlighted || down;

    if (lit)
    {
        g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : 0.06f));
        g.fillRoundedRectangle (bounds, 3.0f);
    }

    g.setColour (accent.withAlpha (lit ? 0.95f : 0.55f));
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
        drawGlow (g, { cx, cy }, bounds.getWidth() * 0.65f, accent, 0.35f);

    g.setColour (accent.withAlpha (lit ? 1.0f : 0.7f));
    g.strokePath (path, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

//==============================================================================
void BrandPlate::paint (juce::Graphics& g)
{
    const juce::Rectangle<int> logoArea (0, 0, getWidth(), 54);
    const auto logoFont = displayFont (43.0f);

    g.setFont (logoFont);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("CS-80E", logoArea.translated (1, 2), juce::Justification::centredLeft, false);

    g.setColour (colour::text.brighter (0.12f));
    g.drawText ("CS-80E", logoArea, juce::Justification::centredLeft, false);

    g.setColour (colour::amber.withAlpha (0.10f));
    g.drawText ("CS-80E", logoArea.translated (0, 4), juce::Justification::centredLeft, false);

    // Amber underline, broken into a long and a short segment.
    g.setColour (colour::amber);
    g.fillRect (2, 52, 232, 2);
    g.setColour (colour::amberDim);
    g.fillRect (240, 52, 18, 2);

    drawTrackedText (g, "POLYPHONIC SYNTHESIZER", { 2, 62, getWidth(), 14 },
                     colour::textDim.brighter (0.15f), juce::Justification::centredLeft,
                     trackedFont (8.8f, 2.8f, false), 0.8f);

    drawTrackedText (g, "TRIBUTE MODEL  /  8 VOICE  /  DUAL VCO", { 2, 79, getWidth(), 12 },
                     colour::textFaint.brighter (0.30f), juce::Justification::centredLeft,
                     monoFont (7.0f), 0.2f);
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
    const float ledSize = 9.0f;
    const float gap = 7.0f;
    const float totalWidth = count * ledSize + (count - 1) * gap;
    const float x = ((float) getWidth() - totalWidth) * 0.5f;
    const float y = 5.0f;

    for (int i = 0; i < count; ++i)
    {
        const float activity = engine.voiceActivity (i);
        const bool on = activity > 0.002f;
        const float intensity = juce::jlimit (0.25f, 1.0f, activity * 1.6f);

        drawLed (g, { x + (float) i * (ledSize + gap), y, ledSize, ledSize }, on,
                 colour::amber, intensity);
    }
}
} // namespace cs80::ui
