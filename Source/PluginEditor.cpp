#include "PluginProcessor.h"
#include "PluginEditor.h"

// Hardware-style LookAndFeel
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HardwareLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff8888ff));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff8888ff));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a1a2e));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 8.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // 1. Outer metal ring (bezel)
        juce::ColourGradient outerGradient(
            juce::Colour(0xff4a4a4a), centreX - radius, centreY - radius,
            juce::Colour(0xff1a1a1a), centreX + radius, centreY + radius,
            true);
        g.setGradientFill(outerGradient);
        g.fillEllipse(rx - 6, ry - 6, rw + 12, rw + 12);
        
        // Inner bevel
        g.setColour(juce::Colour(0xff000000));
        g.drawEllipse(rx - 6, ry - 6, rw + 12, rw + 12, 2);
        g.setColour(juce::Colour(0xff666666));
        g.drawEllipse(rx - 4, ry - 4, rw + 8, rw + 8, 1);

        // 2. Knob body (brushed metal look)
        juce::ColourGradient knobGradient(
            juce::Colour(0xff3a3a3a), centreX, centreY - radius,
            juce::Colour(0xff1a1a1a), centreX, centreY + radius,
            false);
        g.setGradientFill(knobGradient);
        g.fillEllipse(rx, ry, rw, rw);

        // Add circular brush texture lines
        g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));
        for (int i = 0; i < 36; i++)
        {
            float a = i * 10.0f * (juce::MathConstants<float>::pi / 180.0f);
            float x1 = centreX + (radius * 0.7f) * std::cos(a);
            float y1 = centreY + (radius * 0.7f) * std::sin(a);
            float x2 = centreX + (radius * 0.95f) * std::cos(a);
            float y2 = centreY + (radius * 0.95f) * std::sin(a);
            g.drawLine(x1, y1, x2, y2, 0.5f);
        }

        // 3. Center cap (darker metal)
        float capSize = radius * 0.4f;
        juce::ColourGradient capGradient(
            juce::Colour(0xff2a2a2a), centreX - capSize, centreY - capSize,
            juce::Colour(0xff0a0a0a), centreX + capSize, centreY + capSize,
            true);
        g.setGradientFill(capGradient);
        g.fillEllipse(centreX - capSize, centreY - capSize, capSize * 2, capSize * 2);
        
        // Cap edge
        g.setColour(juce::Colour(0xff000000));
        g.drawEllipse(centreX - capSize, centreY - capSize, capSize * 2, capSize * 2, 1);

        // 4. Position indicator (white line)
        juce::Path p;
        float pointerLength = radius * 0.8f;
        float pointerThickness = 3.0f;
        p.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.4f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        
        // White indicator with slight glow
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.strokePath(p, juce::PathStrokeType(5.0f));
        g.setColour(juce::Colours::white);
        g.fillPath(p);

        // 5. Position dots around knob
        g.setColour(juce::Colour(0xff666666));
        int numDots = 11;
        for (int i = 0; i < numDots; ++i)
        {
            float dotAngle = rotaryStartAngle + (i / (float)(numDots - 1)) * (rotaryEndAngle - rotaryStartAngle);
            float dotX = centreX + (radius + 10) * std::cos(dotAngle);
            float dotY = centreY + (radius + 10) * std::sin(dotAngle);
            g.fillEllipse(dotX - 1.5f, dotY - 1.5f, 3.0f, 3.0f);
        }
    }
    
    juce::Font getLabelFont(juce::Label&) override
    {
        return juce::Font(11.0f);
    }
};

// Helper function to draw a screw
void drawScrew(juce::Graphics& g, float x, float y, float size)
{
    // Screw head
    juce::ColourGradient screwGradient(
        juce::Colour(0xff5a5a5a), x - size/2, y - size/2,
        juce::Colour(0xff2a2a2a), x + size/2, y + size/2,
        true);
    g.setGradientFill(screwGradient);
    g.fillEllipse(x - size/2, y - size/2, size, size);
    
    // Edge
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawEllipse(x - size/2, y - size/2, size, size, 1);
    
    // Phillips cross
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(x - size*0.3f, y, x + size*0.3f, y, 1.5f);
    g.drawLine(x, y - size*0.3f, x, y + size*0.3f, 1.5f);
}

// Main Component
class KnobComponent : public juce::Component
{
public:
    KnobComponent(BuildUpVerbAudioProcessor& p) : processor(p)
    {
        // Apply custom look and feel
        setLookAndFeel(&hardwareLookAndFeel);

        // Main Build Up knob (larger)
        setupKnob(buildUpKnob, buildUpLabel, "BUILD UP", 0.0, 100.0, 0.01, "%", 11.0f);
        
        // Secondary knobs
        setupKnob(filterKnob, filterLabel, "FILTER", 0.0, 100.0, 0.01, "%", 10.0f);
        setupKnob(reverbKnob, reverbLabel, "REVERB", 0.0, 100.0, 0.01, "%", 10.0f);
        setupKnob(noiseKnob, noiseLabel, "NOISE", 0.0, 100.0, 0.01, "%", 10.0f);
        
        // Additional knobs
        setupKnob(tremoloRateKnob, tremoloRateLabel, "TREM RATE", 0.1, 20.0, 0.01, "Hz", 9.0f);
        setupKnob(tremoloDepthKnob, tremoloDepthLabel, "TREM DEPTH", 0.0, 100.0, 0.01, "%", 9.0f);
        setupKnob(riserKnob, riserLabel, "RISER", 0.0, 100.0, 0.01, "%", 10.0f);
        setupKnob(riserReleaseKnob, riserReleaseLabel, "RELEASE", 0.01, 5.0, 0.01, "s", 9.0f);
        
        // New controls
        setupKnob(resonanceKnob, resonanceLabel, "RESONANCE", 0.5, 10.0, 0.01, "", 9.0f);
        setupKnob(widthKnob, widthLabel, "WIDTH", 0.0, 200.0, 0.01, "%", 9.0f);
        setupKnob(smartPanKnob, smartPanLabel, "SMART PAN", 0.0, 100.0, 0.01, "%", 9.0f);
        setupKnob(gateKnob, gateLabel, "THRESHOLD", 0.0, 100.0, 0.01, "%", 9.0f);
        setupKnob(vocoderReleaseKnob, vocoderReleaseLabel, "VOC RELEASE", 0.0, 100.0, 0.01, "%", 9.0f);
        setupKnob(vocoderBrightnessKnob, vocoderBrightnessLabel, "BRIGHTNESS", 0.0, 100.0, 0.01, "%", 9.0f);
        setupKnob(driveKnob, driveLabel, "DRIVE", 0.0, 100.0, 0.01, "%", 9.0f);
        
        // Delay controls
        setupKnob(delayMixKnob, delayMixLabel, "DELAY MIX", 0.0, 100.0, 0.01, "%", 9.0f);
        setupKnob(delayFeedbackKnob, delayFeedbackLabel, "FEEDBACK", 0.0, 90.0, 0.01, "%", 9.0f);
        
        // Delay time selector
        delayTimeCombo.addItem("1/2", 1);
        delayTimeCombo.addItem("1/3", 2);
        delayTimeCombo.addItem("1/4", 3);
        delayTimeCombo.addItem("1/8", 4);
        delayTimeCombo.addItem("1/16", 5);
        delayTimeCombo.setSelectedId(3); // Default to 1/4
        delayTimeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        delayTimeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
        delayTimeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        delayTimeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));
        delayTimeCombo.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(delayTimeCombo);
        
        delayTimeLabel.setText("TIME", juce::dontSendNotification);
        delayTimeLabel.setJustificationType(juce::Justification::centred);
        delayTimeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        delayTimeLabel.setFont(juce::Font(9.0f));
        addAndMakeVisible(delayTimeLabel);
        
        // Vocoder label instead of combo box
        vocoderLabel.setText("4-BAND VOCODER", juce::dontSendNotification);
        vocoderLabel.setJustificationType(juce::Justification::centred);
        vocoderLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        vocoderLabel.setFont(juce::Font(12.0f, juce::Font::bold));
        addAndMakeVisible(vocoderLabel);
        
        // Riser type selector
        riserTypeCombo.addItem("Sine", 1);
        riserTypeCombo.addItem("Saw", 2);
        riserTypeCombo.addItem("Square", 3);
        riserTypeCombo.addItem("Noise Sweep", 4);
        riserTypeCombo.addItem("Sub Drop", 5);
        riserTypeCombo.setSelectedId(1);
        riserTypeCombo.onChange = [this] { riserTypeChanged(); };
        riserTypeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        riserTypeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
        riserTypeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        riserTypeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));
        riserTypeCombo.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(riserTypeCombo);
        
        riserTypeLabel.setText("TYPE", juce::dontSendNotification);
        riserTypeLabel.setJustificationType(juce::Justification::centred);
        riserTypeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        riserTypeLabel.setFont(juce::Font(9.0f));
        addAndMakeVisible(riserTypeLabel);
        
        // Auto gain button
        autoGainButton.setButtonText("AUTO GAIN");
        autoGainButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.9f));
        autoGainButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00ff00));
        autoGainButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff666666));
        autoGainButton.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(autoGainButton);
        
        // Macro mode selector
        macroModeCombo.addItem("Macro Off", 1);
        macroModeCombo.addItem("Subtle", 2);
        macroModeCombo.addItem("Aggressive", 3);
        macroModeCombo.addItem("Epic", 4);
        macroModeCombo.addItem("Custom", 5);
        macroModeCombo.setSelectedId(1);
        macroModeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        macroModeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
        macroModeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        macroModeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));
        macroModeCombo.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(macroModeCombo);
        
        macroLabel.setText("MACRO", juce::dontSendNotification);
        macroLabel.setJustificationType(juce::Justification::centredLeft);
        macroLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        macroLabel.setFont(juce::Font(10.0f));
        addAndMakeVisible(macroLabel);
        
        // Filter type selector
        filterTypeCombo.addItem("High Pass", 1);
        filterTypeCombo.addItem("Low Pass", 2);
        filterTypeCombo.addItem("Dual Sweep", 3);
        filterTypeCombo.setSelectedId(1);
        filterTypeCombo.onChange = [this] { filterTypeChanged(); };
        filterTypeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        filterTypeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
        filterTypeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        filterTypeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));
        filterTypeCombo.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(filterTypeCombo);
        
        filterTypeLabel.setText("TYPE", juce::dontSendNotification);
        filterTypeLabel.setJustificationType(juce::Justification::centred);
        filterTypeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        filterTypeLabel.setFont(juce::Font(9.0f));
        addAndMakeVisible(filterTypeLabel);
        
        // Filter slope selector
        filterSlopeCombo.addItem("6 dB/oct", 1);
        filterSlopeCombo.addItem("12 dB/oct", 2);
        filterSlopeCombo.addItem("18 dB/oct", 3);
        filterSlopeCombo.addItem("24 dB/oct", 4);
        filterSlopeCombo.setSelectedId(2); // Default to 12 dB/oct
        filterSlopeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        filterSlopeCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
        filterSlopeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        filterSlopeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));
        filterSlopeCombo.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(filterSlopeCombo);
        
        filterSlopeLabel.setText("SLOPE", juce::dontSendNotification);
        filterSlopeLabel.setJustificationType(juce::Justification::centred);
        filterSlopeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        filterSlopeLabel.setFont(juce::Font(9.0f));
        addAndMakeVisible(filterSlopeLabel);
        
        // Section labels
        filterSectionLabel.setText("FILTER", juce::dontSendNotification);
        filterSectionLabel.setJustificationType(juce::Justification::centred);
        filterSectionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        filterSectionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(filterSectionLabel);
        
        tremoloSectionLabel.setText("TREMOLO", juce::dontSendNotification);
        tremoloSectionLabel.setJustificationType(juce::Justification::centred);
        tremoloSectionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        tremoloSectionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(tremoloSectionLabel);
        
        noiseSectionLabel.setText("NOISE", juce::dontSendNotification);
        noiseSectionLabel.setJustificationType(juce::Justification::centred);
        noiseSectionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        noiseSectionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(noiseSectionLabel);
        
        riserSectionLabel.setText("RISER", juce::dontSendNotification);
        riserSectionLabel.setJustificationType(juce::Justification::centred);
        riserSectionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        riserSectionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(riserSectionLabel);
        
        delaySectionLabel.setText("DELAY", juce::dontSendNotification);
        delaySectionLabel.setJustificationType(juce::Justification::centred);
        delaySectionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        delaySectionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(delaySectionLabel);
        
        // Attach to parameters
        buildUpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "buildup", buildUpKnob);
        filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "filterIntensity", filterKnob);
        filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "filterType", filterTypeCombo);
        reverbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "reverbMix", reverbKnob);
        noiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "noiseAmount", noiseKnob);
        tremoloRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "tremoloRate", tremoloRateKnob);
        tremoloDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "tremoloDepth", tremoloDepthKnob);
        riserAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "riserAmount", riserKnob);
        riserReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "riserRelease", riserReleaseKnob);
        // No noise type attachment needed anymore
        riserTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "riserType", riserTypeCombo);
        resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "filterResonance", resonanceKnob);
        widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "stereoWidth", widthKnob);
        smartPanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "smartPan", smartPanKnob);
        gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "noiseGate", gateKnob);
        vocoderReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "vocoderRelease", vocoderReleaseKnob);
        vocoderBrightnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "vocoderBrightness", vocoderBrightnessKnob);
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "filterDrive", driveKnob);
        delayMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "delayMix", delayMixKnob);
        delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, "delayFeedback", delayFeedbackKnob);
        delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "delayTime", delayTimeCombo);
        filterSlopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "filterSlope", filterSlopeCombo);
        autoGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.parameters, "autoGain", autoGainButton);
        macroModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.parameters, "macroMode", macroModeCombo);
        
        // Preset controls
        presetCombo.addItem("-- Select Preset --", 1);
        for (int i = 0; i < processor.getNumPrograms(); ++i)
        {
            presetCombo.addItem(processor.getProgramName(i), i + 2);
        }
        presetCombo.setSelectedId(1);
        presetCombo.onChange = [this] { presetChanged(); };
        presetCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        presetCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
        presetCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
        presetCombo.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));
        presetCombo.setLookAndFeel(&hardwareLookAndFeel);
        addAndMakeVisible(presetCombo);
        
        presetLabel.setText("PRESET", juce::dontSendNotification);
        presetLabel.setJustificationType(juce::Justification::centred);
        presetLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        presetLabel.setFont(juce::Font(10.0f, juce::Font::bold));
        addAndMakeVisible(presetLabel);
        
        // Navigation buttons
        prevButton.setButtonText("<");
        nextButton.setButtonText(">");
        prevButton.onClick = [this] { navigatePreset(-1); };
        nextButton.onClick = [this] { navigatePreset(1); };
        
        for (auto* btn : {&prevButton, &nextButton})
        {
            btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
            btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a4a4a));
            btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.9f));
            btn->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
            btn->setLookAndFeel(&hardwareLookAndFeel);
            addAndMakeVisible(btn);
        }
    }
    
    ~KnobComponent() override
    {
        setLookAndFeel(nullptr);
        presetCombo.setLookAndFeel(nullptr);
        prevButton.setLookAndFeel(nullptr);
        nextButton.setLookAndFeel(nullptr);
        filterTypeCombo.setLookAndFeel(nullptr);
        filterSlopeCombo.setLookAndFeel(nullptr);
        riserTypeCombo.setLookAndFeel(nullptr);
        autoGainButton.setLookAndFeel(nullptr);
        macroModeCombo.setLookAndFeel(nullptr);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // 1. Background - Dark grey metal
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        // 2. Main panel - Brushed aluminum look
        auto panelBounds = bounds.reduced(15);
        
        // Panel gradient
        juce::ColourGradient panelGradient(
            juce::Colour(0xff4a4a4a), 0, 0,
            juce::Colour(0xff3a3a3a), 0, (float)getHeight(),
            false);
        g.setGradientFill(panelGradient);
        g.fillRoundedRectangle(panelBounds.toFloat(), 4.0f);
        
        // Brushed metal texture (horizontal lines)
        g.setColour(juce::Colour(0xff5a5a5a).withAlpha(0.3f));
        for (int y = panelBounds.getY(); y < panelBounds.getBottom(); y += 2)
        {
            g.drawHorizontalLine(y, (float)panelBounds.getX(), (float)panelBounds.getRight());
        }
        
        // Panel edge highlight
        g.setColour(juce::Colour(0xff6a6a6a));
        g.drawRoundedRectangle(panelBounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
        
        // Panel shadow
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRoundedRectangle(panelBounds.toFloat().expanded(0.5f), 4.0f, 1.0f);
        
        // 3. Screws in corners
        drawScrew(g, 25, 25, 12);
        drawScrew(g, bounds.getWidth() - 25, 25, 12);
        drawScrew(g, 25, bounds.getHeight() - 25, 12);
        drawScrew(g, bounds.getWidth() - 25, bounds.getHeight() - 25, 12);
        
        // 4. Brand/Model text (embossed look)
        g.setColour(juce::Colour(0xff1a1a1a));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("BUILDUP REVERB", bounds.removeFromTop(50).reduced(20, 0).translated(1, 1), 
                   juce::Justification::centred, true);
        
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("BUILDUP REVERB", bounds.removeFromTop(1).reduced(20, 0).translated(0, -51), 
                   juce::Justification::centred, true);
        
        // Draw section backgrounds in clean grid layout
        auto contentArea = getLocalBounds();
        contentArea.removeFromTop(130); // Skip header and preset area
        contentArea = contentArea.reduced(20, 10);
        contentArea.removeFromLeft(310); // Skip main knob area
        
        int sectionWidth = (contentArea.getWidth() - 20) / 2;
        int sectionHeight = 200;
        
        // Top row section backgrounds
        auto topRowArea = contentArea.removeFromTop(sectionHeight);
        
        // Filter section background (top left)
        auto filterSectionBg = topRowArea.removeFromLeft(sectionWidth).reduced(5);
        g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));
        g.fillRoundedRectangle(filterSectionBg.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(filterSectionBg.toFloat(), 4.0f, 1.0f);
        
        // Gap between sections
        topRowArea.removeFromLeft(20);
        
        // Noise section background (top right)
        auto noiseSectionBg = topRowArea.reduced(5);
        g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));
        g.fillRoundedRectangle(noiseSectionBg.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(noiseSectionBg.toFloat(), 4.0f, 1.0f);
        
        // Gap between rows
        contentArea.removeFromTop(30);
        
        // Bottom row section backgrounds
        auto bottomRowArea = contentArea.removeFromTop(sectionHeight);
        
        // Tremolo section background (bottom left)
        auto tremoloSectionBg = bottomRowArea.removeFromLeft(sectionWidth).reduced(5);
        g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));
        g.fillRoundedRectangle(tremoloSectionBg.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(tremoloSectionBg.toFloat(), 4.0f, 1.0f);
        
        // Gap between sections
        bottomRowArea.removeFromLeft(20);
        
        // Riser section background (bottom right)
        auto riserSectionBg = bottomRowArea.reduced(5);
        g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));
        g.fillRoundedRectangle(riserSectionBg.toFloat(), 4.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(riserSectionBg.toFloat(), 4.0f, 1.0f);
        
        // Model number
        g.setFont(juce::Font(10.0f));
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawText("BRV-1", juce::Rectangle<int>(bounds.getWidth() - 60, 35, 50, 20), 
                   juce::Justification::centred, true);
        
        // 5. Section divider line
        auto dividerY = 250;
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawLine(30, dividerY, bounds.getWidth() - 30, dividerY, 2);
        g.setColour(juce::Colour(0xff5a5a5a));
        g.drawLine(30, dividerY + 1, bounds.getWidth() - 30, dividerY + 1, 1);
        
        // 6. LED power indicator
        float ledX = bounds.getWidth() - 50;
        float ledY = bounds.getHeight() - 50;
        
        // LED bezel
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillEllipse(ledX - 8, ledY - 8, 16, 16);
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawEllipse(ledX - 8, ledY - 8, 16, 16, 2);
        
        // LED glow
        juce::ColourGradient ledGradient(
            juce::Colour(0xff00ff00).withAlpha(0.8f), ledX, ledY,
            juce::Colour(0xff00ff00).withAlpha(0.0f), ledX, ledY + 8,
            true);
        g.setGradientFill(ledGradient);
        g.fillEllipse(ledX - 5, ledY - 5, 10, 10);
        
        // Small text labels
        g.setColour(juce::Colour(0xff3a3a3a));
        g.setFont(juce::Font(8.0f));
        g.drawText("POWER", juce::Rectangle<int>(ledX - 20, ledY + 10, 40, 10), 
                   juce::Justification::centred, true);
        
        // Preset section background
        auto presetArea = getLocalBounds().removeFromTop(100).removeFromBottom(40).reduced(20, 0);
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(presetArea.toFloat(), 3.0f);
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(presetArea.toFloat(), 3.0f, 1.0f);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Preset controls at top
        auto presetArea = bounds.removeFromTop(80);
        presetArea.removeFromTop(50); // Space for title
        auto presetControls = presetArea.reduced(25, 5);
        
        // Label
        presetLabel.setBounds(presetControls.removeFromLeft(50));
        
        // Navigation buttons and combo
        int buttonWidth = 30;
        prevButton.setBounds(presetControls.removeFromLeft(buttonWidth));
        presetControls.removeFromLeft(5);
        nextButton.setBounds(presetControls.removeFromRight(buttonWidth));
        presetControls.removeFromRight(5);
        presetCombo.setBounds(presetControls);
        
        // Main content area
        bounds.removeFromTop(20); // spacing after preset section
        auto contentBounds = bounds.reduced(20, 10);
        
        // Left column - Main Build Up knob and Reverb
        auto leftColumn = contentBounds.removeFromLeft(280);
        
        // Main Build Up knob (larger, prominent)
        int mainKnobSize = 140;
        auto mainKnobArea = leftColumn.removeFromTop(200);
        buildUpKnob.setBounds(mainKnobArea.withSizeKeepingCentre(mainKnobSize, mainKnobSize));
        buildUpLabel.setBounds(mainKnobArea.withSizeKeepingCentre(mainKnobSize + 40, 20).translated(0, mainKnobSize/2 + 15));
        
        leftColumn.removeFromTop(30); // spacing
        
        // Reverb knob below main
        auto reverbArea = leftColumn.removeFromTop(100);
        int mediumKnobSize = 80;
        layoutKnob(reverbKnob, reverbLabel, reverbArea, mediumKnobSize);
        
        // Add spacing between columns
        contentBounds.removeFromLeft(30);
        
        // Right side - organized in 2x2 grid of sections + delay section
        auto rightSide = contentBounds;
        int sectionWidth = (rightSide.getWidth() - 20) / 2; // Split remaining width, with gap
        int sectionHeight = 180; // Reduced to make room for delay section
        int knobSize = 60;
        
        // Top row
        auto topRow = rightSide.removeFromTop(sectionHeight);
        
        // Filter Section - top left (expanded for more controls)
        auto filterSection = topRow.removeFromLeft(sectionWidth);
        filterSectionLabel.setBounds(filterSection.removeFromTop(22));
        auto filterRow1 = filterSection.removeFromTop(75);
        
        // Filter knob and resonance on first row
        int smallKnobSize = 50;
        auto filterKnobArea = filterRow1.removeFromLeft(65);
        layoutKnob(filterKnob, filterLabel, filterKnobArea, smallKnobSize);
        
        auto resonanceArea = filterRow1.removeFromLeft(65);
        layoutKnob(resonanceKnob, resonanceLabel, resonanceArea, smallKnobSize);
        
        // Drive knob
        auto driveArea = filterRow1.removeFromLeft(65);
        layoutKnob(driveKnob, driveLabel, driveArea, smallKnobSize);
        
        // Filter type dropdown - full width
        auto filterTypeRow = filterSection.removeFromTop(35).reduced(3, 0);
        filterTypeLabel.setBounds(filterTypeRow.removeFromBottom(12));
        filterTypeCombo.setBounds(filterTypeRow);
        
        // Filter slope as toggle buttons below
        filterSection.removeFromTop(5);
        auto filterSlopeRow = filterSection.removeFromTop(35).reduced(3, 0);
        filterSlopeLabel.setBounds(filterSlopeRow.removeFromBottom(12));
        filterSlopeCombo.setBounds(filterSlopeRow);
        
        // Add gap between sections
        topRow.removeFromLeft(20);
        
        // Noise Section - top right
        auto noiseSection = topRow;
        noiseSectionLabel.setBounds(noiseSection.removeFromTop(25));
        auto noiseRow = noiseSection.removeFromTop(90);
        
        // Noise knob and gate
        auto noiseKnobArea = noiseRow.removeFromLeft(65);
        layoutKnob(noiseKnob, noiseLabel, noiseKnobArea, smallKnobSize);
        
        auto gateArea = noiseRow.removeFromLeft(65);
        layoutKnob(gateKnob, gateLabel, gateArea, smallKnobSize);
        
        auto vocoderReleaseArea = noiseRow.removeFromLeft(65);
        layoutKnob(vocoderReleaseKnob, vocoderReleaseLabel, vocoderReleaseArea, smallKnobSize);
        
        noiseRow.removeFromLeft(5); // Small spacing
        
        auto vocoderBrightnessArea = noiseRow.removeFromLeft(65);
        layoutKnob(vocoderBrightnessKnob, vocoderBrightnessLabel, vocoderBrightnessArea, smallKnobSize);
        
        // Vocoder label
        auto vocoderArea = noiseSection.reduced(5, 0);
        vocoderLabel.setBounds(vocoderArea);
        
        // Add spacing between rows
        rightSide.removeFromTop(30);
        
        // Bottom row
        auto bottomRow = rightSide.removeFromTop(sectionHeight);
        
        // Tremolo Section - bottom left
        auto tremoloSection = bottomRow.removeFromLeft(sectionWidth);
        tremoloSectionLabel.setBounds(tremoloSection.removeFromTop(25));
        auto tremoloRow = tremoloSection.removeFromTop(90);
        
        // Tremolo rate and depth
        auto rateArea = tremoloRow.removeFromLeft(80);
        layoutKnob(tremoloRateKnob, tremoloRateLabel, rateArea, knobSize);
        
        auto depthArea = tremoloRow.removeFromLeft(80);
        layoutKnob(tremoloDepthKnob, tremoloDepthLabel, depthArea, knobSize);
        
        // Add gap between sections
        bottomRow.removeFromLeft(20);
        
        // Riser Section - bottom right
        auto riserSection = bottomRow;
        riserSectionLabel.setBounds(riserSection.removeFromTop(25));
        auto riserRow = riserSection.removeFromTop(90);
        
        // Riser amount and release knobs
        auto riserKnobArea = riserRow.removeFromLeft(80);
        layoutKnob(riserKnob, riserLabel, riserKnobArea, knobSize);
        
        auto riserReleaseArea = riserRow.removeFromLeft(80);
        layoutKnob(riserReleaseKnob, riserReleaseLabel, riserReleaseArea, knobSize);
        
        // Riser type dropdown
        auto riserTypeArea = riserSection.reduced(5, 0);
        riserTypeLabel.setBounds(riserTypeArea.removeFromBottom(15));
        riserTypeCombo.setBounds(riserTypeArea);
        
        // Add spacing before delay section
        rightSide.removeFromTop(30);
        
        // Delay Section - third row, spanning full width
        auto delaySection = rightSide.removeFromTop(sectionHeight);
        delaySectionLabel.setBounds(delaySection.removeFromTop(25));
        auto delayRow = delaySection.removeFromTop(90);
        
        // Delay mix and feedback knobs
        auto delayMixArea = delayRow.removeFromLeft(80);
        layoutKnob(delayMixKnob, delayMixLabel, delayMixArea, knobSize);
        
        auto delayFeedbackArea = delayRow.removeFromLeft(80);
        layoutKnob(delayFeedbackKnob, delayFeedbackLabel, delayFeedbackArea, knobSize);
        
        // Delay time dropdown
        delayRow.removeFromLeft(20); // spacing
        auto delayTimeArea = delayRow.removeFromLeft(120);
        delayTimeLabel.setBounds(delayTimeArea.removeFromBottom(15));
        delayTimeCombo.setBounds(delayTimeArea);
        
        // Stereo Width knob - positioned below left column
        leftColumn.removeFromTop(20); // spacing
        auto widthArea = leftColumn.removeFromTop(100);
        layoutKnob(widthKnob, widthLabel, widthArea, mediumKnobSize);
        
        // Smart Pan knob - positioned below width
        leftColumn.removeFromTop(10); // spacing
        auto smartPanArea = leftColumn.removeFromTop(100);
        layoutKnob(smartPanKnob, smartPanLabel, smartPanArea, mediumKnobSize);
        
        // Bottom controls area
        auto bottomControls = contentBounds.removeFromBottom(60);
        
        // Auto gain button on left
        autoGainButton.setBounds(bottomControls.removeFromLeft(120).reduced(10, 15));
        
        // Spacing
        bottomControls.removeFromLeft(30);
        
        // Macro controls on right
        auto macroArea = bottomControls.removeFromLeft(200);
        macroLabel.setBounds(macroArea.removeFromLeft(60).withTrimmedBottom(25));
        macroModeCombo.setBounds(macroArea.reduced(0, 15));
    }
    
private:
    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text, 
                   double start, double end, double interval, const juce::String& suffix, float fontSize)
    {
        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setRange(start, end, interval);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setTextValueSuffix(suffix);
        addAndMakeVisible(knob);
        
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        label.setFont(juce::Font(fontSize, juce::Font::bold));
        addAndMakeVisible(label);
    }
    
    void layoutKnob(juce::Slider& knob, juce::Label& label, juce::Rectangle<int> area, int size)
    {
        knob.setBounds(area.withSizeKeepingCentre(size, size).translated(0, -10));
        label.setBounds(area.withSizeKeepingCentre(area.getWidth(), 20).translated(0, size/2 - 5));
    }
    
    void presetChanged()
    {
        int selectedId = presetCombo.getSelectedId();
        if (selectedId > 1)
        {
            processor.setCurrentProgram(selectedId - 2);
        }
    }
    
    void navigatePreset(int direction)
    {
        int currentPreset = processor.getCurrentProgram();
        int numPresets = processor.getNumPrograms();
        int newPreset = (currentPreset + direction + numPresets) % numPresets;
        processor.setCurrentProgram(newPreset);
        presetCombo.setSelectedId(newPreset + 2, juce::dontSendNotification);
    }
    
    void filterTypeChanged()
    {
        int selectedId = filterTypeCombo.getSelectedId();
        if (selectedId > 0)
        {
            if (auto* param = processor.parameters.getParameter("filterType"))
                param->setValueNotifyingHost((selectedId - 1) / 2.0f);
        }
    }
    
    
    void riserTypeChanged()
    {
        int selectedId = riserTypeCombo.getSelectedId();
        if (selectedId > 0)
        {
            if (auto* param = processor.parameters.getParameter("riserType"))
                param->setValueNotifyingHost((selectedId - 1) / 4.0f);
        }
    }

    BuildUpVerbAudioProcessor& processor;
    HardwareLookAndFeel hardwareLookAndFeel;
    
    // Main control
    juce::Slider buildUpKnob;
    juce::Label buildUpLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> buildUpAttachment;
    
    // Secondary controls
    juce::Slider filterKnob, reverbKnob, noiseKnob;
    juce::Label filterLabel, reverbLabel, noiseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseAttachment;
    
    // Filter type
    juce::ComboBox filterTypeCombo;
    juce::Label filterTypeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment;
    
    // Additional controls
    juce::Slider tremoloRateKnob, tremoloDepthKnob, riserKnob, riserReleaseKnob;
    juce::Label tremoloRateLabel, tremoloDepthLabel, riserLabel, riserReleaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tremoloRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tremoloDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> riserAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> riserReleaseAttachment;
    
    juce::Label vocoderLabel;
    
    juce::ComboBox riserTypeCombo;
    juce::Label riserTypeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> riserTypeAttachment;
    
    // New controls
    juce::Slider resonanceKnob, widthKnob, gateKnob, vocoderReleaseKnob, vocoderBrightnessKnob, driveKnob, smartPanKnob;
    juce::Label resonanceLabel, widthLabel, gateLabel, vocoderReleaseLabel, vocoderBrightnessLabel, driveLabel, smartPanLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vocoderReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vocoderBrightnessAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> smartPanAttachment;
    
    // Delay controls
    juce::Slider delayMixKnob, delayFeedbackKnob;
    juce::Label delayMixLabel, delayFeedbackLabel;
    juce::ComboBox delayTimeCombo;
    juce::Label delayTimeLabel, delaySectionLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayTimeAttachment;
    
    // Filter slope
    juce::ComboBox filterSlopeCombo;
    juce::Label filterSlopeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterSlopeAttachment;
    
    juce::ToggleButton autoGainButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoGainAttachment;
    
    juce::ComboBox macroModeCombo;
    juce::Label macroLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> macroModeAttachment;
    
    // Section labels
    juce::Label filterSectionLabel;
    juce::Label tremoloSectionLabel;
    juce::Label noiseSectionLabel;
    juce::Label riserSectionLabel;
    
    // Preset controls
    juce::ComboBox presetCombo;
    juce::Label presetLabel;
    juce::TextButton prevButton, nextButton;
};

BuildUpVerbAudioProcessorEditor::BuildUpVerbAudioProcessorEditor (BuildUpVerbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Use native JUCE components instead of WebView
    knobComp = std::make_unique<KnobComponent>(audioProcessor);
    addAndMakeVisible(knobComp.get());
    knobComp->setBounds(0, 0, 900, 750);
    
    setSize (900, 750);  // Wider layout for sections with delay
    setResizable (false, false);
}

BuildUpVerbAudioProcessorEditor::~BuildUpVerbAudioProcessorEditor()
{
}

void BuildUpVerbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void BuildUpVerbAudioProcessorEditor::resized()
{
    if (getNumChildComponents() > 0)
        getChildComponent(0)->setBounds(getLocalBounds());
}

void BuildUpVerbAudioProcessorEditor::initialiseWebView()
{
    // Not used in native implementation
}

void BuildUpVerbAudioProcessorEditor::loadWebContent()
{
    // Not used in native implementation
}

void BuildUpVerbAudioProcessorEditor::timerCallback()
{
    // Not used in native implementation
}

void BuildUpVerbAudioProcessorEditor::sendParameterUpdate()
{
    // Not used in native implementation
}