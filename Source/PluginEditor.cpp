#include "PluginEditor.h"

//==============================================================================
PainLookAndFeel::PainLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
}

void PainLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    const float arcRadius = radius * 0.92f;
    const float strokeW = juce::jmax (3.0f, radius * 0.09f);

    // Track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2a30));
    g.strokePath (track, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
    g.setColour (accent().withMultipliedBrightness (slider.isEnabled() ? 1.0f : 0.4f));
    g.strokePath (value, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body
    const float bodyRadius = radius * 0.74f;
    juce::ColourGradient body (juce::Colour (0xff3a3a42), centre.x, centre.y - bodyRadius,
                               juce::Colour (0xff101014), centre.x, centre.y + bodyRadius, false);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.5f);

    // Pointer
    juce::Path pointer;
    const float pw = juce::jmax (2.5f, bodyRadius * 0.08f);
    pointer.addRoundedRectangle (-pw * 0.5f, -bodyRadius * 0.92f, pw, bodyRadius * 0.45f, pw * 0.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    painKnob = addKnob (ParamIDs::pain, "PAIN");
    painKnob->label.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    painKnob->label.setColour (juce::Label::textColourId, PainLookAndFeel::accent());
    painKnob->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 24);

    rows.push_back ({ "DELAY", {
        addKnob (ParamIDs::timeL,    "TIME L"),
        addKnob (ParamIDs::timeR,    "TIME R"),
        addKnob (ParamIDs::feedback, "FEEDBACK"),
        addKnob (ParamIDs::pingPong, "PING-PONG"),
        addKnob (ParamIDs::mix,      "MIX") }, {} });

    rows.push_back ({ "FEEDBACK FILTERS + SATURATION", {
        addKnob (ParamIDs::hpf,    "HIGH-PASS"),
        addKnob (ParamIDs::lpf,    "LOW-PASS"),
        addKnob (ParamIDs::drive,  "DRIVE"),
        addKnob (ParamIDs::output, "OUTPUT") }, {} });

    rows.push_back ({ "CHORUS", {
        addKnob (ParamIDs::chorusRate,   "RATE"),
        addKnob (ParamIDs::chorusDepth,  "DEPTH"),
        addKnob (ParamIDs::chorusVoices, "VOICES"),
        addKnob (ParamIDs::chorusMix,    "MIX") }, {} });

    rows.push_back ({ "FLANGER", {
        addKnob (ParamIDs::flangerRate,  "RATE"),
        addKnob (ParamIDs::flangerDepth, "DEPTH"),
        addKnob (ParamIDs::flangerFb,    "FEEDBACK"),
        addKnob (ParamIDs::flangerMix,   "MIX") }, {} });

    setSize (900, 610);
    setResizable (false, false);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

PluginEditor::Knob* PluginEditor::addKnob (const juce::String& paramID, const juce::String& labelText)
{
    auto k = std::make_unique<Knob>();

    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 18);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);

    k->label.setText (labelText, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramID, k->slider);

    if (auto* param = audioProcessor.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    auto* raw = k.get();
    knobs.push_back (std::move (k));
    return raw;
}

//==============================================================================
void PluginEditor::paint (juce::Graphics& g)
{
    const auto accent = PainLookAndFeel::accent();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a0a0c), 0.0f, 0.0f,
                                             juce::Colour (0xff200709), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Title
    g.setColour (accent);
    g.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
    g.drawText ("ALMIGHTY PAIN", 20, 12, 460, 44, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText ("ALMIGHTY AUDIO", getWidth() - 320, 12, 300, 44, juce::Justification::centredRight);

    // PAIN macro panel
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (painPanel.toFloat(), 14.0f);
    g.setColour (accent.withAlpha (0.55f));
    g.drawRoundedRectangle (painPanel.toFloat(), 14.0f, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText ("MACRO ENGINE", painPanel.withTrimmedTop (painPanel.getHeight() - 30),
                juce::Justification::centred);

    // Row headers
    for (const auto& row : rows)
    {
        g.setColour (accent);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (row.title, row.bounds.getX(), row.bounds.getY(), row.bounds.getWidth(), 18,
                    juce::Justification::centredLeft);

        g.setColour (accent.withAlpha (0.3f));
        g.fillRect (row.bounds.getX(), row.bounds.getY() + 20, row.bounds.getWidth(), 1);
    }
}

void PluginEditor::resized()
{
    const int leftW = 260;
    const int top = 72;
    const int rowH = 132;
    const int margin = 20;

    // PAIN panel
    painPanel = { margin, top + 10, leftW - 2 * margin, 4 * rowH - 20 };
    {
        auto area = painPanel.reduced (10, 16);
        area.removeFromBottom (24);
        painKnob->label.setBounds (area.removeFromTop (34));
        painKnob->slider.setBounds (area);
    }

    // Parameter rows
    const int rx = leftW;
    const int rw = getWidth() - leftW - margin;
    const int cellW = rw / 5;

    for (size_t r = 0; r < rows.size(); ++r)
    {
        auto& row = rows[r];
        row.bounds = { rx, top + (int) r * rowH, rw, rowH };

        for (size_t k = 0; k < row.knobs.size(); ++k)
        {
            juce::Rectangle<int> cell (rx + (int) k * cellW, row.bounds.getY() + 24, cellW, rowH - 28);
            row.knobs[k]->label.setBounds (cell.removeFromTop (16));
            row.knobs[k]->slider.setBounds (cell);
        }
    }
}
