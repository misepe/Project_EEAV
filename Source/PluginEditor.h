/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class Project_EEAVAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Project_EEAVAudioProcessorEditor (Project_EEAVAudioProcessor&);
    ~Project_EEAVAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Project_EEAVAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Project_EEAVAudioProcessorEditor)
};
