#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr float W = 841.0f, H = 815.0f;           // native artwork size
    constexpr float KX = 420.0f, KY = 348.0f;         // knob centre
    constexpr float KNOB_R = 150.0f;                  // knob radius
    constexpr float RING_R = 180.0f;                  // value ring radius
    constexpr float A0 = -juce::MathConstants<float>::pi * 0.75f;   // 7:30
    constexpr float A1 =  juce::MathConstants<float>::pi * 0.75f;   // 4:30
    const juce::Colour amber (0xffffa21f);
    const juce::Colour amberHot (0xffffc35a);
    const juce::Rectangle<float> readout (331.0f, 644.0f, 180.0f, 33.0f);

    juce::Point<float> polar (float r, float a) { return { KX + r * std::sin (a), KY - r * std::cos (a) }; }

    juce::Font mono (float h)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain));
    }
}

StuckEditor::StuckEditor (StuckProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    panel   = juce::ImageCache::getFromMemory (BinaryData::panel_png, BinaryData::panel_pngSize);
    knobImg = juce::ImageCache::getFromMemory (BinaryData::knob_png,  BinaryData::knob_pngSize);

    knob.setLookAndFeel (&lnf);
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (A0 + juce::MathConstants<float>::twoPi, A1 + juce::MathConstants<float>::twoPi, true);
    knob.setMouseDragSensitivity (300);
    knob.setVelocityBasedMode (false);
    knob.setDoubleClickReturnValue (true, 0.0);
    knob.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    knob.setPopupDisplayEnabled (false, false, nullptr);
    knob.onValueChange = [this] { repaint(); };
    addAndMakeVisible (knob);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "stutter", knob);

    setResizable (true, true);
    setResizeLimits (420, 407, 1262, 1223);
    getConstrainer()->setFixedAspectRatio (W / H);
    setSize (560, 543);
    startTimerHz (30);
}

StuckEditor::~StuckEditor()
{
    stopTimer();
    knob.setLookAndFeel (nullptr);
}

void StuckEditor::timerCallback()
{
    const float ms = proc.engine.currentLoopMs.load();
    if (std::abs (ms - shownLoopMs) > 0.05f) { shownLoopMs = ms; repaint(); }
}

void StuckEditor::resized()
{
    const float s = (float) getWidth() / W;
    const float r = (RING_R + 14.0f) * s;
    knob.setBounds (juce::Rectangle<float> (2 * r, 2 * r).withCentre ({ KX * s, KY * s }).toNearestInt());
}

void StuckEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0b0a));
    const float s = (float) getWidth() / W;
    g.addTransform (juce::AffineTransform::scale (s));

    // faceplate
    g.drawImage (panel, juce::Rectangle<float> (0, 0, W, H), juce::RectanglePlacement::stretchToFit);

    const float v = (float) knob.getValue();
    const bool on = v > 0.0005f;
    const float ang = A0 + v * (A1 - A0);

    // ---- knob drop shadow ----
    {
        // soft drop shadow
        for (int i = 0; i < 10; ++i)
        {
            const float rr = KNOB_R + 4.0f + i * 2.2f;
            g.setColour (juce::Colours::black.withAlpha (0.06f));
            g.fillEllipse (KX - rr + 6.0f, KY - rr + 10.0f, rr * 2, rr * 2);
        }
    }

    // ---- value ring: groove + glowing amber arc ----
    {
        juce::Path track;
        track.addCentredArc (KX, KY, RING_R, RING_R, 0.0f, A0, A1, true);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (track, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff2a2622).withAlpha (0.9f));
        g.strokePath (track, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (on)
        {
            juce::Path arc;
            arc.addCentredArc (KX, KY, RING_R, RING_R, 0.0f, A0, ang, true);
            for (int i = 6; i >= 1; --i)   // soft lamp glow
            {
                g.setColour (amber.withAlpha (0.045f * (7 - i) * 0.5f));
                g.strokePath (arc, juce::PathStrokeType (4.5f + i * 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour (amber);
            g.strokePath (arc, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (amberHot.withAlpha (0.8f));
            g.strokePath (arc, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // ---- knob (static lighting, like a real knob under a fixed lamp) ----
    {
        const float scale = KNOB_R / 163.0f;
        const float sz = (float) knobImg.getWidth() * scale;
        g.drawImage (knobImg, juce::Rectangle<float> (sz, sz).withCentre ({ KX, KY }), juce::RectanglePlacement::stretchToFit);
    }

    // ---- indicator line (rotates) ----
    {
        const auto p0 = polar (KNOB_R - 46.0f, ang);
        const auto p1 = polar (KNOB_R - 10.0f, ang);
        juce::Line<float> ln (p0, p1);
        const auto lineCol = on ? amber : amber.withMultipliedBrightness (0.75f);
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (lineCol.withAlpha ((on ? 0.10f : 0.05f) * (5 - i)));
            g.drawLine (ln, 3.0f + i * 3.0f);
        }
        g.setColour (lineCol);
        g.drawLine (ln, 3.2f);
        g.setColour (amberHot.withAlpha (on ? 0.9f : 0.5f));
        g.drawLine (ln, 1.2f);
    }

    // ---- centre value window ----
    {
        const float cr = 40.0f;
        juce::ColourGradient grad (juce::Colour (0xff0b0a09), KX, KY - cr, juce::Colour (0xff1c1a18), KX, KY + cr, false);
        g.setGradientFill (grad);
        g.fillEllipse (KX - cr, KY - cr, cr * 2, cr * 2);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawEllipse (KX - cr, KY - cr, cr * 2, cr * 2, 1.6f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (KX - cr - 1.2f, KY - cr - 1.2f, cr * 2 + 2.4f, cr * 2 + 2.4f, 0.8f);

        const juce::String txt = on ? juce::String (juce::roundToInt (v * 100.0f)) + "%" : juce::String ("OFF");
        const auto box = juce::Rectangle<float> (cr * 2, cr).withCentre ({ KX, KY + 1.0f });
        g.setFont (mono (25.0f));
        g.setColour (amber.withAlpha (on ? 0.25f : 0.12f));
        for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
            g.drawText (txt, box.translated (o.x, o.y), juce::Justification::centred);
        g.setColour (on ? amber : amber.withMultipliedBrightness (0.6f));
        g.drawText (txt, box, juce::Justification::centred);
    }

    // ---- LOOP readout ----
    {
        const float ms = proc.engine.currentLoopMs.load();
        const juce::String txt = ms > 0.0f ? "LOOP  " + juce::String (juce::roundToInt (ms)) + " ms"
                                           : juce::String ("LOOP  --- ms");
        g.setFont (mono (21.0f));
        g.setColour (amber.withAlpha (0.18f));
        for (auto o : { juce::Point<float> (-1, 0), { 1, 0 }, { 0, -1 }, { 0, 1 } })
            g.drawText (txt, readout.translated (o.x, o.y), juce::Justification::centred);
        g.setColour (ms > 0.0f ? amber : amber.withMultipliedBrightness (0.55f));
        g.drawText (txt, readout, juce::Justification::centred);
    }
}
