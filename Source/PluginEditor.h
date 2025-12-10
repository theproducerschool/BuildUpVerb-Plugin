#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class KnobComponent;

class BuildUpVerbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    BuildUpVerbAudioProcessorEditor (BuildUpVerbAudioProcessor&);
    ~BuildUpVerbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    BuildUpVerbAudioProcessor& audioProcessor;
    std::unique_ptr<KnobComponent> knobComp;
    
    void initialiseWebView();
    void sendParameterUpdate();
    void loadWebContent();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuildUpVerbAudioProcessorEditor)
};