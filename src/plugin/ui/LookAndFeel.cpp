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

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff070b0f));
    setColour (juce::ComboBox::textColourId, colour::amberHot);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::arrowColourId, colour::amberDim);
    setColour (juce::ComboBox::focusedOutlineColourId, colour::amber.withAlpha (0.5f));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0a1117));
    setColour (juce::PopupMenu::textColourId, colour::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::amber.withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId, colour::amberHot);
    setColour (juce::PopupMenu::headerTextColourId, colour::textDim);

    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff070c11));
    setColour (juce::TooltipWindow::textColourId, colour::amberHot);
    setColour (juce::TooltipWindow::outlineColourId, colour::amber.withAlpha (0.35f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff05090d));
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
    const float top = bounds.getY() + 9.0f;
    const float bottom = bounds.getBottom() - 9.0f;
    const float travel = std::max (1.0f, bottom - top);

    const bool enabled = slider.isEnabled();
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = enabled ? (active ? 1.0f : 0.9f) : 0.28f;
    const auto accent = accentFor (slider);
    const float clampedPos = juce::jlimit (top, bottom, sliderPos);

    // Two machined rails flank the slot, so the fader reads as a real
    // component sunk into the plate rather than a painted line.
    for (int side : { -1, 1 })
    {
        const float railX = centreX + (float) side * 15.0f;
        const juce::Rectangle<float> rail (railX - 1.35f, top - 3.0f, 2.7f, travel + 6.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5c5142), rail.getX(), 0.0f,
                                                 juce::Colour (0xff0a0806), rail.getRight(), 0.0f, false));
        g.fillRoundedRectangle (rail, 1.3f);
        g.setColour (juce::Colours::black.withAlpha (0.82f));
        g.drawRoundedRectangle (rail, 1.3f, 0.6f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine (rail.getX() + 0.9f, rail.getY() + 1.0f,
                    rail.getX() + 0.9f, rail.getBottom() - 1.0f, 0.5f);
    }

    // Etched scale ladder. Major ticks are warm ivory; the small ones are
    // hairline so the eye reads the twelve-step travel without clutter.
    for (int i = 0; i <= 12; ++i)
    {
        const float ty = top + travel * (float) i / 12.0f;
        const bool major = (i % 3) == 0;
        const float tickLength = major ? 6.2f : 3.4f;

        g.setColour ((major ? colour::cream : colour::textFaint)
                         .withAlpha ((major ? 0.50f : 0.26f) * intensity));
        g.drawLine (centreX - 11.2f, ty, centreX - 11.2f + tickLength, ty, major ? 0.9f : 0.6f);
        g.drawLine (centreX + 11.2f, ty, centreX + 11.2f - tickLength, ty, major ? 0.9f : 0.6f);
    }

    // Deep, black slot with a lit inner edge.
    juce::Path slot;
    slot.addRoundedRectangle (centreX - 3.4f, top, 6.8f, travel, 3.4f);
    g.setColour (juce::Colour (0xff000000));
    g.fillPath (slot);

    g.setColour (juce::Colours::black.withAlpha (0.98f));
    g.drawRoundedRectangle (centreX - 4.3f, top - 1.0f, 8.6f, travel + 2.0f, 4.3f, 1.1f);

    g.setColour (juce::Colours::white.withAlpha (0.085f));
    g.drawLine (centreX - 3.3f, top + 1.2f, centreX - 3.3f, bottom - 1.2f, 0.7f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (centreX + 3.3f, top, centreX + 3.3f, bottom, 0.45f);

    // The live travel burns in the section colour.
    if (clampedPos < bottom - 0.5f)
    {
        juce::Path lit;
        lit.addRoundedRectangle (centreX - 2.1f, clampedPos + 2.0f, 4.2f, bottom - clampedPos, 2.1f);
        g.setGradientFill (juce::ColourGradient (accent.brighter (0.36f).withAlpha (0.96f * intensity),
                                                 centreX, clampedPos,
                                                 accent.darker (0.48f).withAlpha (0.88f * intensity),
                                                 centreX, bottom, false));
        g.fillPath (lit);
    }

    if (enabled)
        drawGlow (g, { centreX, clampedPos }, 27.0f, accent, active ? 0.68f : 0.26f);

    // Ivory CS-80 slider cap, moulded in layers.
    const float capW = std::min (40.0f, bounds.getWidth() - 2.0f);
    const float capH = 24.0f;
    const auto cap = juce::Rectangle<float> (capW, capH).withCentre ({ centreX, clampedPos });

    g.setColour (juce::Colours::black.withAlpha (0.80f));
    g.fillRoundedRectangle (cap.expanded (1.3f).translated (0.0f, 3.2f), 4.2f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b5a44), cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xff18120c), cap.getCentreX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap.expanded (1.3f), 4.0f);

    juce::ColourGradient ivory (juce::Colour (0xfffffbf0),
                                cap.getCentreX(), cap.getY(),
                                juce::Colour (0xffc3ad8b),
                                cap.getCentreX(), cap.getBottom(), false);
    ivory.addColour (0.24, juce::Colour (0xfffffdf7));
    ivory.addColour (0.58, juce::Colour (0xfff2e4ca));
    ivory.addColour (0.84, juce::Colour (0xffd4be9c));
    g.setGradientFill (ivory);
    g.fillRoundedRectangle (cap, 3.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.80f),
                                             cap.getCentreX(), cap.getY() + 0.5f,
                                             juce::Colours::transparentWhite,
                                             cap.getCentreX(), cap.getY() + capH * 0.52f, false));
    g.fillRoundedRectangle (cap.reduced (1.0f), 2.8f);

    g.setColour (juce::Colours::black.withAlpha (0.64f));
    g.drawRoundedRectangle (cap.reduced (0.4f), 3.5f, 0.9f);

    g.setColour (juce::Colours::white.withAlpha (0.76f));
    g.drawLine (cap.getX() + 4.5f, cap.getY() + 1.8f, cap.getRight() - 4.5f, cap.getY() + 1.8f, 0.9f);

    // Coloured, back-lit value window. This is the strongest colour on a
    // fader, so it has to glow from across the room.
    const auto window = juce::Rectangle<float> (capW - 9.0f, 10.8f)
                            .withCentre ({ cap.getCentreX(), cap.getCentreY() + 0.9f });

    g.setColour (juce::Colour (0xff0b0704));
    g.fillRoundedRectangle (window.expanded (1.3f), 2.6f);

    g.setGradientFill (juce::ColourGradient (accent.brighter (0.62f).withAlpha (enabled ? 1.0f : 0.30f),
                                             window.getCentreX(), window.getY(),
                                             accent.darker (0.18f).withAlpha (enabled ? 1.0f : 0.26f),
                                             window.getCentreX(), window.getBottom(), false));
    g.fillRoundedRectangle (window, 1.8f);

    if (enabled)
    {
        drawGlow (g, window.getCentre(), 16.0f, accent, active ? 0.72f : 0.40f);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.34f),
                                                 window.getCentreX(), window.getY(),
                                                 juce::Colours::transparentWhite,
                                                 window.getCentreX(), window.getBottom(), false));
        g.fillRoundedRectangle (window.reduced (1.0f), 1.2f);

        g.setColour (juce::Colours::black.withAlpha (0.34f));
        g.drawRoundedRectangle (window, 1.8f, 0.6f);
    }

    // Fine grooves and a warm underlight on the cap's lower lip.
    g.setColour (juce::Colours::black.withAlpha (0.17f));
    for (float offset : { -12.0f, -9.0f, 9.0f, 12.0f })
        g.drawLine (cap.getCentreX() + offset, cap.getY() + 4.2f,
                    cap.getCentreX() + offset, cap.getBottom() - 4.2f, 0.55f);

    g.setColour (accent.withAlpha (active ? 0.82f : 0.30f));
    g.drawRoundedRectangle (cap.reduced (0.2f), 3.5f, 0.9f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                             cap.getCentreX(), cap.getBottom() - 3.0f,
                                             accent.withAlpha (0.30f * intensity),
                                             cap.getCentreX(), cap.getBottom() + 1.0f, false));
    g.fillRoundedRectangle (cap.expanded (0.9f), 4.0f);
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
    const float intensity = slider.isEnabled() ? (active ? 1.0f : 0.9f) : 0.28f;
    const auto accent = accentFor (slider);

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Contact shadow and recessed washer.
    g.setColour (juce::Colours::black.withAlpha (0.66f));
    g.fillEllipse (knob.expanded (4.6f).translated (0.0f, 2.2f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff020202),
                                             centre.x, centre.y - radius,
                                             juce::Colour (0xff241c14),
                                             centre.x, centre.y + radius, false));
    g.fillEllipse (knob.expanded (4.4f));

    g.setColour (juce::Colours::black.withAlpha (0.94f));
    g.drawEllipse (knob.expanded (4.4f), 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.085f));
    g.drawEllipse (knob.expanded (3.0f), 0.8f);

    // Outer tick ring.
    for (int i = 0; i <= 20; ++i)
    {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle)
                                                    * (float) i / 20.0f;
        const bool major = (i % 5) == 0;
        const float tickStart = radius + 1.2f;
        const float tickEnd = radius + (major ? 5.4f : 2.9f);

        g.setColour ((major ? colour::cream : colour::textFaint)
                         .withAlpha ((major ? 0.52f : 0.25f) * intensity));
        g.drawLine (centre.x + std::sin (tickAngle) * tickStart,
                    centre.y - std::cos (tickAngle) * tickStart,
                    centre.x + std::sin (tickAngle) * tickEnd,
                    centre.y - std::cos (tickAngle) * tickEnd, major ? 0.95f : 0.65f);
    }

    // Unlit arc track.
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - 0.1f, radius - 0.1f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff000000));
        g.strokePath (track, juce::PathStrokeType (5.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (accent.withAlpha (0.15f * intensity));
        g.strokePath (track, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Value arc, with a hot bead at the tip.
    if (std::abs (angle - rotaryStartAngle) > 0.008f)
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 0.1f, radius - 0.1f, 0.0f,
                           rotaryStartAngle, angle, true);

        g.setColour (accent.withAlpha (0.26f * intensity));
        g.strokePath (arc, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        g.setColour (accent.withAlpha (0.99f * intensity));
        g.strokePath (arc, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        const float beadX = centre.x + std::sin (angle) * (radius - 0.1f);
        const float beadY = centre.y - std::cos (angle) * (radius - 0.1f);
        drawGlow (g, { beadX, beadY }, 12.0f, accent.brighter (0.30f), 0.74f * intensity);
        g.setColour (accent.brighter (0.84f).withAlpha (0.98f * intensity));
        g.fillEllipse (beadX - 1.9f, beadY - 1.9f, 3.8f, 3.8f);
    }

    // Brushed metal outer collar.
    const float bodyRadius = radius - 6.4f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    g.setColour (juce::Colours::black.withAlpha (0.76f));
    g.fillEllipse (body.translated (0.0f, 2.4f));

    juce::ColourGradient collar (juce::Colour (0xffd7d2c6), centre.x, body.getY(),
                                 juce::Colour (0xff15110d), centre.x, body.getBottom(), false);
    collar.addColour (0.18, juce::Colour (0xff9c9484));
    collar.addColour (0.42, juce::Colour (0xff5b5548));
    collar.addColour (0.70, juce::Colour (0xff2b261f));
    collar.addColour (0.90, juce::Colour (0xff19140f));
    g.setGradientFill (collar);
    g.fillEllipse (body);

    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.drawEllipse (body.reduced (0.4f), 1.0f);

    for (int i = 0; i < 84; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 84.0f;
        const float inner = bodyRadius * 0.84f;
        const float outer = bodyRadius * 0.995f;
        g.setColour (juce::Colours::black.withAlpha (i % 2 == 0 ? 0.26f : 0.10f));
        g.drawLine (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner,
                    centre.x + std::sin (a) * outer, centre.y - std::cos (a) * outer, 0.5f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawEllipse (body.reduced (bodyRadius * 0.13f), 0.6f);

    // Black bakelite insert with concentric turning marks.
    const auto insert = juce::Rectangle<float> (bodyRadius * 1.52f, bodyRadius * 1.52f).withCentre (centre);

    juce::ColourGradient bakelite (juce::Colour (0xff3b342b), centre.x, insert.getY(),
                                   juce::Colour (0xff020202), centre.x, insert.getBottom(), false);
    bakelite.addColour (0.40, juce::Colour (0xff17130f));
    g.setGradientFill (bakelite);
    g.fillEllipse (insert);

    g.setColour (juce::Colours::white.withAlpha (0.11f));
    g.drawEllipse (insert.reduced (1.1f), 0.8f);

    for (float ring : { 0.86f, 0.70f, 0.56f })
    {
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.drawEllipse (insert.withSizeKeepingCentre (insert.getWidth() * ring,
                                                     insert.getHeight() * ring), 0.5f);
    }

    // Brass centre cap.
    const auto cap = insert.withSizeKeepingCentre (insert.getWidth() * 0.42f,
                                                   insert.getHeight() * 0.42f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0d29a), cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xff6b4c22), cap.getCentreX(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.74f));
    g.drawEllipse (cap, 0.8f);
    g.setColour (juce::Colours::white.withAlpha (0.30f));
    g.drawEllipse (cap.reduced (1.4f), 0.5f);

    // Cream pointer with a black seat and a tiny accent inlay.
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.65f, -bodyRadius + 0.8f, 3.3f, bodyRadius * 0.74f, 1.65f);
    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.translated (0.0f, 1.2f)));
    g.setColour (colour::cream.withAlpha (0.99f * intensity));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));

    const float tipX = centre.x + std::sin (angle) * (bodyRadius * 0.58f);
    const float tipY = centre.y - std::cos (angle) * (bodyRadius * 0.58f);
    g.setColour (accent.withAlpha (0.94f * intensity));
    g.fillEllipse (tipX - 1.4f, tipY - 1.4f, 2.8f, 2.8f);

    drawGlow (g, centre, radius * 1.42f, accent, active ? 0.42f : 0.13f);
}

//==============================================================================
//==============================================================================
void Cs80LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff16222b),
                                             bounds.getCentreX(), 0.0f,
                                             juce::Colour (0xff03060a),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.94f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (colour::brass.withAlpha (0.32f));
    g.drawLine (4.0f, 0.8f, (float) width - 4.0f, 0.8f, 0.8f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (4.0f, bounds.getBottom() - 1.0f, (float) width - 4.0f, bounds.getBottom() - 1.0f, 0.7f);

    const float ax = (float) width - 10.0f;
    const float ay = (float) height * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.0f, ay - 2.0f);

    g.setColour (colour::amber.withAlpha (box.isMouseOver() ? 0.98f : 0.64f));
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

    g.setColour (juce::Colour (0xff080e14));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colour (0xff41525f).withAlpha (0.92f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    g.setColour (colour::brass.withAlpha (0.22f));
    g.drawLine (5.0f, 1.0f, (float) width - 5.0f, 1.0f, 0.8f);
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
