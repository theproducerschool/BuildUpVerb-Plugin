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
       parameters (*this, nullptr, juce::Identifier ("BuildUpVerb"), createParameterLayout())
{
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
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("reverbMix",
                                                             "Reverb Mix",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             50.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("noiseAmount",
                                                             "Noise Amount",
                                                             juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                             0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterChoice> ("noiseType",
                                                              "Noise Type",
                                                              juce::StringArray {"White", "Pink", "Vinyl"},
                                                              0));
    
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
                                                             juce::NormalisableRange<float> (0.5f, 10.0f, 0.01f, 0.5f),
                                                             2.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> ("stereoWidth",
                                                             "Stereo Width",
                                                             juce::NormalisableRange<float> (0.0f, 200.0f, 0.01f),
                                                             100.0f));
    
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
    
    highPassFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    
    // Initialize noise filter for noise sweep riser
    noiseFilter.prepare (spec);
    noiseFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    
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
    
    updateDSPFromParameters();
    
    // Get all parameters
    float buildUp = *parameters.getRawParameterValue ("buildup");
    
    // Get macro mode early for the control system
    int macroMode = (int)*parameters.getRawParameterValue ("macroMode");
    
    // Apply macro control if enabled
    if (macroMode > 0 && std::abs(buildUp - lastMacroValue) > 0.01f)
    {
        lastMacroValue = buildUp;
        applyMacroControl(buildUp / 100.0f, macroMode);
    }
    float filterIntensity = *parameters.getRawParameterValue ("filterIntensity");
    float reverbMix = *parameters.getRawParameterValue ("reverbMix");
    float noiseAmount = *parameters.getRawParameterValue ("noiseAmount");
    int noiseType = (int)*parameters.getRawParameterValue ("noiseType");
    float tremoloRate = *parameters.getRawParameterValue ("tremoloRate");
    float tremoloDepth = *parameters.getRawParameterValue ("tremoloDepth");
    float riserAmount = *parameters.getRawParameterValue ("riserAmount");
    int riserType = (int)*parameters.getRawParameterValue ("riserType");
    float riserRelease = *parameters.getRawParameterValue ("riserRelease");
    float stereoWidth = *parameters.getRawParameterValue ("stereoWidth");
    float noiseGate = *parameters.getRawParameterValue ("noiseGate");
    bool autoGain = *parameters.getRawParameterValue ("autoGain") > 0.5f;
    
    float buildUpNorm = buildUp / 100.0f;
    float filterIntensityNorm = filterIntensity / 100.0f;
    float reverbMixNorm = reverbMix / 100.0f;
    float noiseAmountNorm = noiseAmount / 100.0f;
    
    // Complete bypass when plugin should be transparent
    bool shouldBypass = (buildUpNorm < 0.001f && reverbMixNorm < 0.001f && 
                        noiseAmountNorm < 0.001f && riserAmount < 0.001f && 
                        tremoloDepth < 0.001f);
    
    if (shouldBypass)
    {
        // Plugin is completely transparent - no processing needed
        return;
    }
    
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
    
    // Create reverb buffer
    juce::AudioBuffer<float> reverbBuffer (buffer);
    
    // Process reverb
    juce::dsp::AudioBlock<float> reverbBlock (reverbBuffer);
    juce::dsp::ProcessContextReplacing<float> reverbContext (reverbBlock);
    freeverb.process (reverbBuffer);
    
    // Process intelligent filter automation (controlled by filter intensity)
    if (filterIntensityNorm > 0.01f && buildUpNorm > 0.01f)
    {
        int filterType = (int)*parameters.getRawParameterValue ("filterType");
        
        if (filterType == 2) // Band Pass - use dual-filter automation
        {
            // Apply both high-pass and low-pass for buildup automation effect
            highPassFilter.process (reverbContext);
            lowPassFilter.process (reverbContext);
        }
        else if (filterType == 0) // High Pass only
        {
            highPassFilter.process (reverbContext);
        }
        else if (filterType == 1) // Low Pass only
        {
            lowPassFilter.process (reverbContext);
        }
    }
    
    // Add intelligent noise that follows the signal
    // Use higher threshold to stop noise more aggressively
    if (noiseAmountNorm > 0.01f && buildUpNorm > 0.01f && smoothEnvelopeGate > 0.1f)
    {
        // Noise level based on build up, noise amount, AND signal presence
        float targetNoiseLevel = buildUpNorm * noiseAmountNorm * smoothEnvelopeGate * 0.05f; // Max 5% noise
        float noiseRampSpeed = 0.001f;
        
        for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
        {
            auto* data = reverbBuffer.getWritePointer (channel);
            
            for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
            {
                // Ramp noise level with smoother transitions
                if (currentNoiseLevel < targetNoiseLevel)
                    currentNoiseLevel += noiseRampSpeed * 0.5f; // Slower ramp up
                else if (currentNoiseLevel > targetNoiseLevel)
                    currentNoiseLevel -= noiseRampSpeed * 2.0f; // Faster ramp down
                
                float noise = 0.0f;
                
                // Apply intelligent noise gate based on input signal
                float gateValue = smoothEnvelopeGate;
                
                switch (noiseType)
                {
                    case 0: // White noise
                        noise = (random.nextFloat() * 2.0f - 1.0f) * currentNoiseLevel;
                        break;
                        
                    case 1: // Pink noise
                    {
                        float white = random.nextFloat() * 2.0f - 1.0f;
                        pinkNoiseState[0] = 0.99886f * pinkNoiseState[0] + white * 0.0555179f;
                        pinkNoiseState[1] = 0.99332f * pinkNoiseState[1] + white * 0.0750759f;
                        pinkNoiseState[2] = 0.96900f * pinkNoiseState[2] + white * 0.1538520f;
                        float pink = pinkNoiseState[0] + pinkNoiseState[1] + pinkNoiseState[2] + white * 0.5362f;
                        noise = pink * currentNoiseLevel * 0.11f; // Compensate for gain
                        break;
                    }
                        
                    case 2: // Vinyl crackle
                        if (random.nextFloat() < 0.02f) // Sparse crackles
                            noise = (random.nextFloat() * 2.0f - 1.0f) * currentNoiseLevel * 3.0f;
                        else
                            noise = (random.nextFloat() * 2.0f - 1.0f) * currentNoiseLevel * 0.3f;
                        break;
                }
                
                data[sample] += noise * gateValue; // Apply gate to noise
            }
        }
    }
    else
    {
        // Aggressively fade out noise when gate is closed
        if (currentNoiseLevel > 0.0f)
        {
            if (smoothEnvelopeGate < 0.1f) // Gate is essentially closed
            {
                currentNoiseLevel *= 0.3f; // EXTREMELY fast exponential decay when no signal
            }
            else
            {
                currentNoiseLevel *= 0.95f; // Normal fade out
            }
            
            if (currentNoiseLevel < 0.0001f)
                currentNoiseLevel = 0.0f;
        }
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
                
                for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
                {
                    auto* data = reverbBuffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
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
                
                for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
                {
                    auto* data = reverbBuffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
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
                
                for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
                {
                    auto* data = reverbBuffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
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
                juce::AudioBuffer<float> noiseBuffer(reverbBuffer.getNumChannels(), reverbBuffer.getNumSamples());
                
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
                for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
                {
                    reverbBuffer.addFrom(channel, 0, noiseBuffer, channel, 0, reverbBuffer.getNumSamples());
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
                
                for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
                {
                    auto* data = reverbBuffer.getWritePointer (channel);
                    
                    for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
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
    
    // Apply tremolo to wet signal only (before mixing with dry)
    if (tremoloDepth / 100.0f > 0.01f && buildUpNorm > 0.01f)
    {
        float tremDepth = (tremoloDepth / 100.0f) * buildUpNorm;
        float phaseIncrement = tremoloRate * 2.0f * juce::MathConstants<float>::pi / spec.sampleRate;
        
        for (int channel = 0; channel < reverbBuffer.getNumChannels(); ++channel)
        {
            auto* data = reverbBuffer.getWritePointer (channel);
            
            for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
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
    
    // Apply stereo width processing to wet signal
    if (buffer.getNumChannels() >= 2 && std::abs(stereoWidth - 100.0f) > 0.1f)
    {
        float width = stereoWidth / 100.0f;
        
        for (int sample = 0; sample < reverbBuffer.getNumSamples(); ++sample)
        {
            float left = reverbBuffer.getSample(0, sample);
            float right = reverbBuffer.getSample(1, sample);
            
            // M/S processing
            float mid = (left + right) * 0.5f;
            float side = (left - right) * 0.5f * width;
            
            reverbBuffer.setSample(0, sample, mid + side);
            reverbBuffer.setSample(1, sample, mid - side);
        }
    }
    
    // Mix dry and wet based on reverb mix and buildup
    // When buildUp is 0, we want 100% dry signal
    if (buildUpNorm < 0.001f)
    {
        // Soft reset - don't immediately reset phases to prevent clicks
        // Let the fade-outs handle the transitions
        
        // Only reset reverb buffers after a delay to prevent pops
        if (previousBuildUp > 0.001f)
        {
            // Just went to zero - start fade out but don't reset yet
        }
        else if (currentNoiseLevel < 0.0001f && std::abs(tremoloPhase) < 0.001f)
        {
            // Everything has faded out, safe to reset
            freeverb.reset();
            highPassFilter.reset();
            lowPassFilter.reset();
            noiseFilter.reset();
        }
        
        // Bypass all effects when buildUp is at 0
        return;
    }
    
    float wetLevel = buildUpNorm * reverbMixNorm;
    float dryLevel = 1.0f - wetLevel;
    
    // Safety check: ensure unity gain when no effects
    if (wetLevel < 0.001f)
    {
        // No reverb, just pass through dry signal
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            // Buffer already contains dry signal - no processing needed
        }
        return;
    }
    
    // Calculate auto gain compensation - much more subtle to prevent gain jumps
    float gainCompensation = 1.0f;
    if (autoGain)
    {
        // Very gentle gain reduction - only for extreme settings
        float effectsActive = 0.0f;
        effectsActive += wetLevel * 0.05f; // Much less aggressive
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
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* dry = buffer.getWritePointer (channel);
        auto* wet = reverbBuffer.getReadPointer (channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            dry[sample] = (dry[sample] * dryLevel + wet[sample] * wetLevel) * gainCompensation;
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
    float buildUpNorm = buildUp / 100.0f;
    float filterIntensityNorm = filterIntensity / 100.0f;
    
    // Update Freeverb parameters based on build up
    // Freeverb has different parameter ranges than JUCE reverb
    freeverb.setRoomSize(0.3f + (buildUpNorm * 0.65f));      // 0.3 to 0.95 (Freeverb sounds best 0.0-1.0)
    freeverb.setDamping(0.7f - (buildUpNorm * 0.5f));        // 0.7 to 0.2 (less damping = brighter)
    freeverb.setWetLevel(0.2f + (buildUpNorm * 0.6f));       // 0.2 to 0.8 (Freeverb can be quite strong)
    freeverb.setDryLevel(1.0f - (buildUpNorm * 0.4f));       // 1.0 to 0.6
    freeverb.setWidth(0.5f + (buildUpNorm * 0.5f));          // 0.5 to 1.0
    freeverb.setFreezeMode(0.0f);                            // No freeze
    
    // Simple linear filter automation for high/low pass, logarithmic only for bandpass
    float filterAmount = buildUpNorm * filterIntensityNorm;
    
    // Unity gain bypass when no filtering
    if (filterAmount < 0.001f)
    {
        // No filtering needed - filters stay at safe defaults
        highPassFilter.setCutoffFrequency(20.0f);  // No low cutting
        lowPassFilter.setCutoffFrequency(20000.0f); // No high cutting 
        highPassFilter.setResonance(0.5f); // Minimal resonance
        lowPassFilter.setResonance(0.5f);
        return;
    }
    
    // Basic resonance calculation
    float resonanceAmount = 0.5f + (filterResonance - 0.5f) * (1.0f + filterAmount * 2.0f);
    
    switch (filterType)
    {
        case 0: // High Pass only - simple linear sweep
            {
                highPassFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
                // Simple linear sweep from 20Hz to 2kHz
                float hpFreq = 20.0f + (filterAmount * 1980.0f); // 20Hz to 2000Hz
                highPassFilter.setCutoffFrequency (hpFreq);
                highPassFilter.setResonance (juce::jmin(resonanceAmount, 10.0f));
                break;
            }
            
        case 1: // Low Pass only - simple linear sweep  
            {
                lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
                // Simple linear sweep from 20kHz down to 500Hz
                float lpFreq = 20000.0f - (filterAmount * 19500.0f); // 20kHz down to 500Hz
                lowPassFilter.setCutoffFrequency (lpFreq);
                lowPassFilter.setResonance (juce::jmin(resonanceAmount, 10.0f));
                break;
            }
            
        case 2: // Dual Sweep - GENTLER, MORE MUSICAL PROGRESSION
            {
                // Much gentler curve for smoother filtering
                float curve = std::pow(filterAmount, 2.0f); // Quadratic curve - starts very gentle
                
                // Use mostly curved component for smoother onset
                float effectiveAmount = curve;
                
                highPassFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
                lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
                
                // Much gentler sweep ranges
                // Low-cut sweeps up more gradually
                float lowCutFreq = 20.0f + (effectiveAmount * 3000.0f); // 20Hz to 3020Hz max (was 7000)
                
                // High-cut sweeps down more gradually  
                float highCutFreq = 20000.0f - (effectiveAmount * 12000.0f); // 20kHz down to 8000Hz min (was 4000)
                
                // Wider minimum gap for less harsh filtering
                highCutFreq = juce::jmax(highCutFreq, lowCutFreq + 2000.0f); // Much wider gap (was 500)
                
                highPassFilter.setCutoffFrequency (lowCutFreq);  // Low-cut filter
                lowPassFilter.setCutoffFrequency (highCutFreq);   // High-cut filter
                
                // Gentler resonance for smoother buildup character
                float sweepResonance = juce::jmin(resonanceAmount * 1.2f, 8.0f); // Reduced from 1.5x/12 to 1.2x/8
                highPassFilter.setResonance (sweepResonance);
                lowPassFilter.setResonance (sweepResonance);
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