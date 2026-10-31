#include "Performance.h"

#include "../PluginProcessor.h"

#include <array>
#include <cmath>

namespace cs80::ui
{
//==============================================================================
ScopeDisplay::ScopeDisplay (const Engine& engineToUse) : engine (engineToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void ScopeDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawInsetWell (g, bounds, 3.0f);

    auto inner = bounds.reduced (5.0f, 4.0f);

    // Phosphor grid.
    g.setColour (colour::amber.withAlpha (0.10f));
    for (int i = 1; i < 4; ++i)
        g.drawLine (inner.getX(), inner.getY() + inner.getHeight() * (float) i / 4.0f,
                    inner.getRight(), inner.getY() + inner.getHeight() * (float) i / 4.0f, 0.7f);

    for (int i = 1; i < 6; ++i)
        g.drawLine (inner.getX() + inner.getWidth() * (float) i / 6.0f, inner.getY(),
                    inner.getX() + inner.getWidth() * (float) i / 6.0f, inner.getBottom(), 0.7f);

    std::array<float, 512> samples {};
    engine.readScope (samples.data(), (int) samples.size());

    int start = 0;
    for (int i = 1; i < 180; ++i)
    {
        if (samples[(size_t) i - 1] <= 0.0f && samples[(size_t) i] > 0.0f)
        {
            start = i;
            break;
        }
    }

    const int count = (int) samples.size() - start;

    if (count > 4)
    {
        g.setColour (colour::amber.withAlpha (0.14f));
        g.drawLine (inner.getX(), inner.getCentreY(), inner.getRight(), inner.getCentreY(), 0.7f);

        juce::Path trace;
        const float midY = inner.getCentreY();
        const float amplitude = inner.getHeight() * 0.36f;

        for (int i = 0; i < count; ++i)
        {
            const float x = inner.getX() + (float) i / (float) (count - 1) * inner.getWidth();
            const float y = midY - juce::jlimit (-1.0f, 1.0f, samples[(size_t) (start + i)] * 1.8f) * amplitude;

            if (i == 0)
                trace.startNewSubPath (x, y);
            else
                trace.lineTo (x, y);
        }

        g.setColour (colour::amber.withAlpha (0.10f));
        g.strokePath (trace, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (colour::amber.withAlpha (0.35f));
        g.strokePath (trace, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (colour::amberHot.withAlpha (0.95f));
        g.strokePath (trace, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Scanline sheen.
        // Scanline sheen.
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        for (float y = inner.getY(); y < inner.getBottom(); y += 3.0f)
            g.drawLine (inner.getX(), y, inner.getRight(), y, 0.6f);

        const float endX = inner.getRight() - 2.0f;
        drawGlow (g, { endX, midY }, 8.0f, colour::amberHot, 0.5f);
        g.setColour (colour::amberHot);
        g.fillEllipse (endX - 1.5f, midY - 1.5f, 3.0f, 3.0f);
    }

    drawTrackedText (g, "POST FX", { (int) inner.getRight() - 58, (int) inner.getBottom() - 11, 54, 9 },
                     colour::amberDim.brighter (0.35f), juce::Justification::centredRight,
                     monoFont (6.0f), 0.6f);
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
    drawInsetWell (g, bounds, 3.0f);

    constexpr int segments = 16;
    const float barWidth = 6.0f;
    const float gap = 6.0f;
    const float totalWidth = barWidth * 2.0f + gap;
    const float startX = bounds.getCentreX() - totalWidth * 0.5f;
    const float top = bounds.getY() + 4.0f;
    const float usableHeight = bounds.getHeight() - 18.0f;
    const float segmentHeight = usableHeight / (float) segments;

    for (int channel = 0; channel < 2; ++channel)
    {
        const float meter = juce::jlimit (0.0f, 1.0f, engine.outputMeter (channel));
        const float display = std::sqrt (meter);
        const int lit = (int) std::ceil (display * (float) segments);

        for (int i = 0; i < segments; ++i)
        {
            const float y = top + usableHeight - (float) (i + 1) * segmentHeight;
            const bool on = i < lit;
            const float position = (float) i / (float) segments;

            auto segmentColour = colour::amber;
            if (position > 0.82f) segmentColour = colour::red;
            else if (position > 0.65f) segmentColour = colour::amberHot;

            g.setColour (on ? segmentColour : juce::Colour (0xff1d2126));
            g.fillRoundedRectangle (startX + (float) channel * (barWidth + gap), y,
                                    barWidth, segmentHeight - 1.6f, 1.0f);

            if (on)
            {
                g.setColour (segmentColour.withAlpha (0.18f));
                g.fillRoundedRectangle (startX + (float) channel * (barWidth + gap) - 1.0f, y - 0.5f,
                                        barWidth + 2.0f, segmentHeight, 1.2f);
            }
        }

        drawTrackedText (g, channel == 0 ? "L" : "R",
                         { (int) (startX + (float) channel * (barWidth + gap) - 2.0f),
                           (int) (bounds.getBottom() - 13.0f), (int) barWidth + 4, 9 },
                         colour::textDim, juce::Justification::centred, monoFont (6.5f, true), 0.2f);
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
    drawInsetWell (g, bounds.reduced (0.0f, 2.0f), 4.0f);

    auto rail = bounds.reduced (10.0f, 7.0f);
    rail.setHeight (rail.getHeight() - 2.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff120d0a), rail.getCentreX(), rail.getY(),
                                             juce::Colour (0xff2b1a12), rail.getCentreX(), rail.getBottom(), false));
    g.fillRoundedRectangle (rail, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (rail.reduced (0.5f), 3.0f, 1.0f);

    // Etched centre detent and travel ticks.
    const float centreX = rail.getCentreX();
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    for (int i = -6; i <= 6; ++i)
    {
        const float x = centreX + (float) i / 6.0f * rail.getWidth() * 0.46f;
        const float tickHeight = (i % 3 == 0) ? 8.0f : 4.0f;
        g.drawLine (x, rail.getCentreY() - tickHeight * 0.5f,
                    x, rail.getCentreY() + tickHeight * 0.5f, 0.7f);
    }

    const float thumbX = centreX + value * rail.getWidth() * 0.47f;

    if (std::abs (value) > 0.001f)
    {
        juce::Path trail;
        trail.addRoundedRectangle (juce::jmin (centreX, thumbX), rail.getCentreY() - 2.0f,
                                   std::abs (thumbX - centreX), 4.0f, 2.0f);
        g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.15f), centreX, 0.0f,
                                                 colour::amber.withAlpha (0.75f), thumbX, 0.0f, false));
        g.fillPath (trail);
    }

    const auto thumb = juce::Rectangle<float> (16.0f, rail.getHeight() + 10.0f)
                           .withCentre ({ thumbX, rail.getCentreY() });

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 1.5f), 3.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8b939d), thumb.getCentreX(), thumb.getY(),
                                             juce::Colour (0xff2a2e34), thumb.getCentreX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.0f);

    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (thumb.reduced (0.5f), 3.0f, 0.8f);

    g.setColour (colour::amber.withAlpha (dragging ? 0.95f : 0.6f));
    g.fillRect (thumb.getCentreX() - 0.6f, thumb.getY() + 4.0f, 1.2f, thumb.getHeight() - 8.0f);

    drawTrackedText (g, "RIBBON", { 14, 3, 70, 10 }, colour::textDim.brighter (0.15f),
                     juce::Justification::centredLeft, trackedFont (6.8f, 1.4f, true), 0.3f);

    const auto semitones = value * 12.0f;
    const juce::String readout = (semitones >= 0.0f ? "+" : "")
                               + juce::String (semitones, 1) + " ST";

    drawTrackedText (g, readout, { getWidth() - 88, 3, 74, 10 },
                     dragging ? colour::amberHot : colour::textDim,
                     juce::Justification::centredRight, monoFont (7.0f, true), 0.3f);
}

void RibbonStrip::setFromMouse (float x)
{
    const float railLeft = 12.0f;
    const float railWidth = (float) getWidth() - 24.0f;
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
Cs80Keyboard::Cs80Keyboard (juce::MidiKeyboardState& stateToUse) : state (stateToUse)
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
    constexpr int highNote = 72;
    constexpr int whiteCount = 22;

    const float railHeight = 9.0f;
    const float keyTop = railHeight + 2.0f;
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
            const float width = whiteWidth * 0.58f;
            const float height = keyHeight * 0.64f;

            blackKeys.push_back ({ note, { centre - width * 0.5f, keyTop, width, height }, true });
        }
        else
        {
            whiteKeys.push_back ({ note, { area.getX() + (float) whiteIndex * whiteWidth, keyTop,
                                          whiteWidth - 1.2f, keyHeight }, false });
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
    const float railHeight = 11.0f;
    const float keyHeight = juce::jmax (1.0f, (float) getHeight() - railHeight - 2.0f);
    const float normalised = juce::jlimit (0.0f, 1.0f, (y - railHeight) / keyHeight);
    return 0.35f + 0.65f * normalised;
}

void Cs80Keyboard::mouseDown (const juce::MouseEvent& event)
{
    const int note = noteAt (event.position);

    if (note >= 0)
    {
        stopCurrentNote();
        currentNote = note;
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
            state.noteOn (1, note, velocityAt (event.position.y));
        }

        repaint();
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
        state.noteOff (1, currentNote, 0.0f);
        currentNote = -1;
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

    // Felt / wood rail above the keys.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a2314), 0.0f, 0.0f,
                                             juce::Colour (0xff1e1109), 0.0f, 9.0f, false));
    g.fillRect (bounds.withHeight (9.0f));
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawLine (0.0f, 9.0f, bounds.getRight(), 9.0f, 1.0f);
    g.setColour (colour::amber.withAlpha (0.10f));
    g.drawLine (0.0f, 0.5f, bounds.getRight(), 0.5f, 1.0f);

    // White keys.
    for (const auto& key : whiteKeys)
    {
        const bool down = state.isNoteOnForChannels (0xffff, key.note);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (key.bounds.translated (0.0f, 1.5f), 2.0f);

        g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xfff3c98d) : colour::keyIvory,
                                                 key.bounds.getCentreX(), key.bounds.getY(),
                                                 down ? juce::Colour (0xffc98a3f) : juce::Colour (0xffcfc5b2),
                                                 key.bounds.getCentreX(), key.bounds.getBottom(), false));
        g.fillRoundedRectangle (key.bounds, 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawRoundedRectangle (key.bounds.reduced (0.5f), 2.0f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawLine (key.bounds.getX() + 2.0f, key.bounds.getY() + 1.0f,
                    key.bounds.getX() + 2.0f, key.bounds.getBottom() - 3.0f, 0.8f);

        // Front lip that catches the light.
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.fillRoundedRectangle (key.bounds.withTop (key.bounds.getBottom() - 3.0f).reduced (1.0f, 0.0f), 1.5f);

        if (down)
            drawGlow (g, { key.bounds.getCentreX(), key.bounds.getCentreY() },
                      key.bounds.getWidth() * 0.7f, colour::amber, 0.35f);

        // Octave labels on C keys.
        if (key.note % 12 == 0)
        {
            drawTrackedText (g, "C" + juce::String (key.note / 12 - 1),
                             key.bounds.withTrimmedTop (key.bounds.getHeight() - 16.0f)
                                 .withHeight (12.0f)
                                 .toNearestInt(),
                             down ? juce::Colour (0xff4a2c0c) : juce::Colour (0xff6b5a45),
                             juce::Justification::centred, monoFont (9.0f, true), 0.2f);
        }
    }

    // Black keys.
    for (const auto& key : blackKeys)
    {
        const bool down = state.isNoteOnForChannels (0xffff, key.note);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (key.bounds.translated (0.0f, 2.0f), 2.5f);

        g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xff8a5420) : juce::Colour (0xff33373d),
                                                 key.bounds.getX(), key.bounds.getY(),
                                                 down ? juce::Colour (0xff3a2208) : juce::Colour (0xff08090b),
                                                 key.bounds.getX(), key.bounds.getBottom(), false));
        g.fillRoundedRectangle (key.bounds, 2.5f);

        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawLine (key.bounds.getX() + 2.0f, key.bounds.getY() + 2.0f,
                    key.bounds.getRight() - 2.0f, key.bounds.getY() + 2.0f, 0.9f);

        g.setColour (juce::Colours::black.withAlpha (0.9f));
        g.drawRoundedRectangle (key.bounds.reduced (0.5f), 2.5f, 1.0f);

        if (down)
        {
            g.setColour (colour::amber.withAlpha (0.8f));
            g.fillRoundedRectangle (key.bounds.withTop (key.bounds.getBottom() - 4.0f).reduced (1.0f, 0.0f), 1.5f);
            drawGlow (g, { key.bounds.getCentreX(), key.bounds.getBottom() - 6.0f },
                      12.0f, colour::amber, 0.35f);
        }
    }
}
} // namespace cs80::ui
