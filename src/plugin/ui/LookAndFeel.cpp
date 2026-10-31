#include "LookAndFeel.h"

#include <cmath>

namespace cs80::ui
{
Cs80LookAndFeel::Cs80LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colour::windowBg);

    setColour (juce::Label::textColourId, colour::text);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff080b0e));
    setColour (juce::ComboBox::textColourId, colour::amberHot);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::arrowColourId, colour::amberDim);
    setColour (juce::ComboBox::focusedOutlineColourId, colour::amber.withAlpha (0.5f));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d1217));
    setColour (juce::PopupMenu::textColourId, colour::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::amber.withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId, colour::amberHot);
    setColour (juce::PopupMenu::headerTextColourId, colour::textDim);

    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff090c10));
    setColour (juce::TooltipWindow::textColourId, colour::amberHot);
    setColour (juce::TooltipWindow::outlineColourId, colour::amber.withAlpha (0.30f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff070a0d));
    setColour (juce::TextEditor::textColourId, colour::text);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, colour::amber.withAlpha (0.45f));
    setColour (juce::CaretComponent::caretColourId, colour::amber);

    setColour (juce::ScrollBar::thumbColourId, colour::line.brighter (0.25f));
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
    const float top = bounds.getY() + 13.0f;
    const float bottom = bounds.getBottom() - 13.0f;

    const bool enabled = slider.isEnabled();
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = enabled ? (active ? 1.0f : 0.78f) : 0.25f;

    // Recessed scale ticks either side of the slot.
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = top + (bottom - top) * (float) i / 10.0f;
        const float tickLength = (i % 5 == 0) ? 7.0f : 3.5f;

        g.setColour ((i % 5 == 0 ? colour::textDim : colour::textFaint).withAlpha (0.34f * intensity));
        g.drawLine (centreX - 11.0f, ty, centreX - 11.0f + tickLength, ty, 0.9f);
        g.drawLine (centreX + 11.0f, ty, centreX + 11.0f - tickLength, ty, 0.9f);
    }

    // Black-anodised slot with a bright machined lip.
    juce::Path slot;
    slot.addRoundedRectangle (centreX - 2.6f, top, 5.2f, bottom - top, 2.6f);
    g.setColour (juce::Colour (0xff020304));
    g.fillPath (slot);

    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (centreX - 3.1f, top - 0.5f, 6.2f, bottom - top + 1.0f, 3.0f, 0.8f);

    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawLine (centreX - 2.6f, top, centreX - 2.6f, bottom, 0.7f);

    // Lit travel.
    const float clampedPos = juce::jlimit (top, bottom, sliderPos);
    juce::Path lit;
    lit.addRoundedRectangle (centreX - 1.6f, clampedPos, 3.2f, bottom - clampedPos, 1.6f);
    g.setGradientFill (juce::ColourGradient (colour::amberHot.withAlpha (0.95f * intensity),
                                             centreX, clampedPos,
                                             colour::amberDim.withAlpha (0.90f * intensity),
                                             centreX, bottom, false));
    g.fillPath (lit);

    if (enabled)
        drawGlow (g, { centreX, clampedPos }, 19.0f, colour::amber, active ? 0.60f : 0.26f);

    // Ivory fader cap, like the CS-80's white sliders.
    const float capW = 34.0f;
    const float capH = 16.0f;
    const auto cap = juce::Rectangle<float> (capW, capH)
                         .withCentre ({ centreX, clampedPos });

    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.4f), 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffaf1df),
                                             cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xffcbbda5),
                                             cap.getCentreX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.5f);

    // Top bevel and underside shadow.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.65f),
                                             cap.getCentreX(), cap.getY(),
                                             juce::Colours::transparentWhite,
                                             cap.getCentreX(), cap.getY() + capH * 0.5f, false));
    g.fillRoundedRectangle (cap.reduced (0.8f), 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (cap.reduced (0.5f), 2.5f, 1.0f);

    // Black centre groove; the lit line makes the value legible at a glance.
    g.setColour (juce::Colour (0xff1a1712));
    g.fillRoundedRectangle (cap.getCentreX() - capW * 0.5f + 4.0f,
                            cap.getCentreY() - 1.0f,
                            capW - 8.0f, 2.0f, 1.0f);

    g.setColour (colour::amber.withAlpha (enabled ? (active ? 1.0f : 0.70f) : 0.18f));
    g.fillRect (cap.getCentreX() - 4.0f, cap.getCentreY() - 0.75f, 8.0f, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawLine (cap.getX() + 3.0f, cap.getY() + 1.6f, cap.getRight() - 3.0f, cap.getY() + 1.6f, 0.8f);

    // Fine grip lines.
    g.setColour (juce::Colours::black.withAlpha (0.13f));
    for (float offset : { -8.0f, -5.0f, 5.0f, 8.0f })
        g.drawLine (cap.getCentreX() + offset, cap.getY() + 3.2f,
                    cap.getCentreX() + offset, cap.getBottom() - 3.2f, 0.6f);
}

//==============================================================================
void Cs80LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                            .reduced (1.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const auto centre = knob.getCentre();
    const float radius = size * 0.5f;
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = slider.isEnabled() ? (active ? 1.0f : 0.80f) : 0.25f;

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Recessed dial well.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff070a0d),
                                             centre.x, centre.y - radius,
                                             juce::Colour (0xff1a2129),
                                             centre.x, centre.y + radius, false));
    g.fillEllipse (knob.expanded (3.0f));

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawEllipse (knob.expanded (3.0f), 1.0f);

    // Ticks around the outside of the arc.
    for (int i = 0; i <= 12; ++i)
    {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 12.0f;
        const float tickStart = radius + 1.0f;
        const float tickEnd = radius + (i % 3 == 0 ? 4.2f : 2.6f);

        g.setColour (colour::textDim.withAlpha (0.32f * intensity));
        g.drawLine (centre.x + std::sin (tickAngle) * tickStart,
                    centre.y - std::cos (tickAngle) * tickStart,
                    centre.x + std::sin (tickAngle) * tickEnd,
                    centre.y - std::cos (tickAngle) * tickEnd, 0.9f);
    }

    // Arc track.
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - 0.6f, radius - 0.6f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff030506));
        g.strokePath (track, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (colour::amber.withAlpha (0.13f * intensity));
        g.strokePath (track, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Zero marker at the top of the dial.
    g.setColour (colour::amber.withAlpha (0.34f * intensity));
    g.fillEllipse (centre.x - 1.2f, centre.y - radius - 3.5f, 2.4f, 2.4f);

    // Value arc.
    if (std::abs (angle - rotaryStartAngle) > 0.01f)
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 0.6f, radius - 0.6f, 0.0f,
                           rotaryStartAngle, angle, true);

        g.setColour (colour::amber.withAlpha (0.24f * intensity));
        g.strokePath (arc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        g.setColour (colour::amber.withAlpha (0.98f * intensity));
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // A bright bead at the end of the arc.
        const float beadX = centre.x + std::sin (angle) * (radius - 0.6f);
        const float beadY = centre.y - std::cos (angle) * (radius - 0.6f);
        drawGlow (g, { beadX, beadY }, 8.0f, colour::amberHot, 0.5f * intensity);
        g.setColour (colour::amberHot.withAlpha (0.9f * intensity));
        g.fillEllipse (beadX - 1.5f, beadY - 1.5f, 3.0f, 3.0f);
    }

    // Brushed metal skirt.
    const float bodyRadius = radius - 5.0f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (body.translated (0.0f, 2.2f));

    juce::ColourGradient skirt (juce::Colour (0xff818a94), centre.x, body.getY(),
                                juce::Colour (0xff20262c), centre.x, body.getBottom(), false);
    skirt.addColour (0.28, juce::Colour (0xff5a626c));
    skirt.addColour (0.62, juce::Colour (0xff2c333a));
    g.setGradientFill (skirt);
    g.fillEllipse (body);

    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawEllipse (body.reduced (0.4f), 1.0f);

    // Black bakelite insert.
    const auto insert = juce::Rectangle<float> (bodyRadius * 1.52f, bodyRadius * 1.52f).withCentre (centre);

    juce::ColourGradient bakelite (juce::Colour (0xff2b3035), centre.x, insert.getY(),
                                   juce::Colour (0xff07090b), centre.x, insert.getBottom(), false);
    bakelite.addColour (0.45, juce::Colour (0xff171b1f));
    g.setGradientFill (bakelite);
    g.fillEllipse (insert);

    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (insert.reduced (1.2f), 0.9f);

    // A tiny brushed centre cap.
    const auto cap = insert.withSizeKeepingCentre (insert.getWidth() * 0.52f,
                                                   insert.getHeight() * 0.52f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9aa3ad), cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xff2a3037), cap.getCentreX(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawEllipse (cap, 0.8f);

    // Cream indicator.
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.2f, -bodyRadius + 1.4f, 2.4f, bodyRadius * 0.58f, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.translated (0.0f, 0.8f)));
    g.setColour (colour::cream.withAlpha (0.95f * intensity));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));

    if (active)
        drawGlow (g, centre, radius * 1.25f, colour::amber, 0.22f);
}

//==============================================================================
void Cs80LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff10161c),
                                             bounds.getCentreX(), 0.0f,
                                             juce::Colour (0xff050709),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (4.0f, 0.8f, (float) width - 4.0f, 0.8f, 0.8f);

    const float ax = (float) width - 10.0f;
    const float ay = (float) height * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);

    g.setColour (colour::amber.withAlpha (box.isMouseOver() ? 0.98f : 0.62f));
    g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void Cs80LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 19, box.getHeight() - 2);
    label.setFont (monoFont (9.5f, true));
    label.setJustificationType (juce::Justification::centredLeft);
}

//==============================================================================
void Cs80LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setColour (juce::Colour (0xff0b1015));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colour (0xff37424c).withAlpha (0.8f));
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
        g.setColour (colour::amber.withAlpha (0.13f));
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
    return monoFont (9.5f, true);
}

juce::Font Cs80LookAndFeel::getPopupMenuFont()
{
    return monoFont (11.0f);
}
} // namespace cs80::ui
