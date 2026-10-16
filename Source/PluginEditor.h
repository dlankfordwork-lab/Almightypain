#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

#include "PluginProcessor.h"

//==============================================================================
class PainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PainLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    static juce::Colour accent() { return juce::Colour (0xffe0182d); }
};

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Row
    {
        juce::String title;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    Knob* addKnob (const juce::String& paramID, const juce::String& labelText);

    PluginProcessor& audioProcessor;

    PainLookAndFeel lookAndFeel;   // declared before the knobs so it outlives them
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Row> rows;
    Knob* painKnob = nullptr;
    juce::Rectangle<int> painPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
