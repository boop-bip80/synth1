#include "LookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace cs80::ui
{
namespace
{
juce::Colour accentFor (const juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    return accent.isTransparent() ? colour::amber : accent;
}
} // namespace

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

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0b1117));
    setColour (juce::PopupMenu::textColourId, colour::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::amber.withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId, colour::amberHot);
    setColour (juce::PopupMenu::headerTextColourId, colour::textDim);

    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff080d12));
    setColour (juce::TooltipWindow::textColourId, colour::amberHot);
    setColour (juce::TooltipWindow::outlineColourId, colour::amber.withAlpha (0.32f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff060a0e));
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
    const float top = bounds.getY() + 14.0f;
    const float bottom = bounds.getBottom() - 14.0f;

    const bool enabled = slider.isEnabled();
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = enabled ? (active ? 1.0f : 0.82f) : 0.28f;
    const auto accent = accentFor (slider);

    // A softly lit scale ladder. Long ticks at the ends and the midpoint make
    // the fader travel readable at a glance, as on the original panel.
    for (int i = 0; i <= 12; ++i)
    {
        const float ty = top + (bottom - top) * (float) i / 12.0f;
        const bool major = (i % 3) == 0;
        const float tickLength = major ? 8.5f : 4.0f;

        g.setColour ((major ? colour::textDim : colour::textFaint).withAlpha (0.34f * intensity));
        g.drawLine (centreX - 13.0f, ty, centreX - 13.0f + tickLength, ty, 0.9f);
        g.drawLine (centreX + 13.0f, ty, centreX + 13.0f - tickLength, ty, 0.9f);
    }

    // Recessed slot with a bright lower lip.
    juce::Path slot;
    slot.addRoundedRectangle (centreX - 2.9f, top, 5.8f, bottom - top, 2.9f);
    g.setColour (juce::Colour (0xff010203));
    g.fillPath (slot);

    g.setColour (juce::Colours::black.withAlpha (0.95f));
    g.drawRoundedRectangle (centreX - 3.4f, top - 0.6f, 6.8f, bottom - top + 1.2f, 3.4f, 0.9f);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawLine (centreX - 2.9f, top, centreX - 2.9f, bottom, 0.7f);

    // Lit travel in the section's own colour.
    const float clampedPos = juce::jlimit (top, bottom, sliderPos);
    juce::Path lit;
    lit.addRoundedRectangle (centreX - 1.8f, clampedPos, 3.6f, bottom - clampedPos, 1.8f);
    g.setGradientFill (juce::ColourGradient (accent.brighter (0.32f).withAlpha (0.98f * intensity),
                                             centreX, clampedPos,
                                             accent.darker (0.45f).withAlpha (0.92f * intensity),
                                             centreX, bottom, false));
    g.fillPath (lit);

    if (enabled)
        drawGlow (g, { centreX, clampedPos }, 21.0f, accent, active ? 0.65f : 0.28f);

    // Ivory cap with a coloured inlay, like the CS-80's slider caps.
    const float capW = std::min (34.0f, bounds.getWidth() - 2.0f);
    const float capH = 17.0f;
    const auto cap = juce::Rectangle<float> (capW, capH).withCentre ({ centreX, clampedPos });

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.6f), 2.6f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffdf5e6),
                                             cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xffc9b99f),
                                             cap.getCentreX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.6f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.75f),
                                             cap.getCentreX(), cap.getY(),
                                             juce::Colours::transparentWhite,
                                             cap.getCentreX(), cap.getY() + capH * 0.55f, false));
    g.fillRoundedRectangle (cap.reduced (0.9f), 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.drawRoundedRectangle (cap.reduced (0.5f), 2.6f, 1.0f);

    // The coloured band is the section identity, and doubles as the value line.
    g.setColour (juce::Colour (0xff1a1611));
    g.fillRoundedRectangle (cap.getCentreX() - capW * 0.5f + 4.0f,
                            cap.getCentreY() - 1.2f,
                            capW - 8.0f, 2.4f, 1.2f);

    g.setColour (accent.withAlpha (enabled ? (active ? 1.0f : 0.86f) : 0.26f));
    g.fillRoundedRectangle (cap.getCentreX() - 5.0f, cap.getCentreY() - 0.9f,
                            10.0f, 1.8f, 0.9f);

    g.setColour (juce::Colours::white.withAlpha (0.62f));
    g.drawLine (cap.getX() + 3.0f, cap.getY() + 1.7f, cap.getRight() - 3.0f, cap.getY() + 1.7f, 0.8f);

    g.setColour (juce::Colours::black.withAlpha (0.12f));
    for (float offset : { -9.0f, -5.5f, 5.5f, 9.0f })
        g.drawLine (cap.getCentreX() + offset, cap.getY() + 3.4f,
                    cap.getCentreX() + offset, cap.getBottom() - 3.4f, 0.6f);
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
    const float intensity = slider.isEnabled() ? (active ? 1.0f : 0.84f) : 0.28f;
    const auto accent = accentFor (slider);

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Machined well.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff040709),
                                             centre.x, centre.y - radius,
                                             juce::Colour (0xff1a232b),
                                             centre.x, centre.y + radius, false));
    g.fillEllipse (knob.expanded (3.5f));

    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.drawEllipse (knob.expanded (3.5f), 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.drawEllipse (knob.expanded (2.4f), 0.8f);

    // Scale ticks around the body.
    for (int i = 0; i <= 14; ++i)
    {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 14.0f;
        const float tickStart = radius + 1.2f;
        const float tickEnd = radius + ((i % 7 == 0) ? 5.0f : 2.8f);

        g.setColour ((i % 7 == 0 ? colour::textDim : colour::textFaint).withAlpha (0.38f * intensity));
        g.drawLine (centre.x + std::sin (tickAngle) * tickStart,
                    centre.y - std::cos (tickAngle) * tickStart,
                    centre.x + std::sin (tickAngle) * tickEnd,
                    centre.y - std::cos (tickAngle) * tickEnd, 0.9f);
    }

    // Unlit arc track.
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - 0.4f, radius - 0.4f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff020405));
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (accent.withAlpha (0.14f * intensity));
        g.strokePath (track, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Value arc with a real glow at the tip.
    if (std::abs (angle - rotaryStartAngle) > 0.008f)
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 0.4f, radius - 0.4f, 0.0f,
                           rotaryStartAngle, angle, true);

        g.setColour (accent.withAlpha (0.22f * intensity));
        g.strokePath (arc, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        g.setColour (accent.withAlpha (0.98f * intensity));
        g.strokePath (arc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        const float beadX = centre.x + std::sin (angle) * (radius - 0.4f);
        const float beadY = centre.y - std::cos (angle) * (radius - 0.4f);
        drawGlow (g, { beadX, beadY }, 9.0f, accent.brighter (0.2f), 0.6f * intensity);
        g.setColour (accent.brighter (0.7f).withAlpha (0.95f * intensity));
        g.fillEllipse (beadX - 1.6f, beadY - 1.6f, 3.2f, 3.2f);
    }

    // Brushed outer collar with fine knurling.
    const float bodyRadius = radius - 5.5f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.fillEllipse (body.translated (0.0f, 2.4f));

    juce::ColourGradient collar (juce::Colour (0xff9aa3ab), centre.x, body.getY(),
                                 juce::Colour (0xff1d2329), centre.x, body.getBottom(), false);
    collar.addColour (0.28, juce::Colour (0xff6c757e));
    collar.addColour (0.64, juce::Colour (0xff303942));
    g.setGradientFill (collar);
    g.fillEllipse (body);

    g.setColour (juce::Colours::black.withAlpha (0.86f));
    g.drawEllipse (body.reduced (0.4f), 1.0f);

    for (int i = 0; i < 54; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 54.0f;
        const float inner = bodyRadius * 0.86f;
        const float outer = bodyRadius * 0.98f;
        g.setColour (juce::Colours::black.withAlpha (i % 2 == 0 ? 0.20f : 0.10f));
        g.drawLine (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner,
                    centre.x + std::sin (a) * outer, centre.y - std::cos (a) * outer, 0.6f);
    }

    // Black bakelite insert.
    const auto insert = juce::Rectangle<float> (bodyRadius * 1.58f, bodyRadius * 1.58f).withCentre (centre);

    juce::ColourGradient bakelite (juce::Colour (0xff2c343b), centre.x, insert.getY(),
                                   juce::Colour (0xff050709), centre.x, insert.getBottom(), false);
    bakelite.addColour (0.46, juce::Colour (0xff141a1f));
    g.setGradientFill (bakelite);
    g.fillEllipse (insert);

    g.setColour (juce::Colours::white.withAlpha (0.075f));
    g.drawEllipse (insert.reduced (1.4f), 0.9f);

    // Brushed centre cap.
    const auto cap = insert.withSizeKeepingCentre (insert.getWidth() * 0.50f,
                                                   insert.getHeight() * 0.50f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffaab2b9), cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xff262d34), cap.getCentreX(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.drawEllipse (cap, 0.8f);

    // Cream pointer, with a subtle dark seat so it reads on any accent.
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.3f, -bodyRadius + 1.2f, 2.6f, bodyRadius * 0.60f, 1.3f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.translated (0.0f, 1.0f)));
    g.setColour (colour::cream.withAlpha (0.98f * intensity));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));

    if (active)
        drawGlow (g, centre, radius * 1.3f, accent, 0.26f);
}

//==============================================================================
void Cs80LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff121b23),
                                             bounds.getCentreX(), 0.0f,
                                             juce::Colour (0xff04070a),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (colour::brass.withAlpha (0.25f));
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

    g.setColour (juce::Colour (0xff0a1016));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colour (0xff3a4853).withAlpha (0.9f));
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
    return monoFont (9.5f, true);
}

juce::Font Cs80LookAndFeel::getPopupMenuFont()
{
    return monoFont (11.0f);
}
} // namespace cs80::ui
