juce::AudioProcessorEditor* AlmightyPainAudioProcessor::createEditor()
{
    return new AlmightyPainAudioProcessorEditor (*this);
}
