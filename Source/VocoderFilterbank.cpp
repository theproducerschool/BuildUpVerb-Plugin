// VocoderFilterbank.cpp - Filterbank vocoder implementation (more stable than FFT)

#include "PluginProcessor.h"
#include <cmath>

// Simple envelope follower class
class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;
    
    void setSampleRate(float sr) 
    { 
        sampleRate = sr; 
        updateCoefficients();
    }
    
    void setAttackMs(float ms) 
    { 
        attackMs = ms; 
        updateCoefficients();
    }
    
    void setReleaseMs(float ms) 
    { 
        releaseMs = ms; 
        updateCoefficients();
    }
    
    float process(float input)
    {
        float rectified = std::abs(input);
        
        if (rectified > envelope)
            envelope += (rectified - envelope) * attackCoeff;
        else
            envelope += (rectified - envelope) * releaseCoeff;
            
        return envelope;
    }
    
    void reset() { envelope = 0.0f; }
    
private:
    float envelope = 0.0f;
    float sampleRate = 44100.0f;
    float attackMs = 1.0f;
    float releaseMs = 10.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    
    void updateCoefficients()
    {
        attackCoeff = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        releaseCoeff = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    }
};

// Simple white noise generator with smoothing
class SmoothNoiseGenerator
{
public:
    void reset() 
    { 
        z1 = 0.0f;
        z2 = 0.0f;
        z3 = 0.0f;
    }
    
    float process(juce::Random& random)
    {
        // Generate white noise
        float white = (random.nextFloat() - 0.5f) * 2.0f;
        
        // Apply 3-pole lowpass filter for smoother noise
        // This removes harsh high frequencies
        const float cutoff = 0.15f; // Adjust for smoothness
        
        z1 += (white - z1) * cutoff;
        z2 += (z1 - z2) * cutoff;
        z3 += (z2 - z3) * cutoff;
        
        // Mix filtered and original for controlled brightness
        return z3 * 0.7f + white * 0.3f;
    }
    
private:
    float z1 = 0.0f, z2 = 0.0f, z3 = 0.0f;
};

// Filterbank vocoder implementation
void BuildUpVerbAudioProcessor::processVocoderFilterbank(juce::AudioBuffer<float>& buffer, 
                                                        juce::AudioBuffer<float>& noiseBuffer,
                                                        float vocoderGain,
                                                        float vocoderRelease)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float sampleRate = getSampleRate();
    
    // 4 bands like Ableton
    const float centerFreqs[4] = {400.0f, 1200.0f, 3600.0f, 10800.0f};
    const float bandwidths[4] = {1.5f, 1.5f, 1.5f, 1.5f}; // Q values
    
    // Static filters and envelopes (should be member variables in real implementation)
    static juce::dsp::StateVariableTPTFilter<float> analysisBands[2][4]; // Stereo
    static juce::dsp::StateVariableTPTFilter<float> synthesisBands[2][4]; // Stereo
    static EnvelopeFollower envelopes[2][4]; // Stereo
    static SmoothNoiseGenerator noiseGen[2]; // Stereo
    static float outputSmooth[2] = {0.0f, 0.0f}; // Output smoothing
    static bool initialized = false;
    
    // Initialize on first use
    if (!initialized)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 4; ++i)
            {
                analysisBands[ch][i].prepare(spec);
                analysisBands[ch][i].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                analysisBands[ch][i].setCutoffFrequency(centerFreqs[i]);
                analysisBands[ch][i].setResonance(bandwidths[i]);
                
                synthesisBands[ch][i].prepare(spec);
                synthesisBands[ch][i].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                synthesisBands[ch][i].setCutoffFrequency(centerFreqs[i]);
                synthesisBands[ch][i].setResonance(bandwidths[i]);
                
                envelopes[ch][i].setSampleRate(sampleRate);
                envelopes[ch][i].setAttackMs(0.5f);
                envelopes[ch][i].setReleaseMs(10.0f + vocoderRelease * 990.0f);
            }
            noiseGen[ch].reset();
        }
        initialized = true;
    }
    
    // Update release times
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < 4; ++i)
        {
            envelopes[ch][i].setReleaseMs(10.0f + vocoderRelease * 990.0f);
        }
    }
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* inputData = buffer.getReadPointer(channel);
        float* outputData = noiseBuffer.getWritePointer(channel);
        
        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = inputData[sample];
            
            // Analyze input through filter bands and get envelopes
            float bandEnvelopes[4];
            for (int band = 0; band < 4; ++band)
            {
                // Filter input signal through analysis band
                float filtered = analysisBands[channel][band].processSample(0, input);
                
                // Get envelope of filtered signal
                bandEnvelopes[band] = envelopes[channel][band].process(filtered);
            }
            
            // Generate ONE smooth noise source
            float noise = noiseGen[channel].process(random);
            
            // Filter the SAME noise through all synthesis bands and modulate
            float output = 0.0f;
            for (int band = 0; band < 4; ++band)
            {
                // Filter the noise through synthesis band
                float filteredNoise = synthesisBands[channel][band].processSample(0, noise);
                
                // Modulate filtered noise with the envelope from analysis
                output += filteredNoise * bandEnvelopes[band] * 2.0f;
            }
            
            // Apply overall gain and extra output smoothing
            const float smoothCoeff = 0.95f; // Adjust for more/less smoothing
            outputSmooth[channel] += (output - outputSmooth[channel]) * (1.0f - smoothCoeff);
            outputData[sample] = outputSmooth[channel] * vocoderGain;
        }
    }
}