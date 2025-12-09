#include "FreeverbWrapper.h"

FreeverbWrapper::FreeverbWrapper()
{
    // Initialize with default values
    model.setroomsize(0.5f);
    model.setdamp(0.5f);
    model.setwet(0.3f);
    model.setdry(0.7f);
    model.setwidth(1.0f);
    model.setmode(0.0f);
}

void FreeverbWrapper::prepare(double sampleRate, int maximumBlockSize)
{
    currentSampleRate = (int)sampleRate;
    
    // Ensure we have a stereo buffer for processing
    stereoBuffer.setSize(2, maximumBlockSize);
    
    // Reset the reverb model
    reset();
}

void FreeverbWrapper::reset()
{
    model.mute();
    stereoBuffer.clear();
}

void FreeverbWrapper::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    // Freeverb expects stereo input, so we need to handle mono/stereo cases
    if (numChannels == 1)
    {
        // Mono input - duplicate to stereo
        stereoBuffer.setSize(2, numSamples, false, false, true);
        stereoBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        stereoBuffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        
        // Process through Freeverb
        model.processreplace(stereoBuffer.getWritePointer(0),
                           stereoBuffer.getWritePointer(1),
                           stereoBuffer.getWritePointer(0),
                           stereoBuffer.getWritePointer(1),
                           numSamples, 1);
        
        // Mix back to mono
        buffer.copyFrom(0, 0, stereoBuffer, 0, 0, numSamples);
        buffer.applyGain(0, 0, numSamples, 0.5f);
        buffer.addFrom(0, 0, stereoBuffer, 1, 0, numSamples, 0.5f);
    }
    else if (numChannels >= 2)
    {
        // Stereo or multi-channel - process first two channels
        stereoBuffer.setSize(2, numSamples, false, false, true);
        stereoBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        stereoBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);
        
        // Process through Freeverb
        model.processreplace(stereoBuffer.getWritePointer(0),
                           stereoBuffer.getWritePointer(1),
                           stereoBuffer.getWritePointer(0),
                           stereoBuffer.getWritePointer(1),
                           numSamples, 1);
        
        // Copy back to buffer
        buffer.copyFrom(0, 0, stereoBuffer, 0, 0, numSamples);
        buffer.copyFrom(1, 0, stereoBuffer, 1, 0, numSamples);
    }
}