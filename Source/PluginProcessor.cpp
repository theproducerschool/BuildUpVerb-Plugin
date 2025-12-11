#include "PluginProcessor.h"
#include "PluginEditor.h"

// Factory presets
const BuildUpVerbAudioProcessor::Preset BuildUpVerbAudioProcessor::factoryPresets[BuildUpVerbAudioProcessor::numPresets] = 
{
    // Name,            BuildUp, Filter, Reverb, Noise, NoiseType, TremRate, TremDepth, Riser, RiserType
    {"Subtle Rise",        25.0f,  50.0f,  30.0f,  0.0f,  0,  4.0f,  0.0f,  10.0f, 0},  // Sine
    {"Heavy Build",        75.0f,  80.0f,  60.0f,  10.0f, 1,  6.0f,  20.0f, 30.0f, 1},  // Saw
    {"Filter Sweep",       50.0f,  100.0f, 40.0f,  0.0f,  0,  0.5f,  0.0f,  0.0f,  0},  // None
    {"Noise Storm",        60.0f,  70.0f,  50.0f,  80.0f, 0,  8.0f,  40.0f, 15.0f, 3},  // Noise Sweep
    {"Cathedral",          40.0f,  30.0f,  90.0f,  5.0f,  2,  2.0f,  10.0f, 0.0f,  0},  // None
    {"Tension Builder",    80.0f,  90.0f,  70.0f,  20.0f, 1,  10.0f, 50.0f, 60.0f, 2},  // Square
    {"Subtle Texture",     30.0f,  40.0f,  25.0f,  15.0f, 2,  3.0f,  15.0f, 5.0f,  0},  // Sine
    {"Drop Ready",         90.0f,  100.0f, 80.0f,  30.0f, 0,  16.0f, 70.0f, 80.0f, 4},  // Sub Drop
    {"Ambient Wash",       35.0f,  20.0f,  85.0f,  10.0f, 1,  0.8f,  25.0f, 0.0f,  0},  // None
    {"Clean Sweep",        45.0f,  75.0f,  15.0f,  0.0f,  0,  1.0f,  0.0f,  20.0f, 1}   // Saw
};

BuildUpVerbAudioProcessor::BuildUpVerbAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters (*this, nullptr, juce::Identifier ("BuildUpVerb"), createParameterLayout()),
       forwardFFT (fftOrder),
       fftData (fftSize * 2),
       window (fftSize),
       frequencyData (fftSize / 2 + 1),
       magnitudes (fftSize / 2 + 1),
       phases (fftSize / 2 + 1),
       bandLevels (numBands),
       smoothedBandLevels (numBands),
       overlapBuffer (fftSize * 2, 0.0f),  // Stereo overlap buffer
       prevPhases (fftSize / 2 + 1, 0.0f),
       inputBuffer (fftSize * 2, 0.0f),    // Stereo input buffer
       outputBuffer (fftSize * 2, 0.0f)    // Stereo output buffer
{
    // Create Hann window
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1));
}

BuildUpVerbAudioProcessor::~BuildUpVerbAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BuildUpVerbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("buildup",
                                                             "Build Up",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("filterIntensity",
                                                             "Filter Intensity",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             75.0f));
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("filterType",
                                                              "Filter Type",
                                                              juce::StringArray {"High Pass", "Low Pass", "Dual Sweep"},
                                                              0));
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("filterSlope",
                                                              "Filter Slope",
                                                              juce::StringArray {"6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct"},
                                                              1)); // Default to 12 dB/oct
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("reverbMix",
                                                             "Reverb Mix",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             50.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("noiseAmount",
                                                             "Noise Amount",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f));
    
    // Removed noise type - now always vocoder
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("vocoderRelease",
                                                             "Vocoder Release",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             50.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("tremoloRate",
                                                             "Tremolo Rate",
                                                             juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.5f),
                                                             4.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("tremoloDepth",
                                                             "Tremolo Depth",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("riserAmount",
                                                             "Riser Amount",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("riserType",
                                                              "Riser Type",
                                                              juce::StringArray {"Sine", "Saw", "Square", "Noise Sweep", "Sub Drop"},
                                                              0));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("riserRelease",
                                                             "Riser Release",
                                                             juce::NormalisableRange<float> (0.01f, 5.0f, 0.01f, 0.5f),
                                                             0.1f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("filterResonance",
                                                             "Filter Resonance",
                                                             juce::NormalisableRange<float> (0.5f, 4.0f, 0.01f, 0.5f),
                                                             1.5f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("filterDrive",
                                                             "Filter Drive",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("filterSlope",
                                                              "Filter Slope",
                                                              juce::StringArray {"6dB", "12dB", "18dB", "24dB"},
                                                              1)); // Default to 12dB
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("stereoWidth",
                                                             "Stereo Width",
                                                             juce::NormalisableRange<float> (0.0f, 200.0f, 0.01f),
                                                             100.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("smartPan",
                                                             "Smart Pan",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f)); // Default off
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("noiseGate",
                                                             "Noise Threshold",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             30.0f));
    
    layout.add (std::make_unique<juce::AudioParameterBool> ("autoGain",
                                                            "Auto Gain",
                                                            true));
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("macroMode",
                                                              "Macro Mode",
                                                              juce::StringArray {"Off", "Subtle", "Aggressive", "Epic", "Custom"},
                                                              0));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("delayMix",
                                                             "Delay Mix",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f)); // Default off
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("delayTime",
                                                              "Delay Time",
                                                              juce::StringArray {"1/2", "1/3", "1/4", "1/8", "1/16"},
                                                              2)); // Default to 1/4
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("delayFeedback",
                                                             "Delay Feedback",
                                                             juce::NormalisableRange<float> (0.0f, 90.0f, 0.01f),
                                                             50.0f)); // Default 50%
    
    return layout;
}

const juce::String BuildUpVerbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BuildUpVerbAudioProcessor::acceptsMidi() const
{
    return false;
}

bool BuildUpVerbAudioProcessor::producesMidi() const
{
    return false;
}

bool BuildUpVerbAudioProcessor::isMidiEffect() const
{
    return false;
}

double BuildUpVerbAudioProcessor::getTailLengthSeconds() const
{
    return 5.0; // Reverb tail
}

int BuildUpVerbAudioProcessor::getNumPrograms()
{
    return numPresets;
}

int BuildUpVerbAudioProcessor::getCurrentProgram()
{
    return currentPreset;
}

void BuildUpVerbAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < numPresets)
    {
        currentPreset = index;
        const Preset& preset = factoryPresets[index];
        
        // Set parameter values
        if (auto* param = parameters.getParameter("buildup"))
            param->setValueNotifyingHost(preset.buildUp / 100.0f);
        if (auto* param = parameters.getParameter("filterIntensity"))
            param->setValueNotifyingHost(preset.filterIntensity / 100.0f);
        if (auto* param = parameters.getParameter("reverbMix"))
            param->setValueNotifyingHost(preset.reverbMix / 100.0f);
        if (auto* param = parameters.getParameter("noiseAmount"))
            param->setValueNotifyingHost(preset.noiseAmount / 100.0f);
        if (auto* param = parameters.getParameter("noiseType"))
            param->setValueNotifyingHost(preset.noiseType / 2.0f);
        if (auto* param = parameters.getParameter("tremoloRate"))
            param->setValueNotifyingHost((preset.tremoloRate - 0.1f) / 19.9f);
        if (auto* param = parameters.getParameter("tremoloDepth"))
            param->setValueNotifyingHost(preset.tremoloDepth / 100.0f);
        if (auto* param = parameters.getParameter("riserAmount"))
            param->setValueNotifyingHost(preset.riserAmount / 100.0f);
        if (auto* param = parameters.getParameter("riserType"))
            param->setValueNotifyingHost(preset.riserType / 4.0f);
    }
}

const juce::String BuildUpVerbAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < numPresets)
        return factoryPresets[index].name;
    return {};
}

void BuildUpVerbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    // Factory presets can't be renamed
}

void BuildUpVerbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    freeverb.prepare (sampleRate, samplesPerBlock);
    highPassFilter.prepare (spec);
    lowPassFilter.prepare (spec);
    highPassFilter2.prepare (spec);
    highPassFilter3.prepare (spec);
    highPassFilter4.prepare (spec);
    lowPassFilter2.prepare (spec);
    lowPassFilter3.prepare (spec);
    lowPassFilter4.prepare (spec);
    
    highPassFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    highPassFilter2.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    highPassFilter3.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    highPassFilter4.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter3.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter4.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    
    // Initialize noise filter for noise sweep riser
    noiseFilter.prepare (spec);
    noiseFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    
    // Initialize delay buffers (up to 2 seconds at any sample rate)
    int maxDelaySamples = (int)(sampleRate * 2.0);
    delayBufferL.setSize(1, maxDelaySamples);
    delayBufferR.setSize(1, maxDelaySamples);
    delayBufferL.clear();
    delayBufferR.clear();
    delayWritePos = 0;
    
    // Initialize FFT buffers for vocoder
    fftInputBuffer.setSize(2, fftSize);
    fftOutputBuffer.setSize(2, fftSize);
    fftInputBuffer.clear();
    fftOutputBuffer.clear();
    
    updateDSPFromParameters();
}

void BuildUpVerbAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BuildUpVerbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void BuildUpVerbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Get buildup value first for immediate bypass check
    float buildUp = *parameters.getRawParameterValue ("buildup");
    float buildUpNorm = buildUp / 100.0f;
    
    // Smooth the build up parameter to prevent clicks
    const float buildUpSmoothCoeff = 0.995f; // Very smooth
    smoothedBuildUp = smoothedBuildUp * buildUpSmoothCoeff + buildUpNorm * (1.0f - buildUpSmoothCoeff);
    
    // Immediate bypass - if Build Up is 0, pass through without ANY processing
    if (buildUpNorm < 0.001f)
    {
        return; // Input buffer passes through untouched
    }
    
    updateDSPFromParameters();
    
    // Get macro mode early for the control system
    int macroMode = (int)*parameters.getRawParameterValue ("macroMode");
    
    // Apply macro control if enabled
    if (macroMode > 0 && std::abs(buildUp - lastMacroValue) > 0.01f)
    {
        lastMacroValue = buildUp;
        applyMacroControl(buildUpNorm, macroMode);
    }
    float filterIntensity = *parameters.getRawParameterValue ("filterIntensity");
    float reverbMix = *parameters.getRawParameterValue ("reverbMix");
    float noiseAmount = *parameters.getRawParameterValue ("noiseAmount");
    // Noise type removed - always vocoder now
    float tremoloRate = *parameters.getRawParameterValue ("tremoloRate");
    float tremoloDepth = *parameters.getRawParameterValue ("tremoloDepth");
    float riserAmount = *parameters.getRawParameterValue ("riserAmount");
    int riserType = (int)*parameters.getRawParameterValue ("riserType");
    float riserRelease = *parameters.getRawParameterValue ("riserRelease");
    float stereoWidth = *parameters.getRawParameterValue ("stereoWidth");
    float smartPan = *parameters.getRawParameterValue ("smartPan");
    float noiseGate = *parameters.getRawParameterValue ("noiseGate");
    bool autoGain = *parameters.getRawParameterValue ("autoGain") > 0.5f;
    float delayMix = *parameters.getRawParameterValue ("delayMix");
    int delayTimeChoice = (int)*parameters.getRawParameterValue ("delayTime");
    float delayFeedback = *parameters.getRawParameterValue ("delayFeedback");
    
    // buildUpNorm already declared at the top
    float filterIntensityNorm = filterIntensity / 100.0f;
    float reverbMixNorm = reverbMix / 100.0f;
    float noiseAmountNorm = noiseAmount / 100.0f;
    
    // Calculate envelope follower from input signal
    float inputRMS = 0.0f;
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float sampleSquared = channelData[sample] * channelData[sample];
            inputRMS += sampleSquared;
        }
    }
    inputRMS = std::sqrt(inputRMS / (numChannels * numSamples));
    
    // Smart envelope follower with musical attack/release
    float targetEnvelope = inputRMS;
    if (targetEnvelope > envelopeLevel)
    {
        // Very fast attack (instant response to signal)
        envelopeLevel += (targetEnvelope - envelopeLevel) * 0.99f;
    }
    else
    {
        // VERY fast release - noise should stop almost immediately
        // About 20ms release time for instant response
        envelopeLevel += (targetEnvelope - envelopeLevel) * 0.8f;
    }
    
    // Smart noise gate: only allow noise when signal is present
    // Convert noise gate parameter (0-100) to threshold (0.0001 to 0.1 = -80dB to -20dB)
    float dynamicThreshold = 0.0001f + (noiseGate / 100.0f) * (0.1f - 0.0001f);
    float envelopeGate = (envelopeLevel > dynamicThreshold) ? 1.0f : 0.0f;
    
    // Very fast gate transitions - almost instant noise cutoff
    static float smoothEnvelopeGate = 0.0f;
    smoothEnvelopeGate += (envelopeGate - smoothEnvelopeGate) * 0.9f; // Near-instant gate response
    
    // NOTE: Reverb buffer creation moved to AFTER noise generation
    // so reverb can process the vocoded noise properly
    
    // Process intelligent filter automation (controlled by filter intensity)
    if (filterIntensityNorm > 0.01f)
    {
        int filterType = (int)*parameters.getRawParameterValue ("filterType");
        int filterSlope = (int)*parameters.getRawParameterValue ("filterSlope");
        float filterDrive = *parameters.getRawParameterValue ("filterDrive");
        float driveNorm = filterDrive / 100.0f;
        
        // Apply pre-drive saturation if enabled
        if (driveNorm > 0.01f)
        {
            float driveGain = 1.0f + driveNorm * 4.0f; // Up to 5x gain
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* data = buffer.getWritePointer(channel);
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    // Soft clip saturation
                    float driven = data[sample] * driveGain;
                    data[sample] = std::tanh(driven) / (1.0f + driveNorm * 0.5f); // Compensate gain
                }
            }
        }
        
        // filterSlope: 0 = 6dB (1 stage), 1 = 12dB (2 stages), 2 = 18dB (3 stages), 3 = 24dB (4 stages)
        // Each stage provides 6dB/octave of attenuation
        int numStages = filterSlope + 1;
        
        // Create filter context
        juce::dsp::AudioBlock<float> filterBlock (buffer);
        juce::dsp::ProcessContextReplacing<float> filterContext (filterBlock);
        
        if (filterType == 2) // Band Pass - use dual-filter automation
        {
            // Apply high-pass stages
            highPassFilter.process (filterContext);
            if (numStages >= 2) highPassFilter2.process (filterContext);
            if (numStages >= 3) highPassFilter3.process (filterContext);
            if (numStages >= 4) highPassFilter4.process (filterContext);
            
            // Apply low-pass stages
            lowPassFilter.process (filterContext);
            if (numStages >= 2) lowPassFilter2.process (filterContext);
            if (numStages >= 3) lowPassFilter3.process (filterContext);
            if (numStages >= 4) lowPassFilter4.process (filterContext);
        }
        else if (filterType == 0) // High Pass only
        {
            highPassFilter.process (filterContext);
            if (numStages >= 2) highPassFilter2.process (filterContext);
            if (numStages >= 3) highPassFilter3.process (filterContext);
            if (numStages >= 4) highPassFilter4.process (filterContext);
        }
        else if (filterType == 1) // Low Pass only
        {
            lowPassFilter.process (filterContext);
            if (numStages >= 2) lowPassFilter2.process (filterContext);
            if (numStages >= 3) lowPassFilter3.process (filterContext);
            if (numStages >= 4) lowPassFilter4.process (filterContext);
        }
    }
    
    // True FFT Vocoder - linked to Build Up
    if (noiseAmountNorm > 0.01f && buildUpNorm > 0.01f)
    {
        // Vocoder gain based on build up AND noise amount
        // Use raw buildUpNorm instead of smoothedBuildUp to prevent modulation
        float vocoderGain = buildUpNorm * noiseAmountNorm;
        float vocoderReleaseAmount = *parameters.getRawParameterValue ("vocoderRelease") / 100.0f;
        
        // Create buffer for vocoder output
        juce::AudioBuffer<float> noiseBuffer(buffer.getNumChannels(), buffer.getNumSamples());
        noiseBuffer.clear();
        
        // Process vocoder - use filterbank for 4-band like Ableton
        processVocoderFilterbank(buffer, noiseBuffer, vocoderGain, vocoderReleaseAmount);
        
        // BYPASS FILTERING FOR NOW TO TEST IF THIS IS THE ISSUE
        // The filters might be causing the ringing with high resonance
        /*
        // Apply filtering to noise based on Build Up position
        if (filterIntensityNorm > 0.01f && buildUpNorm > 0.01f)
        {
            // Use the same filter settings as the main signal
            int filterType = (int)*parameters.getRawParameterValue ("filterType");
            float filterAmount = buildUpNorm * filterIntensityNorm;
            
            juce::dsp::AudioBlock<float> noiseBlock(noiseBuffer);
            juce::dsp::ProcessContextReplacing<float> noiseContext(noiseBlock);
            
            // Apply the same filter that was applied to the main signal
            if (filterType == 0 || filterType == 2) // High pass or dual
            {
                highPassFilter.process(noiseContext);
            }
            if (filterType == 1 || filterType == 2) // Low pass or dual  
            {
                lowPassFilter.process(noiseContext);
            }
        }
        */
        
        
        // Mix filtered noise into main buffer
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* mainData = buffer.getWritePointer(channel);
            auto* noiseData = noiseBuffer.getReadPointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                mainData[sample] += noiseData[sample];
            }
        }
    }
    else
    {
        // Reset vocoder band levels when build up is off
        std::fill(smoothedBandLevels.begin(), smoothedBandLevels.end(), 0.0f);
        fftPos = 0;
    }
    
    // NOW process reverb AFTER noise has been added to the main buffer
    // This ensures reverb processes the vocoded noise with proper release
    juce::AudioBuffer<float> reverbBuffer (buffer.getNumChannels(), buffer.getNumSamples());
    
    // Copy current buffer (including noise) to reverb buffer for processing
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        reverbBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
    
    // Process reverb only if reverb mix > 0
    if (reverbMixNorm > 0.001f)
    {
        juce::dsp::AudioBlock<float> reverbBlock (reverbBuffer);
        juce::dsp::ProcessContextReplacing<float> reverbContext (reverbBlock);
        freeverb.process (reverbBuffer);
    }
    else
    {
        // No reverb, clear the buffer
        reverbBuffer.clear();
    }
    
    // Add riser effect with intelligent envelope
    float targetRiserLevel = buildUpNorm * (riserAmount / 100.0f) * 0.15f;
    
    // Smooth envelope to prevent clicks - both attack and release
    float envelopeSpeed = 0.0001f; // Much smoother transitions
    if (targetRiserLevel > currentRiserLevel)
    {
        // Smooth attack to prevent clicks
        currentRiserLevel += (targetRiserLevel - currentRiserLevel) * envelopeSpeed * 50.0f; // Still fast but smooth
    }
    else if (targetRiserLevel < currentRiserLevel)
    {
        // Smooth release
        float releaseSpeed = envelopeSpeed * (1.0f / riserRelease);
        currentRiserLevel += (targetRiserLevel - currentRiserLevel) * releaseSpeed;
    }
    
    if (currentRiserLevel > 0.01f)
    {
        float riserLevel = currentRiserLevel;
        
        // Smart frequency curves based on riser type
        float baseFreq = 50.0f;
        float maxFreq = 4000.0f;
        
        // Exponential curve for more natural buildup feeling
        float buildUpCurve = buildUpNorm * buildUpNorm * buildUpNorm; // Cubic for dramatic effect
        
        // Add subtle vibrato for organic feel
        float vibratoRate = 4.0f + buildUpNorm * 2.0f; // Faster vibrato as it builds
        float vibratoDepth = 0.02f + buildUpNorm * 0.05f; // Deeper vibrato as it builds  
        float vibrato = std::sin(2.0f * juce::MathConstants<float>::pi * vibratoRate * riserPhase / spec.sampleRate) * vibratoDepth;
        
        switch (riserType)
        {
            case 0: // Sine riser - smooth and musical
            {
                float targetFreq = baseFreq + buildUpCurve * (maxFreq - baseFreq);
                targetFreq *= (1.0f + vibrato); // Add vibrato modulation
                
                // Smooth frequency response to prevent artifacts
                riserFreq += (targetFreq - riserFreq) * 0.001f; // Much smoother
                
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    auto* data = buffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    {
                        float riserSample = std::sin(2.0f * juce::MathConstants<float>::pi * riserPhase) * riserLevel;
                        data[sample] += riserSample;
                        
                        // Use normalized phase increment
                        float phaseIncrement = riserFreq / spec.sampleRate;
                        riserPhase += phaseIncrement;
                        while (riserPhase >= 1.0f)
                            riserPhase -= 1.0f;
                    }
                }
                break;
            }
            
            case 1: // Saw riser - aggressive and cutting
            {
                float targetFreq = baseFreq + buildUpCurve * (maxFreq - baseFreq) * 1.2f; // Slightly higher for aggression
                targetFreq *= (1.0f + vibrato * 0.5f); // Less vibrato for cleaner aggressive sound
                
                // Smooth frequency response to prevent artifacts
                smoothSawFreq += (targetFreq - smoothSawFreq) * 0.001f;
                
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    auto* data = buffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    {
                        // Generate saw wave
                        float sawSample = (2.0f * sawPhase - 1.0f) * riserLevel;
                        data[sample] += sawSample;
                        
                        sawPhase += smoothSawFreq / spec.sampleRate;
                        if (sawPhase >= 1.0f)
                            sawPhase -= 1.0f;
                    }
                }
                break;
            }
            
            case 2: // Square riser - digital and punchy
            {
                float targetFreq = baseFreq + buildUpCurve * (maxFreq - baseFreq) * 0.8f; // Lower max for punchiness
                targetFreq *= (1.0f + vibrato * 0.3f); // Subtle vibrato
                
                // Smooth frequency response to prevent artifacts
                smoothSquareFreq += (targetFreq - smoothSquareFreq) * 0.001f;
                
                // Use member phase for square wave
                
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    auto* data = buffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    {
                        float squareSample = (std::sin(2.0f * juce::MathConstants<float>::pi * squarePhase) > 0.0f ? 1.0f : -1.0f) * riserLevel * 0.7f;
                        data[sample] += squareSample;
                        
                        // Use normalized phase increment
                        float phaseIncrement = smoothSquareFreq / spec.sampleRate;
                        squarePhase += phaseIncrement;
                        while (squarePhase >= 1.0f)
                            squarePhase -= 1.0f;
                    }
                }
                break;
            }
            
            case 3: // Noise sweep
            {
                // Bandpass filter frequency controlled by buildup
                float filterFreq = 100.0f + buildUpNorm * buildUpNorm * 8000.0f;
                noiseFilter.setCutoffFrequency(filterFreq);
                noiseFilter.setResonance(2.0f + buildUpNorm * 3.0f); // Higher resonance as it builds
                
                // Generate filtered noise
                juce::AudioBuffer<float> noiseBuffer(buffer.getNumChannels(), buffer.getNumSamples());
                
                for (int channel = 0; channel < noiseBuffer.getNumChannels(); ++channel)
                {
                    auto* noiseData = noiseBuffer.getWritePointer(channel);
                    for (int sample = 0; sample < noiseBuffer.getNumSamples(); ++sample)
                    {
                        noiseData[sample] = (random.nextFloat() * 2.0f - 1.0f) * riserLevel * 2.0f;
                    }
                }
                
                // Apply filter to noise
                juce::dsp::AudioBlock<float> noiseBlock(noiseBuffer);
                juce::dsp::ProcessContextReplacing<float> noiseContext(noiseBlock);
                noiseFilter.process(noiseContext);
                
                // Add to reverb buffer
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    buffer.addFrom(channel, 0, noiseBuffer, channel, 0, buffer.getNumSamples());
                }
                break;
            }
            
            case 4: // Sub drop - reverse buildup for drops
            {
                // Start high and go low with exponential curve
                float reverseDropCurve = (1.0f - buildUpNorm) * (1.0f - buildUpNorm); // Exponential decay
                float targetFreq = 30.0f + reverseDropCurve * (maxFreq * 0.5f - 30.0f);
                targetFreq *= (1.0f + vibrato * 0.2f); // Minimal vibrato for sub frequencies
                
                // Smooth frequency changes for sub
                smoothSubFreq += (targetFreq - smoothSubFreq) * 0.001f;
                
                // Use member phase for sub drop
                
                for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                {
                    auto* data = buffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    {
                        float subSample = std::sin(2.0f * juce::MathConstants<float>::pi * subPhase) * riserLevel * 1.5f;
                        data[sample] += subSample;
                        
                        // Use normalized phase increment
                        float phaseIncrement = smoothSubFreq / spec.sampleRate;
                        subPhase += phaseIncrement;
                        while (subPhase >= 1.0f)
                            subPhase -= 1.0f;
                    }
                }
                break;
            }
        }
    }
    else
    {
        // Don't reset phases immediately - let them continue naturally
        // This prevents clicks and artifacts
    }
    
    // Apply tremolo to main signal (now works without reverb)
    if (tremoloDepth / 100.0f > 0.01f)
    {
        float tremDepth = (tremoloDepth / 100.0f);
        float phaseIncrement = tremoloRate * 2.0f * juce::MathConstants<float>::pi / spec.sampleRate;
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer (channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float tremolo = 1.0f - (tremDepth * 0.5f * (1.0f + std::sin(tremoloPhase)));
                data[sample] *= tremolo;
                
                tremoloPhase += phaseIncrement;
                if (tremoloPhase > 2.0f * juce::MathConstants<float>::pi)
                    tremoloPhase -= 2.0f * juce::MathConstants<float>::pi;
            }
        }
    }
    else
    {
        // Let tremolo phase continue to prevent clicks
        // It will naturally fade with the amplitude
    }
    
    // Apply stereo width processing to main signal  
    if (buffer.getNumChannels() >= 2 && std::abs(stereoWidth - 100.0f) > 0.1f)
    {
        float width = stereoWidth / 100.0f;
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float left = buffer.getSample(0, sample);
            float right = buffer.getSample(1, sample);
            
            // M/S processing
            float mid = (left + right) * 0.5f;
            float side = (left - right) * 0.5f * width;
            
            buffer.setSample(0, sample, mid + side);
            buffer.setSample(1, sample, mid - side);
        }
    }
    
    // Apply smart panning linked to tremolo
    if (smartPan > 0.01f && buffer.getNumChannels() >= 2)
    {
        float panDepth = (smartPan / 100.0f);
        
        // Use the same tremolo phase but offset for right channel
        float panPhaseL = tremoloPhase;
        float panPhaseR = tremoloPhase + juce::MathConstants<float>::pi; // 180 degrees out of phase
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float left = buffer.getSample(0, sample);
            float right = buffer.getSample(1, sample);
            
            // Calculate pan positions using sin wave (same as tremolo)
            float panL = 0.5f + 0.5f * std::sin(panPhaseL) * panDepth;
            float panR = 0.5f + 0.5f * std::sin(panPhaseR) * panDepth;
            
            // Apply panning
            buffer.setSample(0, sample, left * (1.0f - panR) + right * (1.0f - panL) * 0.5f);
            buffer.setSample(1, sample, right * (1.0f - panL) + left * (1.0f - panR) * 0.5f);
            
            // Update phase (using tremolo rate)
            float phaseIncrement = tremoloRate * 2.0f * juce::MathConstants<float>::pi / spec.sampleRate;
            panPhaseL += phaseIncrement;
            panPhaseR += phaseIncrement;
            
            // Wrap phases
            if (panPhaseL > 2.0f * juce::MathConstants<float>::pi)
                panPhaseL -= 2.0f * juce::MathConstants<float>::pi;
            if (panPhaseR > 2.0f * juce::MathConstants<float>::pi)
                panPhaseR -= 2.0f * juce::MathConstants<float>::pi;
        }
    }
    
    // Always update delay tempo and parameters
    if (auto* playHead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo positionInfo;
        if (playHead->getCurrentPosition(positionInfo))
        {
            if (positionInfo.bpm > 0)
                currentBPM = (float)positionInfo.bpm;
        }
    }
    
    // Calculate delay time based on tempo and note division
    float beatsPerSecond = currentBPM / 60.0f;
    float delayInBeats = 1.0f;
    
    switch (delayTimeChoice)
    {
        case 0: delayInBeats = 0.5f; break;    // 1/2
        case 1: delayInBeats = 0.333f; break;  // 1/3
        case 2: delayInBeats = 0.25f; break;   // 1/4
        case 3: delayInBeats = 0.125f; break;  // 1/8
        case 4: delayInBeats = 0.0625f; break; // 1/16
    }
    
    float delayInSeconds = delayInBeats / beatsPerSecond;
    delaySampleLength = (int)(delayInSeconds * spec.sampleRate);
    
    // Always process delay buffer to keep it in sync
    float delayMixNorm = (delayMix / 100.0f);
    float feedbackNorm = delayFeedback / 100.0f;
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Get delayed samples
        int readPos = delayWritePos - delaySampleLength;
        if (readPos < 0)
            readPos += delayBufferL.getNumSamples();
        
        float delayedL = delayBufferL.getSample(0, readPos);
        float delayedR = delayBufferR.getSample(0, readPos);
        
        // Get current samples from main buffer
        float currentL = buffer.getSample(0, sample);
        float currentR = buffer.getSample(1, sample);
        
        // Only apply feedback if delay is active
        if (delayMix > 0.01f)
        {
            // Write to delay buffer with feedback (with safety limiting)
            float writeL = currentL + delayedL * feedbackNorm * 0.95f;
            float writeR = currentR + delayedR * feedbackNorm * 0.95f;
            
            // Soft clip to prevent overload
            writeL = std::tanh(writeL);
            writeR = std::tanh(writeR);
            
            delayBufferL.setSample(0, delayWritePos, writeL);
            delayBufferR.setSample(0, delayWritePos, writeR);
            
            // Mix delayed signal with current in main buffer
            buffer.setSample(0, sample, currentL + delayedL * delayMixNorm);
            buffer.setSample(1, sample, currentR + delayedR * delayMixNorm);
        }
        else
        {
            // When mix is 0, just write dry signal to clear buffer
            delayBufferL.setSample(0, delayWritePos, currentL * 0.0f);
            delayBufferR.setSample(0, delayWritePos, currentR * 0.0f);
            
            // No mixing - output is unchanged
        }
        
        // Always increment write position to keep buffer moving
        delayWritePos++;
        if (delayWritePos >= delayBufferL.getNumSamples())
            delayWritePos = 0;
    }
    
    // Now mix in the reverb based on reverb amount
    // Calculate reverb wet level based on Build Up intensity AND reverb mix
    float reverbWetLevel = buildUpNorm * reverbMixNorm;
    
    // Apply a subtle gain reduction to compensate for any buildup
    float mixCompensation = 1.0f / (1.0f + reverbWetLevel * 0.2f);
    
    // Only mix in reverb if there's reverb to add
    if (reverbWetLevel > 0.001f)
    {
        // Mix reverb with the already-processed signal
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* dry = buffer.getWritePointer(channel);
            auto* wet = reverbBuffer.getReadPointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                // Simple reverb mix - add wet reverb to processed signal
                dry[sample] = dry[sample] * (1.0f - reverbWetLevel) + wet[sample] * reverbWetLevel;
            }
        }
    }
    
    // Calculate auto gain compensation - much more subtle to prevent gain jumps
    float gainCompensation = 1.0f;
    if (autoGain)
    {
        // Very gentle gain reduction - only for extreme settings
        float effectsActive = 0.0f;
        effectsActive += reverbWetLevel * 0.05f; // Much less aggressive
        effectsActive += (filterIntensityNorm * buildUpNorm) * 0.02f;
        effectsActive += (noiseAmountNorm * buildUpNorm) * 0.03f;
        effectsActive += (riserAmount / 100.0f * buildUpNorm) * 0.02f;
        effectsActive += (tremoloDepth / 100.0f * buildUpNorm) * 0.01f;
        
        // Much gentler reduction curve
        gainCompensation = 1.0f / (1.0f + effectsActive * 0.3f);
        
        // Very slow gain changes to prevent audible jumps
        float targetGain = gainCompensation;
        currentGainReduction += (targetGain - currentGainReduction) * 0.002f; // Much slower
        gainCompensation = currentGainReduction;
    }
    
    // Apply final gain compensation
    if (gainCompensation < 1.0f || mixCompensation < 1.0f)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                data[sample] *= gainCompensation * mixCompensation;
            }
        }
    }
    
    // Store previous buildup to detect changes
    previousBuildUp = buildUpNorm;
}

void BuildUpVerbAudioProcessor::updateDSPFromParameters()
{
    float buildUp = *parameters.getRawParameterValue ("buildup");
    float filterIntensity = *parameters.getRawParameterValue ("filterIntensity");
    float filterResonance = *parameters.getRawParameterValue ("filterResonance");
    int filterType = (int)*parameters.getRawParameterValue ("filterType");
    int filterSlope = (int)*parameters.getRawParameterValue ("filterSlope");
    float buildUpNorm = buildUp / 100.0f;
    float filterIntensityNorm = filterIntensity / 100.0f;
    
    // Update Freeverb parameters based on SMOOTHED build up to prevent clicks
    // Freeverb has different parameter ranges than JUCE reverb
    freeverb.setRoomSize(0.3f + (smoothedBuildUp * 0.65f));      // 0.3 to 0.95 (Freeverb sounds best 0.0-1.0)
    freeverb.setDamping(0.7f - (smoothedBuildUp * 0.5f));        // 0.7 to 0.2 (less damping = brighter)
    freeverb.setWetLevel(0.3f + (smoothedBuildUp * 0.5f));       // 0.3 to 0.8 - balanced wet level
    freeverb.setDryLevel(0.0f);                               // 0% dry - we add dry signal separately
    freeverb.setWidth(0.5f + (smoothedBuildUp * 0.5f));          // 0.5 to 1.0
    freeverb.setFreezeMode(0.0f);                            // No freeze
    
    // Simple linear filter automation for high/low pass, logarithmic only for bandpass
    float filterAmount = buildUpNorm * filterIntensityNorm;
    
    // Helper lambda to set all filter stages
    auto setAllHighPassFilters = [&](float freq, float res) {
        highPassFilter.setCutoffFrequency(freq);
        highPassFilter.setResonance(res);
        highPassFilter2.setCutoffFrequency(freq);
        highPassFilter2.setResonance(0.5f); // Lower resonance on cascaded stages
        highPassFilter3.setCutoffFrequency(freq);
        highPassFilter3.setResonance(0.5f);
        highPassFilter4.setCutoffFrequency(freq);
        highPassFilter4.setResonance(0.5f);
    };
    
    auto setAllLowPassFilters = [&](float freq, float res) {
        lowPassFilter.setCutoffFrequency(freq);
        lowPassFilter.setResonance(res);
        lowPassFilter2.setCutoffFrequency(freq);
        lowPassFilter2.setResonance(0.5f); // Lower resonance on cascaded stages
        lowPassFilter3.setCutoffFrequency(freq);
        lowPassFilter3.setResonance(0.5f);
        lowPassFilter4.setCutoffFrequency(freq);
        lowPassFilter4.setResonance(0.5f);
    };
    
    // Unity gain bypass when no filtering
    if (filterAmount < 0.001f)
    {
        // No filtering needed - filters stay at safe defaults
        setAllHighPassFilters(20.0f, 0.5f);
        setAllLowPassFilters(20000.0f, 0.5f);
        return;
    }
    
    // Resonance compensation - reduce resonance for steeper slopes to avoid excessive ringing
    float slopeResonanceScale = 1.0f / (1.0f + filterSlope * 0.2f);
    
    // More gentle resonance calculation - less aggressive scaling
    float resonanceAmount = filterResonance * (1.0f + filterAmount * 0.3f) * slopeResonanceScale;
    
    switch (filterType)
    {
        case 0: // High Pass only - simple linear sweep
            {
                // Simple linear sweep from 20Hz to 2kHz
                float hpFreq = 20.0f + (filterAmount * 1980.0f); // 20Hz to 2000Hz
                setAllHighPassFilters(hpFreq, juce::jmin(resonanceAmount, 4.0f));
                break;
            }
            
        case 1: // Low Pass only - simple linear sweep  
            {
                // Simple linear sweep from 20kHz down to 500Hz
                float lpFreq = 20000.0f - (filterAmount * 19500.0f); // 20kHz down to 500Hz
                setAllLowPassFilters(lpFreq, juce::jmin(resonanceAmount, 4.0f));
                break;
            }
            
        case 2: // Dual Sweep - GENTLER, MORE MUSICAL PROGRESSION
            {
                // Much gentler curve for smoother filtering
                float curve = std::pow(filterAmount, 2.0f); // Quadratic curve - starts very gentle
                
                // Use mostly curved component for smoother onset
                float effectiveAmount = curve;
                
                // Much gentler sweep ranges
                // Low-cut sweeps up more gradually
                float lowCutFreq = 20.0f + (effectiveAmount * 3000.0f); // 20Hz to 3020Hz max (was 7000)
                
                // High-cut sweeps down more gradually  
                float highCutFreq = 20000.0f - (effectiveAmount * 12000.0f); // 20kHz down to 8000Hz min (was 4000)
                
                // Wider minimum gap for less harsh filtering
                highCutFreq = juce::jmax(highCutFreq, lowCutFreq + 2000.0f); // Much wider gap (was 500)
                
                // Gentler resonance for smoother buildup character
                float sweepResonance = juce::jmin(resonanceAmount * 1.1f, 3.5f); // More controlled resonance
                setAllHighPassFilters(lowCutFreq, sweepResonance);
                setAllLowPassFilters(highCutFreq, sweepResonance);
                break;
            }
    }
}

bool BuildUpVerbAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BuildUpVerbAudioProcessor::createEditor()
{
    return new BuildUpVerbAudioProcessorEditor (*this);
}

void BuildUpVerbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void BuildUpVerbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void BuildUpVerbAudioProcessor::applyMacroControl(float macroValue, int mode) const
{
    // Different macro modes control parameters differently
    switch (mode)
    {
        case 1: // Subtle mode
            if (auto* param = parameters.getParameter("filterIntensity"))
                param->setValueNotifyingHost(macroValue * 0.7f);
            if (auto* param = parameters.getParameter("reverbMix"))
                param->setValueNotifyingHost(macroValue * 0.5f);
            if (auto* param = parameters.getParameter("stereoWidth"))
                param->setValueNotifyingHost(0.5f + macroValue * 0.5f); // 50-100%
            break;
            
        case 2: // Aggressive mode
            if (auto* param = parameters.getParameter("filterIntensity"))
                param->setValueNotifyingHost(macroValue);
            if (auto* param = parameters.getParameter("reverbMix"))
                param->setValueNotifyingHost(macroValue * 0.8f);
            if (auto* param = parameters.getParameter("noiseAmount"))
                param->setValueNotifyingHost(macroValue * macroValue * 0.5f); // Exponential
            if (auto* param = parameters.getParameter("filterResonance"))
                param->setValueNotifyingHost((3.0f - 0.5f) / (10.0f - 0.5f) + macroValue * 0.6f); // Map to normalized range
            if (auto* param = parameters.getParameter("stereoWidth"))
                param->setValueNotifyingHost(1.0f - macroValue * 0.7f); // 100% down to 30%
            break;
            
        case 3: // Epic mode
            if (auto* param = parameters.getParameter("filterIntensity"))
                param->setValueNotifyingHost(macroValue);
            if (auto* param = parameters.getParameter("reverbMix"))
                param->setValueNotifyingHost(macroValue);
            if (auto* param = parameters.getParameter("noiseAmount"))
                param->setValueNotifyingHost(macroValue * 0.7f);
            if (auto* param = parameters.getParameter("riserAmount"))
                param->setValueNotifyingHost(macroValue * macroValue); // Exponential riser
            if (auto* param = parameters.getParameter("tremoloDepth"))
                param->setValueNotifyingHost(macroValue * 0.6f);
            if (auto* param = parameters.getParameter("filterResonance"))
                param->setValueNotifyingHost((2.0f - 0.5f) / (10.0f - 0.5f) + macroValue * 0.8f); // Map to normalized range
            if (auto* param = parameters.getParameter("stereoWidth"))
                param->setValueNotifyingHost(1.0f - macroValue * 0.5f + macroValue * macroValue * 1.0f); // U-shape: 100% -> 50% -> 150%
            break;
            
        case 4: // Custom mode - user controls everything manually
            break;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BuildUpVerbAudioProcessor();
}