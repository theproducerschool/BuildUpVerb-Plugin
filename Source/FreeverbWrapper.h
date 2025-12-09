#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "revmodel.hpp"

class FreeverbWrapper
{
public:
    FreeverbWrapper();
    ~FreeverbWrapper() = default;
    
    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameter setters matching JUCE reverb interface
    void setRoomSize(float value) { model.setroomsize(value); }
    void setDamping(float value) { model.setdamp(value); }
    void setWetLevel(float value) { model.setwet(value); }
    void setDryLevel(float value) { model.setdry(value); }
    void setWidth(float value) { model.setwidth(value); }
    void setFreezeMode(float value) { model.setmode(value); }
    
    // Parameter getters
    float getRoomSize() { return model.getroomsize(); }
    float getDamping() { return model.getdamp(); }
    float getWetLevel() { return model.getwet(); }
    float getDryLevel() { return model.getdry(); }
    float getWidth() { return model.getwidth(); }
    float getFreezeMode() { return model.getmode(); }
    
private:
    revmodel model;
    juce::AudioBuffer<float> stereoBuffer;
    int currentSampleRate = 44100;
};