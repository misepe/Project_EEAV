/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider 
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
	
};

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ResponseCurveComponent(Project_EEAVAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    void timerCallback() override;

    void paint(juce::Graphics& g) override;
private:
    Project_EEAVAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };

	MonoChain monoChain;
};

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


	CustomRotarySlider peakFreqSlider,
        peakGainSlider,
		peakQualitySlider,
		lowCutFreqSlider,
		highCutFreqSlider,
        lowCutSlopeSlider,
		highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;

	Attachment peakFreqSliderAttachment, 
               peakGainSliderAttachment, 
               peakQualitySliderAttachment, 
               lowCutFreqSliderAttachment, 
               highCutFreqSliderAttachment, 
               lowCutSlopeSliderAttachment, 
               highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Project_EEAVAudioProcessorEditor)
};
