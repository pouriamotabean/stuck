#include "../../Source/PluginProcessor.h"
#include "../../Source/PluginEditor.h"
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    StuckProcessor p;
    p.prepareToPlay (48000, 512);
    float vals[] = { 0.0f, 0.37f, 0.9f };
    int i = 0;
    for (float v : vals)
    {
        p.apvts.getParameter ("stutter")->setValueNotifyingHost (v);
        // run audio so LOOP readout has a value
        juce::AudioBuffer<float> b (2, 512); b.clear(); juce::MidiBuffer m;
        for (int k = 0; k < 4; ++k) p.processBlock (b, m);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (841, 815);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (juce::String ("/home/claude/stuck/tests/snap/ui_") + juce::String (i++) + ".png");
        f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}
