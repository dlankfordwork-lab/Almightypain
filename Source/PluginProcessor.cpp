#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AlmightyPainAudioProcessor::AlmightyPainAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #ifndef JucePlugin_IsMidiEffect
                      #ifndef JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

AlmightyPainAudioProcessor::~AlmightyPainAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AlmightyPainAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Pain Macro Parameter
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("PAIN", 1),
        "Pain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));

    return layout;
}

//==============================================================================
const juce::String AlmightyPainAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AlmightyPainAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AlmightyPainAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AlmightyPainAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AlmightyPainAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AlmightyPainAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some DAWs get confused if you report 0
}

int AlmightyPainAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AlmightyPainAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AlmightyPainAudioProcessor::getProgramName (int index)
{
    return {};
}

void AlmightyPainAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AlmightyPainAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback initialisation
}

void AlmightyPainAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as a place to free up any spare memory
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AlmightyPainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AlmightyPainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Audio DSP processing loop goes here
}

//==============================================================================
bool AlmightyPainAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AlmightyPainAudioProcessor::createEditor()
{
    return new AlmightyPainAudioProcessorEditor (*this);
}

//==============================================================================
void AlmightyPainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AlmightyPainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AlmightyPainAudioProcessor();
}
