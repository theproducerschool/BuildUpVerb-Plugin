#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FreeverbWrapper.h"
#include <complex>
#include <array>

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
    
    // Vocoder processing
    void processVocoder(juce::AudioBuffer<float>& buffer, 
                       juce::AudioBuffer<float>& noiseBuffer,
                       float vocoderGain,
                       float vocoderRelease,
                       float vocoderBrightness = 0.5f);
                       
    // Alternative filterbank vocoder
    void processVocoderFilterbank(juce::AudioBuffer<float>& buffer, 
                                 juce::AudioBuffer<float>& noiseBuffer,
                                 float vocoderGain,
                                 float vocoderRelease,
                                 float vocoderBrightness = 0.5f);
                                 
    // Simple vocoder for debugging
    void processVocoderSimple(juce::AudioBuffer<float>& buffer, 
                             juce::AudioBuffer<float>& noiseBuffer,
                             float vocoderGain,
                             float vocoderRelease,
                             float vocoderBrightness = 0.5f);
                             
    // Gate-based vocoder (no envelope following)
    void processVocoderGated(juce::AudioBuffer<float>& buffer, 
                            juce::AudioBuffer<float>& noiseBuffer,
                            float vocoderGain,
                            float vocoderRelease,
                            float vocoderBrightness = 0.5f);
    
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
    mutable float smoothedBuildUp = 0.0f;  // Smoothed build up value
    mutable float currentNoiseLevel = 0.0f;
    mutable float smoothedNoiseLevel = 0.0f;  // Smoothed noise level to prevent clicks
    mutable float smoothedVocoderLevel = 0.0f;  // Extra smoothing for vocoder
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
    
    // FFT for vocoder
    static constexpr int fftOrder = 10; // 1024 samples
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBands = 4; // 4 bands like Ableton
    
    juce::dsp::FFT forwardFFT;
    std::vector<float> fftData;
    std::vector<float> window;
    std::vector<std::complex<float>> frequencyData;
    std::vector<float> magnitudes;
    std::vector<float> phases;
    std::vector<float> bandLevels;
    std::vector<float> smoothedBandLevels;
    int fftPos = 0;
    int hopSize = fftSize / 4; // 75% overlap for smoother output
    int hopCounter = 0;
    std::vector<float> overlapBuffer;
    std::vector<float> prevPhases;
    std::vector<float> inputBuffer;  // Circular buffer for input
    std::vector<float> outputBuffer; // Circular buffer for output
    std::array<int, 2> inputWritePos = {0, 0};  // Per-channel positions
    std::array<int, 2> outputReadPos = {0, 0};  // Per-channel positions  
    std::array<int, 2> channelHopCounter = {0, 0}; // Per-channel hop counter
    
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
    
    juce::AudioBuffer<float> fftInputBuffer;
    juce::AudioBuffer<float> fftOutputBuffer;
    
    void updateDSPFromParameters();
    void applyMacroControl(float macroValue, int mode) const;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuildUpVerbAudioProcessor)
};