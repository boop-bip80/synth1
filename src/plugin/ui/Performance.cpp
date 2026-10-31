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
    drawPlate (g, bounds, 4.0f);

    auto screen = bounds.reduced (5.0f, 5.0f);
    drawInsetWell (g, screen, 2.5f);

    // Warm glass.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff261708),
                                             screen.getCentreX(), screen.getY(),
                                             juce::Colour (0xff090602),
                                             screen.getCentreX(), screen.getBottom(), false));
    g.fillRoundedRectangle (screen.reduced (1.0f), 2.0f);

    drawGlow (g, screen.getCentre(), screen.getWidth() * 0.55f, colour::amber, 0.14f);

    auto inner = screen.reduced (7.0f, 6.0f);

    g.setColour (colour::amber.withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
        g.drawLine (inner.getX(), inner.getY() + inner.getHeight() * (float) i / 4.0f,
                    inner.getRight(), inner.getY() + inner.getHeight() * (float) i / 4.0f, 0.6f);

    for (int i = 1; i < 6; ++i)
        g.drawLine (inner.getX() + inner.getWidth() * (float) i / 6.0f, inner.getY(),
                    inner.getX() + inner.getWidth() * (float) i / 6.0f, inner.getBottom(), 0.6f);

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
        g.setColour (colour::amber.withAlpha (0.15f));
        g.drawLine (inner.getX(), inner.getCentreY(), inner.getRight(), inner.getCentreY(), 0.6f);

        const float midY = inner.getCentreY();
        const float amplitude = inner.getHeight() * 0.37f;

        auto drawTrace = [&] (const std::array<float, 512>& samples, juce::Colour traceColour,
                              float thickness, float alpha)
        {
            juce::Path trace;

            for (int i = 0; i < count; ++i)
            {
                const float x = inner.getX() + (float) i / (float) (count - 1) * inner.getWidth();
                const float y = midY - juce::jlimit (-1.0f, 1.0f, samples[(size_t) (start + i)] * 1.9f) * amplitude;

                if (i == 0)
                    trace.startNewSubPath (x, y);
                else
                    trace.lineTo (x, y);
            }

            g.setColour (traceColour.withAlpha (alpha * 0.10f));
            g.strokePath (trace, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            g.setColour (traceColour.withAlpha (alpha * 0.34f));
            g.strokePath (trace, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            g.setColour (traceColour.withAlpha (alpha));
            g.strokePath (trace, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        };

        // Right first, a little quieter: a true stereo pair, not a faked shadow.
        drawTrace (samplesR, colour::copper, 0.9f, 0.58f);
        drawTrace (samplesL, colour::amberHot, 1.15f, 0.98f);

        const float endX = inner.getRight() - 2.0f;
        drawGlow (g, { endX, midY }, 10.0f, colour::amberHot, 0.55f);
        g.setColour (colour::amberHot);
        g.fillEllipse (endX - 1.6f, midY - 1.6f, 3.2f, 3.2f);
    }

    // Scanlines and a soft glass reflection.
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    for (float y = inner.getY(); y < inner.getBottom(); y += 3.0f)
        g.drawLine (inner.getX(), y, inner.getRight(), y, 0.55f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f),
                                             screen.getCentreX(), screen.getY(),
                                             juce::Colours::transparentWhite,
                                             screen.getCentreX(), screen.getY() + screen.getHeight() * 0.36f, false));
    g.fillRoundedRectangle (screen.reduced (1.0f), 2.0f);

    drawTrackedText (g, "OUTPUT", { (int) inner.getX(), (int) inner.getY() - 1, 60, 8 },
                     colour::amberDim.brighter (0.55f), juce::Justification::centredLeft,
                     monoFont (5.8f, true), 0.5f);

    drawTrackedText (g, "POST FX  ·  L / R", { (int) inner.getRight() - 78, (int) inner.getBottom() - 8, 78, 7 },
                     colour::amberDim.brighter (0.50f), juce::Justification::centredRight,
                     monoFont (5.4f), 0.4f);
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

    const auto well = bounds.reduced (3.0f, 3.0f).withTrimmedTop (11.0f).withTrimmedBottom (13.0f);
    drawInsetWell (g, well, 2.0f);

    drawTrackedText (g, "VU", { (int) bounds.getX(), (int) bounds.getY() + 2, (int) bounds.getWidth(), 8 },
                     colour::textFaint, juce::Justification::centred, monoFont (5.8f, true), 0.4f);

    constexpr int segments = 18;
    const float barWidth = 6.0f;
    const float gap = 5.0f;
    const float totalWidth = barWidth * 2.0f + gap;
    const float startX = well.getCentreX() - totalWidth * 0.5f;
    const float usableHeight = well.getHeight() - 2.0f;
    const float segmentHeight = usableHeight / (float) segments;

    // Scale ticks.
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    for (int i = 0; i <= 4; ++i)
    {
        const float y = well.getBottom() - 1.0f - usableHeight * (float) i / 4.0f;
        g.drawLine (well.getX() - 2.0f, y, well.getX(), y, 0.7f);
    }

    for (int channel = 0; channel < 2; ++channel)
    {
        const float meter = juce::jlimit (0.0f, 1.0f, engine.outputMeter (channel));
        const float display = std::sqrt (meter);
        const int lit = (int) std::ceil (display * (float) segments);

        for (int i = 0; i < segments; ++i)
        {
            const float y = well.getBottom() - 1.0f - (float) (i + 1) * segmentHeight;
            const bool on = i < lit;
            const float position = (float) i / (float) segments;

            auto segmentColour = colour::green;
            if (position > 0.88f) segmentColour = colour::red;
            else if (position > 0.66f) segmentColour = colour::amber;
            else if (position > 0.40f) segmentColour = colour::sage;

            g.setColour (on ? segmentColour : juce::Colour (0xff171d23));
            g.fillRoundedRectangle (startX + (float) channel * (barWidth + gap), y,
                                    barWidth, segmentHeight - 1.4f, 0.8f);

            if (on)
            {
                g.setColour (segmentColour.withAlpha (0.16f));
                g.fillRoundedRectangle (startX + (float) channel * (barWidth + gap) - 1.0f, y - 0.5f,
                                        barWidth + 2.0f, segmentHeight, 1.2f);
            }
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

    auto rail = bounds.reduced (12.0f, 6.0f);

    // Brushed bronze strip, like the CS-80's ribbon controller.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a4725), rail.getCentreX(), rail.getY(),
                                             juce::Colour (0xff180d06), rail.getCentreX(), rail.getBottom(), false));
    g.fillRoundedRectangle (rail, 2.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd3a065).withAlpha (0.40f),
                                             rail.getCentreX(), rail.getY(),
                                             juce::Colours::transparentBlack,
                                             rail.getCentreX(), rail.getCentreY(), false));
    g.fillRoundedRectangle (rail.reduced (1.0f), 2.0f);

    g.setColour (juce::Colour (0xffd9ab6c).withAlpha (0.34f));
    g.drawLine (rail.getX() + 4.0f, rail.getY() + 1.0f,
                rail.getRight() - 4.0f, rail.getY() + 1.0f, 0.9f);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (rail.reduced (0.5f), 2.5f, 1.0f);

    // Etched centre detent and travel ticks.
    const float centreX = rail.getCentreX();
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    for (int i = -6; i <= 6; ++i)
    {
        const float x = centreX + (float) i / 6.0f * rail.getWidth() * 0.46f;
        const float tickHeight = (i % 3 == 0) ? 8.0f : 4.0f;
        g.drawLine (x, rail.getCentreY() - tickHeight * 0.5f,
                    x, rail.getCentreY() + tickHeight * 0.5f, 0.7f);
    }

    g.setColour (colour::amber.withAlpha (0.30f));
    g.drawLine (centreX, rail.getY() + 3.0f, centreX, rail.getBottom() - 3.0f, 0.8f);

    // A thin red felt centre marker.
    g.setColour (colour::red.withAlpha (0.65f));
    g.fillRect (centreX - 0.8f, rail.getY() + 2.0f, 1.6f, 4.0f);

    const float thumbX = centreX + value * rail.getWidth() * 0.47f;

    if (std::abs (value) > 0.001f)
    {
        juce::Path trail;
        trail.addRoundedRectangle (juce::jmin (centreX, thumbX), rail.getCentreY() - 2.0f,
                                   std::abs (thumbX - centreX), 4.0f, 2.0f);
        g.setGradientFill (juce::ColourGradient (colour::amber.withAlpha (0.12f), centreX, 0.0f,
                                                 colour::amber.withAlpha (0.80f), thumbX, 0.0f, false));
        g.fillPath (trail);
    }

    const auto thumb = juce::Rectangle<float> (18.0f, rail.getHeight() + 10.0f)
                           .withCentre ({ thumbX, rail.getCentreY() });

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 1.5f), 3.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7ecd8), thumb.getCentreX(), thumb.getY(),
                                             juce::Colour (0xffb4a58c), thumb.getCentreX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (thumb.reduced (0.5f), 3.0f, 0.9f);

    g.setColour (colour::amberDim);
    g.fillRect (thumb.getCentreX() - 0.7f, thumb.getY() + 3.0f, 1.4f, thumb.getHeight() - 6.0f);

    drawTrackedText (g, "PITCH RIBBON", { 18, 3, 90, 10 }, colour::textDim.brighter (0.12f),
                     juce::Justification::centredLeft, trackedFont (6.6f, 1.4f, true), 0.3f);

    const auto semitones = value * 12.0f;
    const juce::String readout = (semitones >= 0.0f ? "+" : "")
                               + juce::String (semitones, 1) + " ST";

    drawTrackedText (g, readout, { getWidth() - 90, 3, 72, 10 },
                     dragging ? colour::amberHot : colour::textDim,
                     juce::Justification::centredRight, monoFont (6.8f, true), 0.3f);

    drawTrackedText (g, "TOUCH + BEND", { getWidth() - 172, 3, 74, 10 },
                     colour::textFaint, juce::Justification::centredRight,
                     monoFont (5.5f), 0.2f);
}

void RibbonStrip::setFromMouse (float x)
{
    const float railLeft = 14.0f;
    const float railWidth = (float) getWidth() - 28.0f;
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

    const float railHeight = 10.0f;
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
    const float railHeight = 12.0f;
    const float keyHeight = juce::jmax (1.0f, (float) getHeight() - railHeight - 2.0f);
    const float normalised = juce::jlimit (0.0f, 1.0f, (y - railHeight) / keyHeight);
    return 0.35f + 0.65f * normalised;
}

void Cs80Keyboard::updatePressure (float y)
{
    if (currentNote < 0)
        return;

    const float railHeight = 12.0f;
    const float keyHeight = juce::jmax (1.0f, (float) getHeight() - railHeight - 2.0f);
    const float pressure = juce::jlimit (0.0f, 1.0f, (y - railHeight) / keyHeight);

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
        // The CS-80 keybed is pressure sensitive. Sliding down a held key
        // sends per-note aftertouch, which is where the instrument sings.
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

    // Walnut rail above the keys, with a small engraved nameplate.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4b2814), 0.0f, 0.0f,
                                             juce::Colour (0xff1b0c05), 0.0f, 10.0f, false));
    g.fillRect (bounds.withHeight (10.0f));
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.drawLine (0.0f, 10.0f, bounds.getRight(), 10.0f, 1.0f);
    g.setColour (colour::amber.withAlpha (0.12f));
    g.drawLine (0.0f, 0.6f, bounds.getRight(), 0.6f, 0.9f);

    drawTrackedText (g, "CS-80E  /  KEYBED", { 8, 1, 140, 8 },
                     colour::amberDim.brighter (0.55f), juce::Justification::centredLeft,
                     monoFont (5.4f), 0.3f);

    drawTrackedText (g, "61 KEY  /  VELOCITY + POLY TOUCH  /  C2 - C7", { getWidth() - 290, 1, 282, 8 },
                     colour::textFaint, juce::Justification::centredRight,
                     monoFont (5.2f), 0.2f);

    // White keys.
    for (const auto& key : whiteKeys)
    {
        const bool down = state.isNoteOnForChannels (0xffff, key.note);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (key.bounds.translated (0.0f, 1.8f), 2.0f);

        g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xfff0c184) : juce::Colour (0xfff2e8d6),
                                                 key.bounds.getCentreX(), key.bounds.getY(),
                                                 down ? juce::Colour (0xffbd8340) : juce::Colour (0xffcfc2ac),
                                                 key.bounds.getCentreX(), key.bounds.getBottom(), false));
        g.fillRoundedRectangle (key.bounds, 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.drawRoundedRectangle (key.bounds.reduced (0.5f), 2.0f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.40f));
        g.drawLine (key.bounds.getX() + 2.0f, key.bounds.getY() + 1.0f,
                    key.bounds.getX() + 2.0f, key.bounds.getBottom() - 3.0f, 0.8f);

        // Felt shadow at the front of the key.
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                 key.bounds.getCentreX(), key.bounds.getBottom() - 16.0f,
                                                 juce::Colours::black.withAlpha (0.22f),
                                                 key.bounds.getCentreX(), key.bounds.getBottom(), false));
        g.fillRoundedRectangle (key.bounds, 2.0f);

        if (down)
        {
            const float pressure = key.note == currentNote ? currentPressure : 0.0f;
            drawGlow (g, { key.bounds.getCentreX(), key.bounds.getCentreY() },
                      key.bounds.getWidth() * (0.8f + pressure * 0.4f), colour::amber,
                      0.30f + pressure * 0.35f);
        }

        if (key.note % 12 == 0)
        {
            drawTrackedText (g, "C" + juce::String (key.note / 12 - 1),
                             key.bounds.withTrimmedTop (key.bounds.getHeight() - 15.0f)
                                 .withHeight (11.0f)
                                 .toNearestInt(),
                             down ? juce::Colour (0xff4a2c0c) : juce::Colour (0xff6d5b45),
                             juce::Justification::centred, monoFont (8.0f, true), 0.2f);
        }
    }

    // Black keys.
    for (const auto& key : blackKeys)
    {
        const bool down = state.isNoteOnForChannels (0xffff, key.note);

        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (key.bounds.translated (0.0f, 2.2f), 2.5f);

        g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xff8a5420) : juce::Colour (0xff3a4046),
                                                 key.bounds.getX(), key.bounds.getY(),
                                                 down ? juce::Colour (0xff2d1a06) : juce::Colour (0xff07090b),
                                                 key.bounds.getX(), key.bounds.getBottom(), false));
        g.fillRoundedRectangle (key.bounds, 2.5f);

        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawLine (key.bounds.getX() + 2.0f, key.bounds.getY() + 2.0f,
                    key.bounds.getRight() - 2.0f, key.bounds.getY() + 2.0f, 0.9f);

        g.setColour (juce::Colours::black.withAlpha (0.9f));
        g.drawRoundedRectangle (key.bounds.reduced (0.5f), 2.5f, 1.0f);

        if (down)
        {
            g.setColour (colour::amber.withAlpha (0.85f));
            g.fillRoundedRectangle (key.bounds.withTop (key.bounds.getBottom() - 4.0f).reduced (1.0f, 0.0f), 1.5f);
            drawGlow (g, { key.bounds.getCentreX(), key.bounds.getBottom() - 6.0f },
                      13.0f, colour::amber, 0.35f);
        }
    }
}
} // namespace cs80::ui
