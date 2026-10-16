#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    inline float wrapPhase (float p) noexcept
    {
        return p - std::floor (p);
    }

    // Soft saturator. drive 0..1 -> gain 1..15. Bounded to +/-1, which keeps the
    // feedback loop stable even when feedback is boosted by the PAIN macro.
    inline float saturate (float x, float drive) noexcept
    {
        const float gain = 1.0f + drive * 14.0f;
        return std::tanh (x * gain);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    using juce::AudioParameterFloat;
    using juce::AudioParameterFloatAttributes;
    using juce::NormalisableRange;
    using juce::ParameterID;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto msText   = [] (float v, int) { return juce::String (v, v < 100.0f ? 1 : 0) + " ms"; };
    auto pctText  = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto hzText   = [] (float v, int) { return juce::String (v, v < 1000.0f ? 0 : 0) + " Hz"; };
    auto rateText = [] (float v, int) { return juce::String (v, 2) + " Hz"; };
    auto dbText   = [] (float v, int) { return juce::String (v, 1) + " dB"; };

    auto addFloat = [&params] (const char* id, const juce::String& name, NormalisableRange<float> range,
                               float def, std::function<juce::String (float, int)> toText)
    {
        params.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText))));
    };

    // Delay engine
    addFloat (ParamIDs::timeL,    "Time L",     { 1.0f, 2000.0f, 0.01f, 0.4f }, 375.0f, msText);
    addFloat (ParamIDs::timeR,    "Time R",     { 1.0f, 2000.0f, 0.01f, 0.4f }, 500.0f, msText);
    addFloat (ParamIDs::feedback, "Feedback",   { 0.0f, 0.95f, 0.001f },        0.40f,  pctText);
    addFloat (ParamIDs::pingPong, "Ping-Pong",  { 0.0f, 1.0f, 0.001f },         0.50f,  pctText);
    addFloat (ParamIDs::mix,      "Mix",        { 0.0f, 1.0f, 0.001f },         0.35f,  pctText);

    // PAIN macro + saturation
    addFloat (ParamIDs::pain,     "PAIN",       { 0.0f, 1.0f, 0.001f },         0.0f,   pctText);
    addFloat (ParamIDs::drive,    "Drive",      { 0.0f, 1.0f, 0.001f },         0.10f,  pctText);

    // Feedback-loop filters
    addFloat (ParamIDs::hpf,      "HP Filter",  { 20.0f, 2000.0f, 0.1f, 0.35f }, 20.0f,    hzText);
    addFloat (ParamIDs::lpf,      "LP Filter",  { 200.0f, 20000.0f, 0.1f, 0.35f }, 12000.0f, hzText);

    // Chorus
    addFloat (ParamIDs::chorusRate,  "Chorus Rate",  { 0.05f, 8.0f, 0.01f, 0.5f }, 0.8f, rateText);
    addFloat (ParamIDs::chorusDepth, "Chorus Depth", { 0.0f, 1.0f, 0.001f },       0.4f, pctText);
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        ParameterID { ParamIDs::chorusVoices, 1 }, "Chorus Voices", 1, maxChorusVoices, 3));
    addFloat (ParamIDs::chorusMix,   "Chorus Mix",   { 0.0f, 1.0f, 0.001f },       0.0f, pctText);

    // Flanger
    addFloat (ParamIDs::flangerRate,  "Flanger Rate",     { 0.05f, 5.0f, 0.01f, 0.5f }, 0.3f,  rateText);
    addFloat (ParamIDs::flangerDepth, "Flanger Depth",    { 0.0f, 1.0f, 0.001f },       0.5f,  pctText);
    addFloat (ParamIDs::flangerFb,    "Flanger Feedback", { -0.95f, 0.95f, 0.001f },    0.4f,  pctText);
    addFloat (ParamIDs::flangerMix,   "Flanger Mix",      { 0.0f, 1.0f, 0.001f },       0.0f,  pctText);

    // Output
    addFloat (ParamIDs::output,   "Output",     { -24.0f, 12.0f, 0.1f }, 0.0f, dbText);

    return { params.begin(), params.end() };
}

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ALMIGHTY_PAIN_STATE", createParameterLayout())
{
    timeLParam        = apvts.getRawParameterValue (ParamIDs::timeL);
    timeRParam        = apvts.getRawParameterValue (ParamIDs::timeR);
    feedbackParam     = apvts.getRawParameterValue (ParamIDs::feedback);
    pingPongParam     = apvts.getRawParameterValue (ParamIDs::pingPong);
    mixParam          = apvts.getRawParameterValue (ParamIDs::mix);
    painParam         = apvts.getRawParameterValue (ParamIDs::pain);
    driveParam        = apvts.getRawParameterValue (ParamIDs::drive);
    hpfParam          = apvts.getRawParameterValue (ParamIDs::hpf);
    lpfParam          = apvts.getRawParameterValue (ParamIDs::lpf);
    chorusRateParam   = apvts.getRawParameterValue (ParamIDs::chorusRate);
    chorusDepthParam  = apvts.getRawParameterValue (ParamIDs::chorusDepth);
    chorusVoicesParam = apvts.getRawParameterValue (ParamIDs::chorusVoices);
    chorusMixParam    = apvts.getRawParameterValue (ParamIDs::chorusMix);
    flangerRateParam  = apvts.getRawParameterValue (ParamIDs::flangerRate);
    flangerDepthParam = apvts.getRawParameterValue (ParamIDs::flangerDepth);
    flangerFbParam    = apvts.getRawParameterValue (ParamIDs::flangerFb);
    flangerMixParam   = apvts.getRawParameterValue (ParamIDs::flangerMix);
    outputParam       = apvts.getRawParameterValue (ParamIDs::output);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    msToSamples = (float) (sampleRate * 0.001);

    const int maxDelay  = (int) std::ceil (sampleRate * 2.0) + 8;
    const int maxModDly = (int) std::ceil (sampleRate * 0.05) + 8;

    for (int ch = 0; ch < 2; ++ch)
    {
        delayBuf[ch].prepare (maxDelay);
        chorusBuf[ch].prepare (maxModDly);
        flangerBuf[ch].prepare (maxModDly);
    }

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) juce::jmax (1, samplesPerBlock);
    spec.numChannels = 2;

    hpFilter.prepare (spec);
    lpFilter.prepare (spec);
    hpFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    lpFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    hpFilter.reset();
    lpFilter.reset();

    auto initSmoothed = [sampleRate] (Smoothed& s, float seconds, float value)
    {
        s.reset (sampleRate, (double) seconds);
        s.setCurrentAndTargetValue (value);
    };

    initSmoothed (timeLSm,    0.08f, timeLParam->load());
    initSmoothed (timeRSm,    0.08f, timeRParam->load());
    initSmoothed (feedbackSm, 0.03f, feedbackParam->load());
    initSmoothed (mixSm,      0.03f, mixParam->load());
    initSmoothed (painSm,     0.05f, painParam->load());
    initSmoothed (driveSm,    0.03f, driveParam->load());
    initSmoothed (hpfSm,      0.03f, hpfParam->load());
    initSmoothed (lpfSm,      0.03f, lpfParam->load());
    initSmoothed (outputSm,   0.03f, juce::Decibels::decibelsToGain (outputParam->load()));

    painLfoPhase = 0.0f;
    chorusPhase = 0.0f;
    flangerPhase = 0.0f;
    filterCountdown = 0;
}

void PluginProcessor::releaseResources()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        delayBuf[ch].clear();
        chorusBuf[ch].clear();
        flangerBuf[ch].clear();
    }
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() && layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void PluginProcessor::updateFilters (float hpfBase, float lpfBase, float pain)
{
    // PAIN-driven filter movement: an internal LFO whose speed and sweep range both grow with PAIN.
    const float rateHz = 0.25f + pain * 5.0f;
    painLfoPhase = wrapPhase (painLfoPhase + rateHz * (float) filterUpdateInterval / (float) currentSampleRate);
    const float lfo = std::sin (kTwoPi * painLfoPhase);

    const float nyquistLimit = 0.45f * (float) currentSampleRate;

    float lp = lpfBase * std::exp2 (pain * (1.5f * lfo - 0.5f));
    float hp = hpfBase * std::exp2 (pain * 2.5f * (1.0f + lfo));

    lp = juce::jlimit (100.0f, juce::jmin (20000.0f, nyquistLimit), lp);
    hp = juce::jlimit (20.0f,  juce::jmin (12000.0f, nyquistLimit), hp);
    hp = juce::jmin (hp, lp * 0.8f);

    hpFilter.setCutoffFrequency (hp);
    lpFilter.setCutoffFrequency (lp);
}

float PluginProcessor::processFeedbackPath (int channel, float x, float driveAmount, float feedbackAmount)
{
    x = hpFilter.processSample (channel, x);
    x = lpFilter.processSample (channel, x);
    x = saturate (x, driveAmount);
    return x * feedbackAmount;
}

float PluginProcessor::processModulation (int channel, float x, const ModParams& mp)
{
    // ---- Multi-voice chorus ----
    float chorusOut = x;
    {
        const float channelOffset = (channel == 1) ? 0.25f : 0.0f;
        float sum = 0.0f;

        for (int v = 0; v < mp.voices; ++v)
        {
            const float phase = wrapPhase (chorusPhase + (float) v / (float) mp.voices + channelOffset);
            const float lfo = std::sin (kTwoPi * phase);
            const float delayMs = (10.0f + 4.0f * (float) v) + mp.chorusDepth * 6.0f * lfo;
            sum += chorusBuf[channel].read (delayMs * msToSamples);
        }

        chorusBuf[channel].write (x);
        const float wetChorus = sum / (float) mp.voices;
        chorusOut = x + mp.chorusMix * (wetChorus - x);
    }

    // ---- Flanger ----
    float out = chorusOut;
    {
        const float channelOffset = (channel == 1) ? 0.25f : 0.0f;
        const float lfo = 0.5f * (1.0f + std::sin (kTwoPi * wrapPhase (flangerPhase + channelOffset)));
        const float delayMs = 0.4f + lfo * mp.flangerDepth * 7.0f;

        const float fl = flangerBuf[channel].read (delayMs * msToSamples);
        flangerBuf[channel].write (juce::jlimit (-4.0f, 4.0f, chorusOut + fl * mp.flangerFb));
        out = chorusOut + mp.flangerMix * (fl - chorusOut);
    }

    return out;
}

//==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numChannels < 1 || numSamples < 1 || currentSampleRate <= 0.0)
        return;

    // Block-rate parameter reads
    timeLSm.setTargetValue    (timeLParam->load());
    timeRSm.setTargetValue    (timeRParam->load());
    feedbackSm.setTargetValue (feedbackParam->load());
    mixSm.setTargetValue      (mixParam->load());
    painSm.setTargetValue     (painParam->load());
    driveSm.setTargetValue    (driveParam->load());
    hpfSm.setTargetValue      (hpfParam->load());
    lpfSm.setTargetValue      (lpfParam->load());
    outputSm.setTargetValue   (juce::Decibels::decibelsToGain (outputParam->load()));

    const float pingPong = pingPongParam->load();
    chorusRateHz  = chorusRateParam->load();
    flangerRateHz = flangerRateParam->load();

    ModParams mp;
    mp.voices       = juce::jlimit (1, maxChorusVoices, juce::roundToInt (chorusVoicesParam->load()));
    mp.chorusDepth  = chorusDepthParam->load();
    mp.chorusMix    = chorusMixParam->load();
    mp.flangerDepth = flangerDepthParam->load();
    mp.flangerFb    = juce::jlimit (-0.95f, 0.95f, flangerFbParam->load());
    mp.flangerMix   = flangerMixParam->load();

    float* left  = buffer.getWritePointer (0);
    float* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    const float chorusInc  = chorusRateHz  / (float) currentSampleRate;
    const float flangerInc = flangerRateHz / (float) currentSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const float pain    = painSm.getNextValue();
        const float drive   = juce::jlimit (0.0f, 1.0f, driveSm.getNextValue() + pain * 0.7f);
        const float fbAmt   = juce::jmin (0.97f, feedbackSm.getNextValue() + pain * 0.25f);
        const float hpfBase = hpfSm.getNextValue();
        const float lpfBase = lpfSm.getNextValue();
        const float mix     = mixSm.getNextValue();
        const float outGain = outputSm.getNextValue();
        const float timeLSamples = juce::jmax (1.0f, timeLSm.getNextValue() * msToSamples);
        const float timeRSamples = juce::jmax (1.0f, timeRSm.getNextValue() * msToSamples);

        if (--filterCountdown <= 0)
        {
            filterCountdown = filterUpdateInterval;
            updateFilters (hpfBase, lpfBase, pain);
        }

        const float dryL = left[i];
        const float dryR = right != nullptr ? right[i] : dryL;

        // Ping-pong input routing: at 100% the (mono) input only feeds the left delay,
        // so repeats bounce L -> R -> L ...
        const float mono   = 0.5f * (dryL + dryR);
        const float inputL = dryL + pingPong * (mono - dryL);
        const float inputR = dryR * (1.0f - pingPong);

        // Delay taps
        const float tapL = delayBuf[0].read (timeLSamples);
        const float tapR = delayBuf[1].read (timeRSamples);

        // Cross-feedback (0 = each side feeds itself, 1 = fully crossed)
        const float fbSrcL = (1.0f - pingPong) * tapL + pingPong * tapR;
        const float fbSrcR = (1.0f - pingPong) * tapR + pingPong * tapL;

        // Filters + saturation + feedback gain inside the loop
        const float fbL = processFeedbackPath (0, fbSrcL, drive, fbAmt);
        const float fbR = processFeedbackPath (1, fbSrcR, drive, fbAmt);

        delayBuf[0].write (inputL + fbL);
        delayBuf[1].write (inputR + fbR);

        // Modulation on the wet signal
        const float wetL = processModulation (0, tapL, mp);
        const float wetR = processModulation (1, tapR, mp);

        chorusPhase  = wrapPhase (chorusPhase  + chorusInc);
        flangerPhase = wrapPhase (flangerPhase + flangerInc);

        float outL = (dryL * (1.0f - mix) + wetL * mix) * outGain;
        float outR = (dryR * (1.0f - mix) + wetR * mix) * outGain;

        if (! std::isfinite (outL)) outL = 0.0f;
        if (! std::isfinite (outR)) outR = 0.0f;

        left[i] = juce::jlimit (-16.0f, 16.0f, outL);
        if (right != nullptr)
            right[i] = juce::jlimit (-16.0f, 16.0f, outR);
    }
}

//==============================================================================
juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
