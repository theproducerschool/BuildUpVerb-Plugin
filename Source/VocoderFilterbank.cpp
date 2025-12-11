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
                                                        float vocoderRelease,
                                                        float vocoderBrightness)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float sampleRate = getSampleRate();
    
    // 4 bands - High-mids + crispy highs
    const float centerFreqs[4] = {1500.0f, 3000.0f, 6000.0f, 12000.0f};
    const float bandwidths[4] = {1.2f, 1.0f, 0.8f, 0.8f}; // Wider low bands for body
    
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
                synthesisBands[ch][i].setResonance(bandwidths[i] * 0.7f); // Lower Q for wider bands
                
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
            
            // MOSTLY raw white noise (90% mix) for maximum brightness
            float rawNoise = (random.nextFloat() - 0.5f) * 2.0f;
            
            // Variable high-pass based on brightness
            static float hpState[2] = {0.0f, 0.0f};
            float hpCutoff = 0.05f + vocoderBrightness * 0.25f; // More HP when brighter
            hpState[channel] += (rawNoise - hpState[channel]) * hpCutoff;
            float highpassedNoise = rawNoise - hpState[channel];
            
            // More high-passed noise when brighter
            float hpMix = 0.5f + vocoderBrightness * 0.4f; // 50-90% based on brightness
            noise = noise * (1.0f - hpMix) + highpassedNoise * hpMix;
            
            // Filter the SAME noise through all synthesis bands and modulate
            float output = 0.0f;
            for (int band = 0; band < 4; ++band)
            {
                // Filter the noise through synthesis band
                float filteredNoise = synthesisBands[channel][band].processSample(0, noise);
                
                // Modulate filtered noise with the envelope from analysis
                // Dynamic gain based on brightness parameter
                float bandGain = 1.0f;
                
                // Brightness morphs between warm and bright settings
                if (band == 0) // 1.5kHz
                    bandGain = 4.0f * (1.0f - vocoderBrightness) + 0.5f * vocoderBrightness;
                if (band == 1) // 3kHz
                    bandGain = 4.0f * (1.0f - vocoderBrightness) + 1.0f * vocoderBrightness;
                if (band == 2) // 6kHz
                    bandGain = 3.0f * (1.0f - vocoderBrightness) + 10.0f * vocoderBrightness;
                if (band == 3) // 12kHz
                    bandGain = 2.0f * (1.0f - vocoderBrightness) + 20.0f * vocoderBrightness;
                    
                output += filteredNoise * bandEnvelopes[band] * bandGain;
            }
            
            // Apply overall gain and extra output smoothing
            const float smoothCoeff = 0.95f; // Adjust for more/less smoothing
            outputSmooth[channel] += (output - outputSmooth[channel]) * (1.0f - smoothCoeff);
            // Multiple high-frequency emphasis stages
            static float highShelf1[2] = {0.0f, 0.0f};
            static float highShelf2[2] = {0.0f, 0.0f};
            
            // First emphasis stage
            float brightened = outputSmooth[channel] + (outputSmooth[channel] - highShelf1[channel]) * 1.0f;
            highShelf1[channel] = outputSmooth[channel];
            
            // Second emphasis stage - variable based on brightness
            float emphasisAmount = vocoderBrightness * 1.2f; // 0-120% emphasis
            float superBright = brightened + (brightened - highShelf2[channel]) * emphasisAmount;
            highShelf2[channel] = brightened;
            
            outputData[sample] = superBright * vocoderGain * 2.0f;
        }
    }
}