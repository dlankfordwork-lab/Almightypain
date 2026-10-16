
#include "PluginProcessor.h"
#include "PluginEditor.h"

AlmightyPainAudioProcessorEditor::AlmightyPainAudioProcessorEditor (AlmightyPainAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (800, 600);
}

AlmightyPainAudioProcessorEditor::~AlmightyPainAudioProcessorEditor() = default;

juce::Colour AlmightyPainAudioProcessorEditor::getDynamicColor(float intensity)
{
    // Shifts smoothly from dark red into neon purple/cyan as the macro knob turns up
    return juce::Colour::fromHSV(intensity * 0.7f, 0.85f, 0.95f, 1.0f);
}

void AlmightyPainAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Safely grab the current value of the PAIN macro parameter (0.0 to 1.0)
    float macroValue = 0.0f;
    if (auto* param = processorRef.apvts.getParameter("PAIN"))
        macroValue = param->getValue();

    juce::Colour dynamicAccent = getDynamicColor(macroValue);

    // Fill background with a dark gradient tinted by the dynamic color
    juce::ColourGradient bgGradient(juce::Colours::black, 0, 0, 
                                   dynamicAccent.darker(0.9f), 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Draw Title Text safely using integer bounds to prevent compile errors
    g.setColour(dynamicAccent);
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    
    auto titleBounds = getLocalBounds().removeFromTop(60).toNearestInt();
    g.drawText("ALMIGHTY PAIN", titleBounds, juce::Justification::centred, true);
}

void AlmightyPainAudioProcessorEditor::resized()
{
    // Place your layout code here
}
