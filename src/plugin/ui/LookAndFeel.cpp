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
    const float top = bounds.getY() + 10.0f;
    const float bottom = bounds.getBottom() - 10.0f;

    const bool enabled = slider.isEnabled();
    const bool active = slider.isMouseButtonDown() || slider.isMouseOverOrDragging();
    const float intensity = enabled ? (active ? 1.0f : 0.86f) : 0.28f;
    const auto accent = accentFor (slider);

    // Machined scale ladder. Long ticks mark the ends and the midpoint.
    for (int i = 0; i <= 12; ++i)
    {
        const float ty = top + (bottom - top) * (float) i / 12.0f;
        const bool major = (i % 3) == 0;
        const float tickLength = major ? 9.0f : 4.5f;

        g.setColour ((major ? colour::textDim : colour::textFaint).withAlpha (0.36f * intensity));
        g.drawLine (centreX - 14.0f, ty, centreX - 14.0f + tickLength, ty, 0.9f);
        g.drawLine (centreX + 14.0f, ty, centreX + 14.0f - tickLength, ty, 0.9f);

        if (major)
        {
            g.setColour (colour::textFaint.withAlpha (0.22f * intensity));
            g.fillEllipse (centreX - 20.5f, ty - 0.9f, 1.8f, 1.8f);
            g.fillEllipse (centreX + 19.0f, ty - 0.9f, 1.8f, 1.8f);
        }
    }

    // The slot itself: a deep well with a bright lower lip.
    juce::Path slot;
    slot.addRoundedRectangle (centreX - 3.6f, top, 7.2f, bottom - top, 3.6f);
    g.setColour (juce::Colour (0xff010203));
    g.fillPath (slot);

    g.setColour (juce::Colours::black.withAlpha (0.96f));
    g.drawRoundedRectangle (centreX - 4.2f, top - 0.8f, 8.4f, bottom - top + 1.6f, 4.2f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (centreX - 3.4f, top + 1.0f, centreX - 3.4f, bottom - 1.0f, 0.7f);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (centreX + 3.4f, top, centreX + 3.4f, bottom, 0.5f);

    // Lit travel in the section colour.
    const float clampedPos = juce::jlimit (top, bottom, sliderPos);
    juce::Path lit;
    lit.addRoundedRectangle (centreX - 2.2f, clampedPos, 4.4f, bottom - clampedPos, 2.2f);
    g.setGradientFill (juce::ColourGradient (accent.brighter (0.42f).withAlpha (0.99f * intensity),
                                             centreX, clampedPos,
                                             accent.darker (0.44f).withAlpha (0.94f * intensity),
                                             centreX, bottom, false));
    g.fillPath (lit);

    if (enabled)
        drawGlow (g, { centreX, clampedPos }, 26.0f, accent, active ? 0.72f : 0.30f);

    // Ivory cap with a coloured, back-lit window, like the CS-80's sliders.
    const float capW = std::min (40.0f, bounds.getWidth() - 1.0f);
    const float capH = 19.0f;
    const auto cap = juce::Rectangle<float> (capW, capH).withCentre ({ centreX, clampedPos });

    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillRoundedRectangle (cap.translated (0.0f, 3.0f), 3.0f);

    juce::ColourGradient ivory (juce::Colour (0xfffff8ea),
                                cap.getCentreX(), cap.getY(),
                                juce::Colour (0xffc7b496),
                                cap.getCentreX(), cap.getBottom(), false);
    ivory.addColour (0.34, juce::Colour (0xfffdf3e2));
    ivory.addColour (0.76, juce::Colour (0xffddc9a8));
    g.setGradientFill (ivory);
    g.fillRoundedRectangle (cap, 3.0f);

    // Chamfered top surface.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.82f),
                                             cap.getCentreX(), cap.getY(),
                                             juce::Colours::transparentWhite,
                                             cap.getCentreX(), cap.getY() + capH * 0.55f, false));
    g.fillRoundedRectangle (cap.reduced (0.9f), 2.3f);

    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.drawRoundedRectangle (cap.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.72f));
    g.drawLine (cap.getX() + 4.0f, cap.getY() + 1.8f, cap.getRight() - 4.0f, cap.getY() + 1.8f, 0.9f);

    // Coloured, back-lit value window. This is the one place the section
    // colour appears on a fader, so it needs to read from across the room.
    const auto window = juce::Rectangle<float> (capW - 11.0f, 7.0f)
                            .withCentre ({ cap.getCentreX(), cap.getCentreY() });

    g.setColour (juce::Colour (0xff120e0a));
    g.fillRoundedRectangle (window.expanded (0.8f), 2.2f);

    g.setGradientFill (juce::ColourGradient (accent.brighter (0.72f).withAlpha (enabled ? 1.0f : 0.30f),
                                             window.getCentreX(), window.getY(),
                                             accent.brighter (0.02f).withAlpha (enabled ? 1.0f : 0.28f),
                                             window.getCentreX(), window.getBottom(), false));
    g.fillRoundedRectangle (window.reduced (0.9f), 1.5f);

    if (enabled)
    {
        drawGlow (g, window.getCentre(), 14.0f, accent, active ? 0.70f : 0.34f);
        g.setColour (juce::Colours::white.withAlpha (active ? 0.68f : 0.46f));
        g.drawLine (window.getX() + 2.0f, window.getCentreY(),
                    window.getRight() - 2.0f, window.getCentreY(), 0.8f);
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawRoundedRectangle (window.reduced (0.4f), 1.4f, 0.6f);
    }

    // Side grooves make the cap read as a moulded part.
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    for (float offset : { -13.0f, -9.5f, 9.5f, 13.0f })
        g.drawLine (cap.getCentreX() + offset, cap.getY() + 4.0f,
                    cap.getCentreX() + offset, cap.getBottom() - 4.0f, 0.6f);

    g.setColour (accent.withAlpha (active ? 0.72f : 0.22f));
    g.drawRoundedRectangle (cap.reduced (0.4f), 3.0f, 0.8f);
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
    const float intensity = slider.isEnabled() ? (active ? 1.0f : 0.88f) : 0.28f;
    const auto accent = accentFor (slider);

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Machined well and a soft contact shadow.
    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.fillEllipse (knob.expanded (4.0f).translated (0.0f, 1.6f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff030607),
                                             centre.x, centre.y - radius,
                                             juce::Colour (0xff1a252d),
                                             centre.x, centre.y + radius, false));
    g.fillEllipse (knob.expanded (3.8f));

    g.setColour (juce::Colours::black.withAlpha (0.94f));
    g.drawEllipse (knob.expanded (3.8f), 1.2f);

    g.setColour (juce::Colours::white.withAlpha (0.075f));
    g.drawEllipse (knob.expanded (2.6f), 0.8f);

    // Scale ring: brighter major ticks, fine minor ticks.
    for (int i = 0; i <= 18; ++i)
    {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (float) i / 18.0f;
        const bool major = (i % 3) == 0;
        const float tickStart = radius + 1.0f;
        const float tickEnd = radius + (major ? 5.6f : 3.0f);

        g.setColour ((major ? colour::textDim : colour::textFaint).withAlpha ((major ? 0.48f : 0.26f) * intensity));
        g.drawLine (centre.x + std::sin (tickAngle) * tickStart,
                    centre.y - std::cos (tickAngle) * tickStart,
                    centre.x + std::sin (tickAngle) * tickEnd,
                    centre.y - std::cos (tickAngle) * tickEnd, major ? 1.0f : 0.7f);
    }

    // Unlit arc track.
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - 0.2f, radius - 0.2f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff010203));
        g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (accent.withAlpha (0.16f * intensity));
        g.strokePath (track, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Value arc with a real glow at the tip.
    if (std::abs (angle - rotaryStartAngle) > 0.008f)
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 0.2f, radius - 0.2f, 0.0f,
                           rotaryStartAngle, angle, true);

        g.setColour (accent.withAlpha (0.20f * intensity));
        g.strokePath (arc, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        g.setColour (accent.withAlpha (0.99f * intensity));
        g.strokePath (arc, juce::PathStrokeType (3.4f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        const float beadX = centre.x + std::sin (angle) * (radius - 0.2f);
        const float beadY = centre.y - std::cos (angle) * (radius - 0.2f);
        drawGlow (g, { beadX, beadY }, 11.0f, accent.brighter (0.24f), 0.70f * intensity);
        g.setColour (accent.brighter (0.78f).withAlpha (0.98f * intensity));
        g.fillEllipse (beadX - 1.8f, beadY - 1.8f, 3.6f, 3.6f);
    }

    // Brushed aluminium collar.
    const float bodyRadius = radius - 6.0f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillEllipse (body.translated (0.0f, 2.6f));

    juce::ColourGradient collar (juce::Colour (0xffb3bbc1), centre.x, body.getY(),
                                 juce::Colour (0xff171d22), centre.x, body.getBottom(), false);
    collar.addColour (0.22, juce::Colour (0xff828c94));
    collar.addColour (0.48, juce::Colour (0xff454f58));
    collar.addColour (0.76, juce::Colour (0xff242b31));
    g.setGradientFill (collar);
    g.fillEllipse (body);

    g.setColour (juce::Colours::black.withAlpha (0.88f));
    g.drawEllipse (body.reduced (0.4f), 1.0f);

    for (int i = 0; i < 72; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 72.0f;
        const float inner = bodyRadius * 0.82f;
        const float outer = bodyRadius * 0.99f;
        g.setColour (juce::Colours::black.withAlpha (i % 2 == 0 ? 0.24f : 0.11f));
        g.drawLine (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner,
                    centre.x + std::sin (a) * outer, centre.y - std::cos (a) * outer, 0.55f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (body.reduced (bodyRadius * 0.12f), 0.6f);

    // Black bakelite insert.
    const auto insert = juce::Rectangle<float> (bodyRadius * 1.55f, bodyRadius * 1.55f).withCentre (centre);

    juce::ColourGradient bakelite (juce::Colour (0xff303840), centre.x, insert.getY(),
                                   juce::Colour (0xff030506), centre.x, insert.getBottom(), false);
    bakelite.addColour (0.44, juce::Colour (0xff12181d));
    g.setGradientFill (bakelite);
    g.fillEllipse (insert);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (insert.reduced (1.2f), 0.9f);

    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.fillEllipse (insert.withSizeKeepingCentre (insert.getWidth() * 0.56f,
                                                 insert.getHeight() * 0.56f));

    // Brushed centre cap.
    const auto cap = insert.withSizeKeepingCentre (insert.getWidth() * 0.47f,
                                                   insert.getHeight() * 0.47f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc2c9ce), cap.getCentreX(), cap.getY(),
                                             juce::Colour (0xff2a3239), cap.getCentreX(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.drawEllipse (cap, 0.8f);

    g.setColour (juce::Colours::white.withAlpha (0.24f));
    g.drawEllipse (cap.reduced (1.2f), 0.5f);

    // Cream pointer with a dark seat and a coloured jewel at the tip.
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -bodyRadius + 1.0f, 3.0f, bodyRadius * 0.64f, 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.translated (0.0f, 1.2f)));
    g.setColour (colour::cream.withAlpha (0.99f * intensity));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));

    const float tipX = centre.x + std::sin (angle) * (bodyRadius * 0.52f);
    const float tipY = centre.y - std::cos (angle) * (bodyRadius * 0.52f);
    g.setColour (accent.withAlpha (0.90f * intensity));
    g.fillEllipse (tipX - 1.5f, tipY - 1.5f, 3.0f, 3.0f);

    if (active)
        drawGlow (g, centre, radius * 1.45f, accent, 0.30f);
}

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
