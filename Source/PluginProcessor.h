#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FreeverbWrapper.h"

class BuildUpVerbAudioProcessor : public juce::AudioProcessor
{
public:
    BuildUpVerbAudioProcessor();
    ~BuildUpVerbAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    // Preset structure
    struct Preset
    {
        juce::String name;
        float buildUp;
        float filterIntensity;
        float reverbMix;
        float noiseAmount;
        int noiseType;
        float tremoloRate;
        float tremoloDepth;
        float riserAmount;
        int riserType;
    };
    
    static const int numPresets = 10;
    static const Preset factoryPresets[numPresets];

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    
private:
    FreeverbWrapper freeverb;
    juce::dsp::StateVariableTPTFilter<float> highPassFilter;
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter; // For dual-filter automation
    // Additional filter stages for steeper slopes (6dB = 1 stage, 12dB = 2, 18dB = 3, 24dB = 4)
    juce::dsp::StateVariableTPTFilter<float> highPassFilter2;
    juce::dsp::StateVariableTPTFilter<float> highPassFilter3;
    juce::dsp::StateVariableTPTFilter<float> highPassFilter4;
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter2;
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter3;
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter4;
    juce::dsp::ProcessSpec spec;
    juce::Random random;
    
    float previousBuildUp = 0.0f;
    mutable float currentNoiseLevel = 0.0f;
    mutable float smoothedNoiseLevel = 0.0f;  // Smoothed noise level to prevent clicks
    int currentPreset = 0;
    
    // Tremolo
    mutable float tremoloPhase = 0.0f;
    
    // Riser
    mutable float riserFreq = 100.0f;
    mutable float riserPhase = 0.0f;
    mutable float sawPhase = 0.0f;
    mutable float squarePhase = 0.0f;
    mutable float subPhase = 0.0f;
    mutable float smoothSubFreq = 30.0f;
    mutable float currentRiserLevel = 0.0f;
    mutable float smoothSawFreq = 100.0f;
    mutable float smoothSquareFreq = 100.0f;
    juce::dsp::StateVariableTPTFilter<float> noiseFilter;
    float lastBuildUp = 0.0f;
    
    // Pink noise filter
    mutable float pinkNoiseState[3] = {0.0f, 0.0f, 0.0f};
    
    // Stereo width
    float widthDelayL = 0.0f;
    float widthDelayR = 0.0f;
    
    // Noise gate
    mutable float gateEnvelope = 0.0f;
    mutable float gatePhase = 0.0f;
    
    // Auto gain
    mutable float currentGainReduction = 1.0f;
    
    // Macro control
    mutable float lastMacroValue = -1.0f;
    
    // Envelope follower for intelligent noise gating
    mutable float envelopeLevel = 0.0f;
    mutable float noiseGateThreshold = 0.001f; // -60dB threshold
    
    // Delay processing
    juce::AudioBuffer<float> delayBufferL, delayBufferR;
    int delayWritePos = 0;
    float currentBPM = 120.0f;
    int delaySampleLength = 0;
    
    void updateDSPFromParameters();
    void applyMacroControl(float macroValue, int mode) const;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuildUpVerbAudioProcessor)
};