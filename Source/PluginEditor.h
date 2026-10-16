#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class AlmightyPainAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AlmightyPainAudioProcessorEditor (AlmightyPainAudioProcessor&);
    ~AlmightyPainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AlmightyPainAudioProcessor& processorRef;
    
    // Helper method for shifting colors based on the macro knob intensity
    juce::Colour getDynamicColor(float intensity);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlmightyPainAudioProcessorEditor)
};
