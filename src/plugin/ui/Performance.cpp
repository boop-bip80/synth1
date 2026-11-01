#include "Performance.h"

#include "../PluginProcessor.h"

#include <array>
#include <cmath>

namespace cs80::ui
{
namespace
{
void drawTraceLegend (juce::Graphics& g, juce::Point<float> origin)
{
    g.setFont (monoFont (6.3f, true));

    g.setColour (colour::amberHot);
    g.fillEllipse (origin.x, origin.y + 3.4f, 3.4f, 3.4f);
    g.drawText ("L", juce::Rectangle<int> ((int) origin.x + 6, (int) origin.y, 10, 9),
                juce::Justification::centredLeft, false);

    g.setColour (colour::ice);
    g.fillEllipse (origin.x + 24.0f, origin.y + 3.4f, 3.4f, 3.4f);
    g.drawText ("R", juce::Rectangle<int> ((int) origin.x + 30, (int) origin.y, 10, 9),
                juce::Justification::centredLeft, false);
}
} // namespace

//==============================================================================
ScopeDisplay::ScopeDisplay (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void ScopeDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawPlate (g, bounds, 4.0f);

    auto screen = bounds.reduced (4.5f, 4.5f);
    drawInsetWell (g, screen, 2.5f);

    // Warm phosphor glass.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c1a08),
                                             screen.getCentreX(), screen.getY(),
                                             juce::Colour (0xff080502),
                                             screen.getCentreX(), screen.getBottom(), false));
    g.fillRoundedRectangle (screen.reduced (1.0f), 2.0f);

    drawGlow (g, screen.getCentre(), screen.getWidth() * 0.62f, colour::amber, 0.17f);

    auto inner = screen.reduced (8.0f, 7.0f);

    // Graticule.
    g.setColour (colour::amber.withAlpha (0.14f));
    for (int i = 1; i < 4; ++i)
        g.drawLine (inner.getX(), inner.getY() + inner.getHeight() * (float) i / 4.0f,
                    inner.getRight(), inner.getY() + inner.getHeight() * (float) i / 4.0f, 0.6f);

    for (int i = 1; i < 6; ++i)
        g.drawLine (inner.getX() + inner.getWidth() * (float) i / 6.0f, inner.getY(),
                    inner.getX() + inner.getWidth() * (float) i / 6.0f, inner.getBottom(), 0.6f);

    g.setColour (colour::amber.withAlpha (0.20f));
    g.drawRect (inner, 0.7f);

    std::array<float, 512> samplesL {}, samplesR {};
    engine.readScopeStereo (samplesL.data(), samplesR.data(), (int) samplesL.size());

    int start = 0;
    for (int i = 1; i < 180; ++i)
    {
        if (samplesL[(size_t) i - 1] <= 0.0f && samplesL[(size_t) i] > 0.0f)
        {
            start = i;
            break;
        }
    }

    const int count = (int) samplesL.size() - start;

    if (count > 4)
    {
        g.setColour (colour::amber.withAlpha (0.16f));
        g.drawLine (inner.getX(), inner.getCentreY(), inner.getRight(), inner.getCentreY(), 0.6f);

        const float midY = inner.getCentreY();
        const float amplitude = inner.getHeight() * 0.37f;

        auto drawTrace = [&] (const std::array<float, 512>& samples, juce::Colour traceColour,
                              float thickness, float alpha)
        {
            juce::Path trace;
            juce::Path fill;

            for (int i = 0; i < count; ++i)
            {
                const float x = inner.getX() + (float) i / (float) (count - 1) * inner.getWidth();
                const float y = midY - juce::jlimit (-1.0f, 1.0f, samples[(size_t) (start + i)] * 1.9f) * amplitude;

                if (i == 0)
                {
                    trace.startNewSubPath (x, y);
                    fill.startNewSubPath (x, midY);
                    fill.lineTo (x, y);
                }
                else
                {
                    trace.lineTo (x, y);
                    fill.lineTo (x, y);
                }
            }

            fill.lineTo (inner.getRight(), midY);
            fill.closeSubPath();

            g.setColour (traceColour.withAlpha (alpha * 0.055f));
            g.fillPath (fill);

            g.setColour (traceColour.withAlpha (alpha * 0.11f));
            g.strokePath (trace, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            g.setColour (traceColour.withAlpha (alpha * 0.36f));
            g.strokePath (trace, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            g.setColour (traceColour.withAlpha (alpha));
            g.strokePath (trace, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        };

        // Right in cool phosphor, left in warm amber: a true stereo pair.
        drawTrace (samplesR, colour::ice, 0.9f, 0.64f);
        drawTrace (samplesL, colour::amberHot, 1.15f, 1.0f);

        const float endX = inner.getRight() - 2.0f;
        drawGlow (g, { endX, midY }, 11.0f, colour::amberHot, 0.62f);
        g.setColour (colour::amberHot);
        g.fillEllipse (endX - 1.7f, midY - 1.7f, 3.4f, 3.4f);
    }

    // Scanlines and glass reflection.
    g.setColour (juce::Colours::black.withAlpha (0.11f));
    for (float y = inner.getY(); y < inner.getBottom(); y += 3.0f)
        g.drawLine (inner.getX(), y, inner.getRight(), y, 0.55f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.075f),
                                             screen.getCentreX(), screen.getY(),
                                             juce::Colours::transparentWhite,
                                             screen.getCentreX(), screen.getY() + screen.getHeight() * 0.36f, false));
    g.fillRoundedRectangle (screen.reduced (1.0f), 2.0f);

    drawTrackedText (g, "DUAL TRACE", { (int) inner.getX(), (int) inner.getY() - 1, 82, 9 },
                     colour::amberDim.brighter (0.82f), juce::Justification::centredLeft,
                     monoFont (6.5f, true), 0.5f);

    drawTraceLegend (g, { (float) inner.getRight() - 66.0f, (float) inner.getY() - 1.0f });

    drawTrackedText (g, "POST FX", { (int) inner.getRight() - 62, (int) inner.getBottom() - 9, 62, 8 },
                     colour::amberDim.brighter (0.62f), juce::Justification::centredRight,
                     monoFont (5.9f), 0.4f);
}

//==============================================================================
MeterDisplay::MeterDisplay (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void MeterDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawPlate (g, bounds, 3.0f);

    drawTrackedText (g, "VU", { (int) bounds.getX(), (int) bounds.getY() + 2, (int) bounds.getWidth(), 8 },
                     colour::textFaint, juce::Justification::centred, monoFont (5.9f, true), 0.5f);

    const auto well = bounds.reduced (3.0f, 3.0f).withTrimmedTop (12.0f).withTrimmedBottom (14.0f);
    drawInsetWell (g, well, 2.0f);

    constexpr int segments = 20;
    const float barWidth = 6.5f;
    const float gap = 4.5f;
    const float totalWidth = barWidth * 2.0f + gap;
    const float startX = well.getCentreX() - totalWidth * 0.5f;
    const float usableHeight = well.getHeight() - 2.0f;
    const float segmentHeight = usableHeight / (float) segments;

    g.setColour (juce::Colours::white.withAlpha (0.12f));
    for (int i = 0; i <= 4; ++i)
    {
        const float y = well.getBottom() - 1.0f - usableHeight * (float) i / 4.0f;
        g.drawLine (well.getX() - 2.0f, y, well.getX(), y, 0.7f);
    }

    const float rawL = juce::jlimit (0.0f, 1.0f, engine.outputMeter (0));
    const float rawR = juce::jlimit (0.0f, 1.0f, engine.outputMeter (1));
    const float displayL = std::sqrt (rawL);
    const float displayR = std::sqrt (rawR);

    peakL = std::max (displayL, peakL * 0.962f);
    peakR = std::max (displayR, peakR * 0.962f);

    for (int channel = 0; channel < 2; ++channel)
    {
        const float display = channel == 0 ? displayL : displayR;
        const int lit = (int) std::ceil (display * (float) segments);

        for (int i = 0; i < segments; ++i)
        {
            const float y = well.getBottom() - 1.0f - (float) (i + 1) * segmentHeight;
            const bool on = i < lit;
            const float position = (float) i / (float) segments;

            auto segmentColour = colour::emerald;
            if (position > 0.90f)      segmentColour = colour::red;
            else if (position > 0.72f) segmentColour = colour::amber;
            else if (position > 0.48f) segmentColour = colour::sage;
            else                       segmentColour = colour::emerald;

            g.setColour (on ? segmentColour : juce::Colour (0xff151b20));
            g.fillRoundedRectangle (startX + (float) channel * (barWidth + gap), y,
                                    barWidth, segmentHeight - 1.4f, 0.8f);

            if (on)
            {
                g.setColour (segmentColour.withAlpha (0.18f));
                g.fillRoundedRectangle (startX + (float) channel * (barWidth + gap) - 0.8f, y - 0.5f,
                                        barWidth + 1.6f, segmentHeight, 1.2f);
            }
        }

        const float peak = channel == 0 ? peakL : peakR;
        const float peakY = well.getBottom() - 1.0f - peak * usableHeight;

        if (peak > 0.02f)
        {
            g.setColour ((peak > 0.9f ? colour::red : colour::cream).withAlpha (0.88f));
            g.fillRect (startX + (float) channel * (barWidth + gap) - 1.0f, peakY,
                        barWidth + 2.0f, 1.4f);
        }

        drawTrackedText (g, channel == 0 ? "L" : "R",
                         { (int) (startX + (float) channel * (barWidth + gap) - 2.0f),
                           (int) (bounds.getBottom() - 12.0f), (int) barWidth + 4, 8 },
                         colour::textDim, juce::Justification::centred, monoFont (5.8f, true), 0.1f);
    }
}

//==============================================================================
RibbonStrip::RibbonStrip (Cs80AudioProcessor& processorToUse) : processor (processorToUse)
{
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    setTooltip ("Pitch ribbon - drag left or right, release to return to centre");
    startTimerHz (60);
}

void RibbonStrip::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawInsetWell (g, bounds, 3.0f);

    auto rail = bounds.reduced (14.0f, 0.0f).withTrimmedTop (10.0f).withTrimmedBottom (5.0f);

    // Brushed bronze strip, like the CS-80's ribbon controller.
    juce::ColourGradient bronze (juce::Colour (0xff7a522b), rail.getCentreX(), rail.getY(),
                                 juce::Colour (0xff160c05), rail.getCentreX(), rail.getBottom(), false);
    bronze.addColour (0.32, juce::Colour (0xffb7834a));
    bronze.addColour (0.62, juce::Colour (0xff4f3116));
    g.setGradientFill (bronze);
    g.fillRoundedRectangle (rail, 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe0ac6c).withAlpha (0.48f),
                                             rail.getCentreX(), rail.getY(),
                                             juce::Colours::transparentBlack,
                                             rail.getCentreX(), rail.getCentreY(), false));
    g.fillRoundedRectangle (rail.reduced (1.0f), 2.0f);

    g.setColour (juce::Colour (0xffe0b47a).withAlpha (0.42f));
    g.drawLine (rail.getX() + 4.0f, rail.getY() + 1.0f,
                rail.getRight() - 4.0f, rail.getY() + 1.0f, 0.9f);

    g.setColour (juce::Colours::black.withAlpha (0.86f));
    g.drawRoundedRectangle (rail.reduced (0.5f), 2.5f, 1.0f);

    // Fine brushed metal streaks.
    for (int i = 0; i < 44; ++i)
    {
        const float y = rail.getY() + (float) i / 44.0f * rail.getHeight();
        g.setColour (juce::Colours::white.withAlpha (0.012f + 0.020f * (float) (i % 5) / 5.0f));
        g.drawLine (rail.getX() + 2.0f, y, rail.getRight() - 2.0f, y, 0.55f);
    }

    // Etched centre detent and travel ticks.
    const float centreX = rail.getCentreX();
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    for (int i = -8; i <= 8; ++i)
    {
        const float x = centreX + (float) i / 8.0f * rail.getWidth() * 0.47f;
        const float tickHeight = (i % 4 == 0) ? 8.0f : 4.0f;
        g.drawLine (x, rail.getCentreY() - tickHeight * 0.5f,
                    x, rail.getCentreY() + tickHeight * 0.5f, 0.7f);
    }

    g.setColour (colour::amber.withAlpha (0.34f));
    g.drawLine (centreX, rail.getY() + 3.0f, centreX, rail.getBottom() - 3.0f, 0.8f);

    g.setColour (colour::red.withAlpha (0.72f));
    g.fillRect (centreX - 0.9f, rail.getY() + 2.0f, 1.8f, 4.4f);

    // Spring coils at both ends.
    for (int side = -1; side <= 1; side += 2)
    {
        const float endX = centreX + (float) side * rail.getWidth() * 0.495f;
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawEllipse (endX - 3.0f + (float) side * (float) i * 1.4f,
                           rail.getCentreY() - 5.0f, 3.0f, 10.0f, 0.8f);
        }
    }

    const float thumbX = centreX + value * rail.getWidth() * 0.47f;

    if (std::abs (value) > 0.001f)
    {
        juce::Path trail;
        trail.addRoundedRectangle (juce::jmin (centreX, thumbX), rail.getCentreY() - 2.2f,
                                   std::abs (thumbX - centreX), 4.4f, 2.2f);
        g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.10f), centreX, 0.0f,
                                                 colour::amber.withAlpha (0.88f), thumbX, 0.0f, false));
        g.fillPath (trail);
        drawGlow (g, { thumbX, rail.getCentreY() }, 24.0f, colour::amber, dragging ? 0.62f : 0.32f);
    }

    const auto thumb = juce::Rectangle<float> (20.0f, rail.getHeight() + 10.0f)
                           .withCentre ({ thumbX, rail.getCentreY() });

    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 2.0f), 3.5f);

    juce::ColourGradient ivory (juce::Colour (0xfffff8ea), thumb.getCentreX(), thumb.getY(),
                                juce::Colour (0xffb7a588), thumb.getCentreX(), thumb.getBottom(), false);
    ivory.addColour (0.35, juce::Colour (0xfffdf2df));
    g.setGradientFill (ivory);
    g.fillRoundedRectangle (thumb, 3.5f);

    g.setColour (juce::Colours::black.withAlpha (0.66f));
    g.drawRoundedRectangle (thumb.reduced (0.5f), 3.5f, 0.9f);

    g.setColour (colour::amberDim);
    g.fillRect (thumb.getCentreX() - 0.8f, thumb.getY() + 3.0f, 1.6f, thumb.getHeight() - 6.0f);

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawLine (thumb.getX() + 3.0f, thumb.getY() + 1.8f, thumb.getRight() - 3.0f, thumb.getY() + 1.8f, 0.7f);

    drawTrackedText (g, "PITCH RIBBON", { 20, 0, 110, 10 }, colour::textDim.brighter (0.42f),
                     juce::Justification::centredLeft, trackedFont (7.6f, 1.5f, true), 0.3f);

    drawTrackedText (g, "SPRING RETURN  ·  ±12 ST", { 118, 1, 160, 9 },
                     colour::textFaint.brighter (0.38f), juce::Justification::centredLeft,
                     monoFont (6.2f), 0.2f);

    const auto semitones = value * 12.0f;
    const juce::String readout = (semitones >= 0.0f ? "+" : "")
                               + juce::String (semitones, 1) + " ST";

    drawTrackedText (g, readout, { getWidth() - 98, 0, 78, 10 },
                     dragging ? colour::amberHot : colour::textDim.brighter (0.22f),
                     juce::Justification::centredRight, monoFont (7.8f, true), 0.3f);
}

void RibbonStrip::setFromMouse (float x)
{
    const float railLeft = 15.0f;
    const float railWidth = (float) getWidth() - 30.0f;
    const float centre = railLeft + railWidth * 0.5f;
    const float normalised = (x - centre) / (railWidth * 0.47f);
    value = juce::jlimit (-1.0f, 1.0f, normalised);
    pushToProcessor();
    repaint();
}

void RibbonStrip::pushToProcessor()
{
    processor.setRibbon (value);
}

void RibbonStrip::mouseDown (const juce::MouseEvent& event)
{
    dragging = true;
    setFromMouse (event.position.x);
}

void RibbonStrip::mouseDrag (const juce::MouseEvent& event)
{
    dragging = true;
    setFromMouse (event.position.x);
}

void RibbonStrip::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

void RibbonStrip::mouseDoubleClick (const juce::MouseEvent&)
{
    value = 0.0f;
    dragging = false;
    pushToProcessor();
    repaint();
}

void RibbonStrip::timerCallback()
{
    if (! dragging && std::abs (value) > 0.0005f)
    {
        value *= 0.72f;

        if (std::abs (value) < 0.0005f)
            value = 0.0f;

        pushToProcessor();
        repaint();
    }
}

//==============================================================================
Cs80Keyboard::Cs80Keyboard (Cs80AudioProcessor& processorToUse)
    : processor (processorToUse), state (processorToUse.keyboardState)
{
    state.addListener (this);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

Cs80Keyboard::~Cs80Keyboard()
{
    state.removeListener (this);
}

void Cs80Keyboard::resized()
{
    whiteKeys.clear();
    blackKeys.clear();

    constexpr int lowNote = 36;
    constexpr int highNote = 96;
    constexpr int whiteCount = 36;

    const float railHeight = 15.0f;
    const float feltHeight = 4.0f;
    const float keyTop = railHeight + feltHeight + 1.0f;
    const float keyHeight = (float) getHeight() - keyTop - 2.0f;
    const float whiteWidth = (float) getWidth() / (float) whiteCount;

    juce::Rectangle<float> area (0.0f, keyTop, (float) getWidth(), keyHeight);
    int whiteIndex = 0;

    for (int note = lowNote; note <= highNote; ++note)
    {
        const int pitchClass = note % 12;
        const bool isBlack = (pitchClass == 1 || pitchClass == 3 || pitchClass == 6
                              || pitchClass == 8 || pitchClass == 10);

        if (isBlack)
        {
            const float centre = area.getX() + (float) whiteIndex * whiteWidth;
            const float width = whiteWidth * 0.56f;
            const float height = keyHeight * 0.63f;

            blackKeys.push_back ({ note, { centre - width * 0.5f, keyTop, width, height }, true });
        }
        else
        {
            whiteKeys.push_back ({ note, { area.getX() + (float) whiteIndex * whiteWidth, keyTop,
                                          whiteWidth - 1.1f, keyHeight }, false });
            ++whiteIndex;
        }
    }
}

int Cs80Keyboard::noteAt (juce::Point<float> position) const
{
    for (const auto& key : blackKeys)
        if (key.bounds.contains (position))
            return key.note;

    for (const auto& key : whiteKeys)
        if (key.bounds.contains (position))
            return key.note;

    return -1;
}

float Cs80Keyboard::velocityAt (float y) const
{
    const float keyTop = 20.0f;
    const float keyHeight = juce::jmax (1.0f, (float) getHeight() - keyTop - 2.0f);
    const float normalised = juce::jlimit (0.0f, 1.0f, (y - keyTop) / keyHeight);
    return 0.34f + 0.66f * normalised;
}

void Cs80Keyboard::updatePressure (float y)
{
    if (currentNote < 0)
        return;

    const float keyTop = 20.0f;
    const float keyHeight = juce::jmax (1.0f, (float) getHeight() - keyTop - 2.0f);
    const float pressure = juce::jlimit (0.0f, 1.0f, (y - keyTop) / keyHeight);

    if (std::abs (pressure - currentPressure) > 0.005f)
    {
        currentPressure = pressure;
        processor.getEngine().setPolyPressure (currentNote, pressure);
        repaint();
    }
}

void Cs80Keyboard::mouseDown (const juce::MouseEvent& event)
{
    const int note = noteAt (event.position);

    if (note >= 0)
    {
        stopCurrentNote();
        currentNote = note;
        currentPressure = 0.0f;
        state.noteOn (1, note, velocityAt (event.position.y));
        repaint();
    }
}

void Cs80Keyboard::mouseDrag (const juce::MouseEvent& event)
{
    const int note = noteAt (event.position);

    if (note != currentNote)
    {
        stopCurrentNote();

        if (note >= 0)
        {
            currentNote = note;
            currentPressure = 0.0f;
            state.noteOn (1, note, velocityAt (event.position.y));
        }

        repaint();
    }
    else
    {
        updatePressure (event.position.y);
    }
}

void Cs80Keyboard::mouseUp (const juce::MouseEvent&)
{
    stopCurrentNote();
    repaint();
}

void Cs80Keyboard::mouseExit (const juce::MouseEvent&)
{
    if (currentNote >= 0 && juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
        return;

    stopCurrentNote();
    repaint();
}

void Cs80Keyboard::stopCurrentNote()
{
    if (currentNote >= 0)
    {
        processor.getEngine().setPolyPressure (currentNote, 0.0f);
        state.noteOff (1, currentNote, 0.0f);
        currentNote = -1;
        currentPressure = 0.0f;
    }
}

void Cs80Keyboard::handleNoteOn (juce::MidiKeyboardState*, int, int, float)
{
    triggerAsyncUpdate();
}

void Cs80Keyboard::handleNoteOff (juce::MidiKeyboardState*, int, int, float)
{
    triggerAsyncUpdate();
}

void Cs80Keyboard::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Walnut rail above the keys, with a brass nameplate and a small lamp.
    juce::ColourGradient rail (juce::Colour (0xff57301a), 0.0f, 0.0f,
                               juce::Colour (0xff180b05), 0.0f, 15.0f, false);
    rail.addColour (0.45, juce::Colour (0xff2f170c));
    g.setGradientFill (rail);
    g.fillRect (bounds.withHeight (15.0f));

    for (int i = 0; i < 90; ++i)
    {
        const float y = (float) i / 90.0f * 15.0f;
        g.setColour (juce::Colours::black.withAlpha (0.04f + 0.04f * (float) (i % 7) / 7.0f));
        g.drawLine (0.0f, y, bounds.getRight(), y + 0.6f, 0.5f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawLine (0.0f, 15.0f, bounds.getRight(), 15.0f, 1.1f);
    g.setColour (colour::amber.withAlpha (0.16f));
    g.drawLine (0.0f, 0.6f, bounds.getRight(), 0.6f, 0.9f);

    // Brass nameplate.
    const juce::Rectangle<float> nameplate (8.0f, 3.0f, 190.0f, 9.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd7b076),
                                             nameplate.getCentreX(), nameplate.getY(),
                                             juce::Colour (0xff6b4a23),
                                             nameplate.getCentreX(), nameplate.getBottom(), false));
    g.fillRoundedRectangle (nameplate, 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (nameplate, 1.5f, 0.6f);

    drawTrackedText (g, "CS-80E  /  POLYPHONIC KEYBED", nameplate.toNearestInt(),
                     juce::Colour (0xff201207), juce::Justification::centred,
                     trackedFont (6.1f, 1.2f, true), 0.2f);

    drawLed (g, { (float) getWidth() - 18.0f, 5.5f, 4.4f, 4.4f }, true, colour::amber, 0.60f);

    drawTrackedText (g, "61 KEY  /  VELOCITY + POLY TOUCH  /  C2 - C7",
                     { getWidth() - 380, 2, 352, 10 },
                     colour::textFaint.brighter (0.26f), juce::Justification::centredRight,
                     monoFont (6.1f), 0.2f);

    // Red felt strip.
    drawFeltRail (g, bounds.withY (15.0f).withHeight (4.0f));

    // White keys.
    for (const auto& key : whiteKeys)
    {
        const bool down = state.isNoteOnForChannels (0xffff, key.note);

        g.setColour (juce::Colours::black.withAlpha (0.62f));
        g.fillRoundedRectangle (key.bounds.translated (0.0f, 2.0f), 2.5f);

        juce::ColourGradient ivory (down ? juce::Colour (0xffffd39a) : juce::Colour (0xfffffaf0),
                                    key.bounds.getCentreX(), key.bounds.getY(),
                                    down ? juce::Colour (0xffc68b43) : juce::Colour (0xffb9a586),
                                    key.bounds.getCentreX(), key.bounds.getBottom(), false);
        ivory.addColour (0.18, down ? juce::Colour (0xffffe4bb) : juce::Colour (0xfffffbf2));
        ivory.addColour (0.72, down ? juce::Colour (0xffd9a055) : juce::Colour (0xffded0b8));
        g.setGradientFill (ivory);
        g.fillRoundedRectangle (key.bounds, 2.5f);

        // Gloss band and front lip.
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (down ? 0.34f : 0.55f),
                                                 key.bounds.getCentreX(), key.bounds.getY() + 2.0f,
                                                 juce::Colours::transparentWhite,
                                                 key.bounds.getCentreX(), key.bounds.getY() + 22.0f, false));
        g.fillRoundedRectangle (key.bounds.reduced (1.6f, 1.2f), 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (key.bounds.getX() + 2.2f, key.bounds.getY() + 1.5f,
                    key.bounds.getX() + 2.2f, key.bounds.getBottom() - 4.0f, 0.7f);

        g.setColour (juce::Colours::black.withAlpha (0.34f));
        g.drawRoundedRectangle (key.bounds.reduced (0.5f), 2.5f, 1.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                 key.bounds.getCentreX(), key.bounds.getBottom() - 18.0f,
                                                 juce::Colours::black.withAlpha (0.20f),
                                                 key.bounds.getCentreX(), key.bounds.getBottom(), false));
        g.fillRoundedRectangle (key.bounds, 2.5f);

        if (down)
        {
            const float pressure = key.note == currentNote ? currentPressure : 0.0f;
            drawGlow (g, { key.bounds.getCentreX(), key.bounds.getCentreY() },
                      key.bounds.getWidth() * (0.85f + pressure * 0.50f), colour::amber,
                      0.34f + pressure * 0.36f);
        }

        if (key.note % 12 == 0)
        {
            drawTrackedText (g, "C" + juce::String (key.note / 12 - 1),
                             key.bounds.withTrimmedTop (key.bounds.getHeight() - 16.0f)
                                 .withHeight (11.0f)
                                 .toNearestInt(),
                             down ? juce::Colour (0xff4a2c0c) : juce::Colour (0xff75634b),
                             juce::Justification::centred, monoFont (8.7f, true), 0.2f);

            g.setColour (colour::amber.withAlpha (down ? 0.85f : 0.30f));
            g.fillEllipse (key.bounds.getCentreX() - 1.3f,
                           key.bounds.getBottom() - 6.0f, 2.6f, 2.6f);
        }
    }

    // Black keys.
    for (const auto& key : blackKeys)
    {
        const bool down = state.isNoteOnForChannels (0xffff, key.note);

        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.fillRoundedRectangle (key.bounds.translated (0.0f, 2.4f), 2.5f);

        juce::ColourGradient blackKey (down ? juce::Colour (0xff9a6228) : juce::Colour (0xff3d464e),
                                       key.bounds.getX(), key.bounds.getY(),
                                       down ? juce::Colour (0xff2d1a06) : juce::Colour (0xff050709),
                                       key.bounds.getX(), key.bounds.getBottom(), false);
        blackKey.addColour (0.18, down ? juce::Colour (0xffc1863f) : juce::Colour (0xff59636b));
        blackKey.addColour (0.62, down ? juce::Colour (0xff5a3510) : juce::Colour (0xff171d22));
        g.setGradientFill (blackKey);
        g.fillRoundedRectangle (key.bounds, 2.5f);

        g.setColour (juce::Colours::white.withAlpha (0.26f));
        g.drawLine (key.bounds.getX() + 2.0f, key.bounds.getY() + 2.0f,
                    key.bounds.getRight() - 2.0f, key.bounds.getY() + 2.0f, 0.9f);

        g.setColour (juce::Colours::black.withAlpha (0.94f));
        g.drawRoundedRectangle (key.bounds.reduced (0.5f), 2.5f, 1.0f);

        if (down)
        {
            g.setColour (colour::amber.withAlpha (0.92f));
            g.fillRoundedRectangle (key.bounds.withTop (key.bounds.getBottom() - 4.0f).reduced (1.0f, 0.0f), 1.5f);
            drawGlow (g, { key.bounds.getCentreX(), key.bounds.getBottom() - 7.0f },
                      15.0f, colour::amber, 0.40f);
        }
    }
}
} // namespace cs80::ui
