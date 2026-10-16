#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <atomic>
#include <vector>

//==============================================================================
namespace ParamIDs
{
    inline constexpr const char* timeL          = "timeL";
    inline constexpr const char* timeR          = "timeR";
    inline constexpr const char* feedback       = "feedback";
    inline constexpr const char* pingPong       = "pingPong";
    inline constexpr const char* mix            = "mix";
    inline constexpr const char* pain           = "pain";
    inline constexpr const char* drive          = "drive";
    inline constexpr const char* hpf            = "hpf";
    inline constexpr const char* lpf            = "lpf";
    inline constexpr const char* chorusRate     = "chorusRate";
    inline constexpr const char* chorusDepth    = "chorusDepth";
    inline constexpr const char* chorusVoices   = "chorusVoices";
    inline constexpr const char* chorusMix      = "chorusMix";
    inline constexpr const char* flangerRate    = "flangerRate";
    inline constexpr const char* flangerDepth   = "flangerDepth";
    inline constexpr const char* flangerFb      = "flangerFeedback";
    inline constexpr const char* flangerMix     = "flangerMix";
    inline constexpr const char* output         = "output";
}

//==============================================================================
/** Simple circular buffer with linearly interpolated fractional reads.
    read(d) returns the sample written d samples ago (call read before write). */
class DelayBuffer
{
public:
    void prepare (int maxDelaySamples)
    {
        buffer.assign ((size_t) maxDelaySamples + 4, 0.0f);
        writeIndex = 0;
    }

    void clear()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    float read (float delaySamples) const noexcept
    {
        const int size = (int) buffer.size();
        if (size < 4)
            return 0.0f;

        delaySamples = juce::jlimit (1.0f, (float) (size - 2), delaySamples);

        float readPos = (float) writeIndex - delaySamples;
        if (readPos < 0.0f)
            readPos += (float) size;

        int i0 = (int) readPos;
        const float frac = readPos - (float) i0;
        if (i0 >= size)
            i0 -= size;

        int i1 = i0 + 1;
        if (i1 >= size)
            i1 = 0;

        return buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);
    }

    void write (float sample) noexcept
    {
        if (buffer.empty())
            return;

        buffer[(size_t) writeIndex] = sample;
        if (++writeIndex >= (int) buffer.size())
            writeIndex = 0;
    }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
};

//==============================================================================
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    struct ModParams
    {
        int   voices        = 3;
        float chorusDepth   = 0.0f;
        float chorusMix     = 0.0f;
        float flangerDepth  = 0.0f;
        float flangerFb     = 0.0f;
        float flangerMix    = 0.0f;
    };

    static constexpr int filterUpdateInterval = 32;
    static constexpr int maxChorusVoices = 4;

    float processFeedbackPath (int channel, float x, float driveAmount, float feedbackAmount);
    float processModulation (int channel, float x, const ModParams& mp);
    void  updateFilters (float hpfBase, float lpfBase, float pain);

    // Raw parameter pointers
    std::atomic<float>* timeLParam = nullptr;
    std::atomic<float>* timeRParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* pingPongParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* painParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* hpfParam = nullptr;
    std::atomic<float>* lpfParam = nullptr;
    std::atomic<float>* chorusRateParam = nullptr;
    std::atomic<float>* chorusDepthParam = nullptr;
    std::atomic<float>* chorusVoicesParam = nullptr;
    std::atomic<float>* chorusMixParam = nullptr;
    std::atomic<float>* flangerRateParam = nullptr;
    std::atomic<float>* flangerDepthParam = nullptr;
    std::atomic<float>* flangerFbParam = nullptr;
    std::atomic<float>* flangerMixParam = nullptr;
    std::atomic<float>* outputParam = nullptr;

    // Smoothed values
    using Smoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    Smoothed timeLSm, timeRSm, feedbackSm, mixSm, painSm, driveSm, hpfSm, lpfSm, outputSm;

    // DSP state
    DelayBuffer delayBuf[2];
    DelayBuffer chorusBuf[2];
    DelayBuffer flangerBuf[2];

    juce::dsp::StateVariableTPTFilter<float> hpFilter, lpFilter;

    double currentSampleRate = 44100.0;
    float  msToSamples = 44.1f;
    int    filterCountdown = 0;

    float painLfoPhase = 0.0f;
    float chorusPhase = 0.0f;
    float flangerPhase = 0.0f;
    float chorusRateHz = 0.8f;
    float flangerRateHz = 0.3f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
