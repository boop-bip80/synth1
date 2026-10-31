#include "LookAndFeel.h"

#include <cmath>

namespace cs80::ui
{
Cs80LookAndFeel::Cs80LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colour::windowBg);

    setColour (juce::Label::textColourId, colour::text);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0e1013));
    setColour (juce::ComboBox::textColourId, colour::amber);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::arrowColourId, colour::amberDim);
    setColour (juce::ComboBox::focusedOutlineColourId, colour::amber.withAlpha (0.5f));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff121519));
    setColour (juce::PopupMenu::textColourId, colour::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::amber.withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId, colour::amberHot);
    setColour (juce::PopupMenu::headerTextColourId, colour::textDim);

    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff0d0f12));
    setColour (juce::TooltipWindow::textColourId, colour::amberHot);
    setColour (juce::TooltipWindow::outlineColourId, colour::amber.withAlpha (0.28f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0d10));
    setColour (juce::TextEditor::textColourId, colour::text);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, colour::amber.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, colour::amber);

    setColour (juce::ScrollBar::thumbColourId, colour::line.brighter (0.2f));
}

//==============================================================================
void Cs80LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float centreX = bounds.getCentreX();
    const float top = bounds.getY() + 10.0f;
    const float bottom = bounds.getBottom() - 10.0f;

    const bool enabled = slider.isEnabled();
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = enabled ? (active ? 1.0f : 0.72f) : 0.25f;

    // Ticks either side of the slot.
    g.setColour (juce::Colours::white.withAlpha (0.16f * intensity));
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = top + (bottom - top) * (float) i / 10.0f;
        const float tickLength = (i % 5 == 0) ? 6.0f : 3.0f;
        g.drawLine (centreX - 11.0f, ty, centreX - 11.0f + tickLength, ty, 0.8f);
        g.drawLine (centreX + 11.0f, ty, centreX + 11.0f - tickLength, ty, 0.8f);
    }

    // Slot.
    juce::Path slot;
    slot.addRoundedRectangle (centreX - 2.2f, top, 4.4f, bottom - top, 2.2f);
    g.setColour (juce::Colour (0xff050607));
    g.fillPath (slot);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (centreX - 2.2f, top, centreX - 2.2f, bottom, 0.8f);

    // Lit travel.
    const float clampedPos = juce::jlimit (top, bottom, sliderPos);
    juce::Path lit;
    lit.addRoundedRectangle (centreX - 1.4f, clampedPos, 2.8f, bottom - clampedPos, 1.4f);
    g.setGradientFill (juce::ColourGradient (colour::amberHot.withAlpha (0.9f * intensity),
                                             centreX, clampedPos,
                                             colour::amberDim.withAlpha (0.85f * intensity),
                                             centreX, bottom, false));
    g.fillPath (lit);

    if (enabled)
        drawGlow (g, { centreX, clampedPos }, 16.0f, colour::amber, active ? 0.55f : 0.22f);

    // Cap.
    const float capW = 31.0f;
    const float capH = 14.0f;
    const auto cap = juce::Rectangle<float> (capW, capH)
                         .withCentre ({ centreX, clampedPos });

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.0f), 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5b626c),
                                             cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xff20242a),
                                             cap.getCentreX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f),
                                             cap.getCentreX(), cap.getY() + 1.0f,
                                             juce::Colours::transparentWhite,
                                             cap.getCentreX(), cap.getY() + capH * 0.55f, false));
    g.fillRoundedRectangle (cap.reduced (1.0f), 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle (cap.reduced (0.5f), 2.5f, 1.0f);

    g.setColour (colour::amber.withAlpha (enabled ? (active ? 0.95f : 0.65f) : 0.20f));
    g.drawLine (cap.getX() + 4.0f, cap.getCentreY(), cap.getRight() - 4.0f, cap.getCentreY(), 1.2f);

    // Fine grip lines either side of the centre line.
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (float offset : { -5.0f, 5.0f })
        g.drawLine (cap.getCentreX() + offset, cap.getY() + 2.0f,
                    cap.getCentreX() + offset, cap.getBottom() - 2.0f, 0.7f);

    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawLine (cap.getX() + 3.0f, cap.getY() + 2.0f, cap.getRight() - 3.0f, cap.getY() + 2.0f, 0.8f);
}

//==============================================================================
void Cs80LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (1.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const auto centre = knob.getCentre();
    const float radius = size * 0.5f;
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = slider.isEnabled() ? (active ? 1.0f : 0.75f) : 0.25f;

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Detent ticks around the outside of the arc.
    g.setColour (juce::Colours::white.withAlpha (0.14f * intensity));
    for (int i = 0; i <= 10; ++i)
    {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 10.0f;
        const float tickStart = radius - 0.5f;
        const float tickEnd = radius + 2.6f;
        g.drawLine (centre.x + std::sin (tickAngle) * tickStart,
                    centre.y - std::cos (tickAngle) * tickStart,
                    centre.x + std::sin (tickAngle) * tickEnd,
                    centre.y - std::cos (tickAngle) * tickEnd, 0.8f);
    }

    // Arc track.
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - 1.5f, radius - 1.5f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff0a0c0e));
        g.strokePath (track, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Value arc.
    if (std::abs (angle - rotaryStartAngle) > 0.01f)
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 1.5f, radius - 1.5f, 0.0f,
                           rotaryStartAngle, angle, true);

        g.setColour (colour::amber.withAlpha (0.30f * intensity));
        g.strokePath (arc, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        g.setColour (colour::amber.withAlpha (0.98f * intensity));
        g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    // Knob body.
    const float bodyRadius = radius - 4.5f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (body.translated (0.0f, 1.8f));

    juce::ColourGradient metal (juce::Colour (0xff5c636d), centre.x, body.getY(),
                                juce::Colour (0xff171a1f), centre.x, body.getBottom(), false);
    metal.addColour (0.35, juce::Colour (0xff3a4048));
    metal.addColour (0.75, juce::Colour (0xff23272d));
    g.setGradientFill (metal);
    g.fillEllipse (body);

    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (body.reduced (2.2f), 0.8f);

    // Indicator.
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.0f, -bodyRadius + 2.0f, 2.0f, bodyRadius * 0.52f, 1.0f);
    g.setColour (colour::amber.withAlpha (0.95f * intensity));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));

    if (active)
        drawGlow (g, centre, radius * 1.15f, colour::amber, 0.18f);
}

//==============================================================================
void Cs80LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1e23),
                                             bounds.getCentreX(), 0.0f,
                                             juce::Colour (0xff080a0c),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (4.0f, 0.8f, (float) width - 4.0f, 0.8f, 0.8f);

    // Machined arrow.
    const float ax = (float) width - 10.0f;
    const float ay = (float) height * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);
    g.setColour (colour::amber.withAlpha (box.isMouseOver() ? 0.95f : 0.6f));
    g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void Cs80LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (5, 1, box.getWidth() - 18, box.getHeight() - 2);
    label.setFont (monoFont (10.0f, true));
    label.setJustificationType (juce::Justification::centredLeft);
}

//==============================================================================
void Cs80LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setColour (juce::Colour (0xff0e1115));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (colour::amber.withAlpha (0.22f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}

void Cs80LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isChecked, bool isRadio,
                                         const juce::String& text,
                                         const juce::String& shortcutKeyText,
                                         const juce::Drawable* icon,
                                         const juce::Colour* textColour)
{
    juce::ignoreUnused (icon);

    if (isSeparator)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine ((float) area.getX() + 8.0f, (float) area.getCentreY(),
                    (float) area.getRight() - 8.0f, (float) area.getCentreY(), 1.0f);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (colour::amber.withAlpha (0.14f));
        g.fillRoundedRectangle (area.reduced (3, 1).toFloat(), 2.5f);
        g.setColour (colour::amber);
        g.fillRect (area.getX() + 3, area.getY() + 4, 2, area.getHeight() - 8);
    }

    g.setColour (isActive ? (isHighlighted ? colour::amberHot : colour::text)
                          : colour::textFaint);
    g.setFont (monoFont (11.0f, isHighlighted));
    g.drawText (text, area.withTrimmedLeft (14), juce::Justification::centredLeft, true);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (colour::textFaint);
        g.setFont (monoFont (9.0f));
        g.drawText (shortcutKeyText, area.withTrimmedRight (8), juce::Justification::centredRight, true);
    }

    if ((isChecked || isRadio) && isActive)
    {
        g.setColour (colour::amber);
        auto areaCopy = area;
        const auto tick = areaCopy.removeFromRight (18).toFloat();
        g.fillEllipse (tick.withSizeKeepingCentre (4.0f, 4.0f));
    }
}

juce::Font Cs80LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return monoFont (10.0f, true);
}

juce::Font Cs80LookAndFeel::getPopupMenuFont()
{
    return monoFont (11.0f);
}
} // namespace cs80::ui
