#pragma once
#include "PluginProcessor.h"

class InvisibleKnobLnF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
};

class StuckEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StuckEditor (StuckProcessor&);
    ~StuckEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    StuckProcessor& proc;
    juce::Image panel, knobImg;
    InvisibleKnobLnF lnf;
    juce::Slider knob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    float shownLoopMs = -1.0f, shownValue = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckEditor)
};
