#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stutter", 1 }, "Stutter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                const int pct = juce::roundToInt (v * 100.0f);
                return pct <= 0 ? juce::String ("OFF") : juce::String (pct) + "%";
            })
            .withValueFromStringFunction ([] (const juce::String& s)
            {
                return s.trim().equalsIgnoreCase ("off") ? 0.0f
                                                         : juce::jlimit (0.0f, 1.0f, s.getFloatValue() / 100.0f);
            })));
    return l;
}

StuckProcessor::StuckProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", makeLayout())
{
    stutterParam = apvts.getRawParameterValue ("stutter");
}

bool StuckProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == out;
}

void StuckProcessor::prepareToPlay (double sr, int block)
{
    engine.prepare (sr, block);
    lastKnob = stutterParam->load();
}

void StuckProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 1.0) lastBpm = *bpm;

    const float knob = stutterParam->load();
    engine.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(),
                    buffer.getNumSamples(), lastKnob, knob, lastBpm);
    lastKnob = knob;
}

void StuckProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void StuckProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* StuckProcessor::createEditor() { return new StuckEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StuckProcessor(); }
