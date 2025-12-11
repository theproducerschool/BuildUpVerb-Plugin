// VocoderGated.cpp - Gate-based vocoder (no envelope following)

#include "PluginProcessor.h"
#include <cmath>

void BuildUpVerbAudioProcessor::processVocoderGated(juce::AudioBuffer<float>& buffer, 
                                                   juce::AudioBuffer<float>& noiseBuffer,
                                                   float vocoderGain,
                                                   float vocoderRelease,
                                                   float vocoderBrightness)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    // Static gate state
    static float gateLevel[2] = {0.0f, 0.0f};
    static float noiseLevel[2] = {0.0f, 0.0f};
    
    // Gate threshold
    const float threshold = 0.001f; // -60dB
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* inputData = buffer.getReadPointer(channel);
        float* outputData = noiseBuffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Simple gate - is input above threshold?
            float inputMag = std::abs(inputData[sample]);
            float targetGate = (inputMag > threshold) ? 1.0f : 0.0f;
            
            // Smooth the gate transition
            const float gateSmooth = 0.95f; // Heavy smoothing
            gateLevel[channel] += (targetGate - gateLevel[channel]) * (1.0f - gateSmooth);
            
            // Generate smooth noise (lowpass filtered white noise)
            static float noiseZ1[2] = {0.0f, 0.0f};
            float white = (random.nextFloat() - 0.5f) * 2.0f;
            noiseZ1[channel] += (white - noiseZ1[channel]) * 0.1f; // Lowpass
            
            // Apply gate to noise with additional smoothing
            float targetNoiseLevel = noiseZ1[channel] * gateLevel[channel];
            const float outputSmooth = 0.99f; // Very heavy output smoothing
            noiseLevel[channel] += (targetNoiseLevel - noiseLevel[channel]) * (1.0f - outputSmooth);
            
            // TEST: Output actual WHITE NOISE at constant level
            float whiteNoise = (random.nextFloat() - 0.5f) * 2.0f;
            outputData[sample] = whiteNoise * 0.2f; // Constant amplitude white noise
        }
    }
}