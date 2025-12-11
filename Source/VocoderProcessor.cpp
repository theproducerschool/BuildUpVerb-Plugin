// VocoderProcessor.cpp - True FFT-based vocoder implementation

#include "PluginProcessor.h"
#include <complex>
#include <cmath>

void BuildUpVerbAudioProcessor::processVocoder(juce::AudioBuffer<float>& buffer, 
                                               juce::AudioBuffer<float>& noiseBuffer,
                                               float vocoderGain,
                                               float vocoderRelease)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float sampleRate = getSampleRate();
    const float binHz = sampleRate / fftSize;
    
    // Debug timing (will be optimized out in release)
    static int debugCounter = 0;
    static int lastFFTSample = 0;
    
    // Clear output
    noiseBuffer.clear();
    
    // Try different overlap percentage
    const int testHopSize = fftSize / 8; // 87.5% overlap for testing
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* inputData = buffer.getReadPointer(channel);
        float* outputData = noiseBuffer.getWritePointer(channel);
        
        // Process all samples in this block
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            // Add input to circular buffer
            inputBuffer[channel * fftSize + inputWritePos[channel]] = inputData[sampleIndex];
            
            // Check if we need to process FFT
            if (++channelHopCounter[channel] >= testHopSize)
            {
                channelHopCounter[channel] = 0;
                
                // Debug: Check FFT timing consistency
                if (channel == 0)
                {
                    int samplesSinceLastFFT = debugCounter - lastFFTSample;
                    lastFFTSample = debugCounter;
                    
                    // This should always be testHopSize except for the first call
                    if (samplesSinceLastFFT != testHopSize && debugCounter > fftSize)
                    {
                        DBG("FFT timing inconsistent: " << samplesSinceLastFFT << " samples (expected " << testHopSize << ")");
                    }
                }
                
                // Prepare FFT input with proper windowing
                for (int i = 0; i < fftSize; ++i)
                {
                    int idx = (inputWritePos[channel] - fftSize + 1 + i + fftSize) % fftSize;
                    fftData[i] = inputBuffer[channel * fftSize + idx] * window[i];
                }
                
                // JUCE FFT expects data in a specific format
                // For real-only transform, it needs 2*fftSize array
                // First half is real data, second half is for complex results
                
                // Perform FFT
                forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());
                
                // Now fftData contains magnitude information
                // Calculate band levels (4 bands)
                const float bandFreqs[5] = {248.0f, 850.0f, 2900.0f, 10000.0f, 18000.0f};
                
                for (int band = 0; band < numBands; ++band)
                {
                    int startBin = static_cast<int>(bandFreqs[band] / binHz);
                    int endBin = static_cast<int>(bandFreqs[band + 1] / binHz);
                    
                    startBin = juce::jmax(1, startBin);
                    endBin = juce::jmin(fftSize / 2, endBin);
                    
                    // Sum magnitudes in band
                    float sum = 0.0f;
                    for (int bin = startBin; bin < endBin; ++bin)
                    {
                        sum += fftData[bin]; // performFrequencyOnlyForwardTransform gives magnitudes
                    }
                    
                    bandLevels[band] = sum / juce::jmax(1, endBin - startBin);
                }
                
                // Smooth band levels
                float attackMs = 0.5f;
                float releaseMs = 10.0f + vocoderRelease * 990.0f;
                
                float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate / testHopSize));
                float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate / testHopSize));
                
                for (int band = 0; band < numBands; ++band)
                {
                    if (bandLevels[band] > smoothedBandLevels[band])
                        smoothedBandLevels[band] += (bandLevels[band] - smoothedBandLevels[band]) * (1.0f - attackCoeff);
                    else
                        smoothedBandLevels[band] += (bandLevels[band] - smoothedBandLevels[band]) * (1.0f - releaseCoeff);
                }
                
                // Generate vocoded output directly (simple approach)
                // Create filtered noise based on band levels
                for (int i = 0; i < testHopSize; ++i)
                {
                    float sample = 0.0f;
                    
                    // Simple synthesis - sum filtered noise for each band
                    for (int band = 0; band < numBands; ++band)
                    {
                        // Generate band-limited noise
                        float noise = (random.nextFloat() - 0.5f) * 2.0f;
                        
                        // Simple band filtering simulation
                        // In real implementation, use proper bandpass filters
                        float bandContribution = noise * smoothedBandLevels[band];
                        
                        // Weight by frequency band (higher bands naturally quieter)
                        float bandWeight = 1.0f / (band + 1);
                        
                        sample += bandContribution * bandWeight;
                    }
                    
                    // Write to output buffer
                    int writePos = (outputReadPos[channel] + i) % fftSize;
                    outputBuffer[channel * fftSize + writePos] = sample * vocoderGain * 2.0f;
                }
                
                // Advance output position
                outputReadPos[channel] = (outputReadPos[channel] + testHopSize) % fftSize;
            }
            
            // Read from output buffer
            outputData[sampleIndex] = outputBuffer[channel * fftSize + outputReadPos[channel]];
            
            // Advance positions
            inputWritePos[channel] = (inputWritePos[channel] + 1) % fftSize;
            outputReadPos[channel] = (outputReadPos[channel] + 1) % fftSize;
            
            debugCounter++;
        }
    }
    
    // Debug: Check buffer alignment
    static int frameCount = 0;
    if (++frameCount % 100 == 0)
    {
        DBG("ProcessBlock size: " << numSamples << " samples");
        DBG("Hop size: " << testHopSize << " (" << (100.0f - (testHopSize * 100.0f / fftSize)) << "% overlap)");
    }
}