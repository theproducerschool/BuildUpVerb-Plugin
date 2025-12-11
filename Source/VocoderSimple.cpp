// VocoderSimple.cpp - 4-band vocoder like Ableton

#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>

void BuildUpVerbAudioProcessor::processVocoderSimple(juce::AudioBuffer<float>& buffer, 
                                                    juce::AudioBuffer<float>& noiseBuffer,
                                                    float vocoderGain,
                                                    float vocoderRelease)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    // Static envelope follower state
    static float envelope[2] = {0.0f, 0.0f};
    
    // Smoother coefficients to prevent ringing
    float attackCoeff = 0.01f;  // Much smoother attack to prevent ringing
    float releaseCoeff = 0.0001f * (1.0f - vocoderRelease * 0.9f); // Smoother release
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* inputData = buffer.getReadPointer(channel);
        float* outputData = noiseBuffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Get input magnitude
            float inputMag = std::abs(inputData[sample]);
            
            // Simple envelope follower
            if (inputMag > envelope[channel])
                envelope[channel] += (inputMag - envelope[channel]) * attackCoeff;
            else
                envelope[channel] += (inputMag - envelope[channel]) * releaseCoeff;
            
            // Generate simple white noise
            float noise = (random.nextFloat() - 0.5f) * 2.0f;
            
            // Apply envelope to noise with smoothing
            outputData[sample] = noise * envelope[channel] * vocoderGain * 2.0f;
        }
    }
}