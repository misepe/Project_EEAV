/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Project_EEAVAudioProcessor::Project_EEAVAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

Project_EEAVAudioProcessor::~Project_EEAVAudioProcessor()
{
}

//==============================================================================
const juce::String Project_EEAVAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Project_EEAVAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Project_EEAVAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Project_EEAVAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Project_EEAVAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Project_EEAVAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Project_EEAVAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Project_EEAVAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Project_EEAVAudioProcessor::getProgramName (int index)
{
    return {};
}

void Project_EEAVAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Project_EEAVAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

	spec.sampleRate = sampleRate;

	leftChain.prepare(spec);
	rightChain.prepare(spec);

	updateFilters();

}

void Project_EEAVAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Project_EEAVAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Project_EEAVAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

	juce::dsp::AudioBlock<float> block(buffer);

	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rigthContext(rightBlock);

	leftChain.process(leftContext);
	rightChain.process(rigthContext);



}

//==============================================================================
bool Project_EEAVAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Project_EEAVAudioProcessor::createEditor()
{
    return new Project_EEAVAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Project_EEAVAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos (destData, true);
	apvts.state.writeToStream(mos);
}

void Project_EEAVAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
		apvts.replaceState(tree);
		updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) 
{

	ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
	settings.filterName = static_cast<FilterType> (apvts.getRawParameterValue("Choose filter")->load());
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

	return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
	return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
		chainSettings.peakFreq,
		chainSettings.peakQuality,
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

Coefficients makeNotchFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality);
}

Coefficients makeBandPassFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality);
}

Coefficients makeChooseFilter(const ChainSettings& chainSettings, double sampleRate)
{
    switch (chainSettings.filterName)
    {
    case PeakFilter:
        return makePeakFilter(chainSettings, sampleRate);
    case NotchFilter:
        return makeNotchFilter(chainSettings, sampleRate);
    case BandPassFilter:
        return makeBandPassFilter(chainSettings, sampleRate);
    default:
        jassertfalse; // Invalid filter type
        return {};
    }
}

void Project_EEAVAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
	auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

	updateCoefficients(leftChain.get<ChainPositions::Choose>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Choose>().coefficients, peakCoefficients);
}

void Project_EEAVAudioProcessor::updateNotchFilter(const ChainSettings& chainSettings)
{
    auto nocthCoefficients = makeNotchFilter(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::Choose>().coefficients, nocthCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Choose>().coefficients, nocthCoefficients);

}

void Project_EEAVAudioProcessor::updateBandPassFilter(const ChainSettings& chainSettings)
{

    auto bandPassCoefficients = makeBandPassFilter(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::Choose>().coefficients, bandPassCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Choose>().coefficients, bandPassCoefficients);
}

void updateCoefficients(Coefficients & old, const Coefficients & replacements)
{
	*old = *replacements;
}

void Project_EEAVAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings) 
{
	auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, lowCutCoefficients, static_cast<Slope>(chainSettings.lowCutSlope));
    updateCutFilter(rightLowCut, lowCutCoefficients, static_cast<Slope>(chainSettings.lowCutSlope));
}

void Project_EEAVAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
	auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, static_cast<Slope>(chainSettings.highCutSlope));
    updateCutFilter(rightHighCut, highCutCoefficients, static_cast<Slope>(chainSettings.highCutSlope));
}

void Project_EEAVAudioProcessor::updateChooseFilter(const ChainSettings& chainSettings) 
{

    switch (chainSettings.filterName)
    {
    case PeakFilter:
    {
		DBG("Peak Filter Selected");
        updatePeakFilter(chainSettings);
        break;
    }
    case NotchFilter:
    {
		DBG("Notch Filter Selected");
        updateNotchFilter(chainSettings);
        break;
    }
    case BandPassFilter:
    {
        DBG("Band Pass Filter Selected");
        updateBandPassFilter(chainSettings);
        break;
    }
    }
}

void Project_EEAVAudioProcessor::updateFilters() 
{
	auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
	updateChooseFilter(chainSettings);
	updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout Project_EEAVAudioProcessor::createParameterLayout() 
{
    //SPEC:
    //  -3bands: low,high,Parametric/Peak
    //  Cut bands:Controllable Frecuency/Slope
    //  Parametric bands: Controllable Frequency, Gain, Quality

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 
                                                           20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20000.f));
    juce::StringArray filterNames;
    filterNames.add("Peak");
    filterNames.add("Notch");
    filterNames.add("BandPass");

    layout.add(std::make_unique<juce::AudioParameterChoice>("Choose filter", "Choose filter", filterNames, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                            "Peak Freq",
                                                            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                            750.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                            "Peak Gain",
                                                            juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                            0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                            "Peak Quality",
                                                            juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                            1.f));
  
    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12+ i*12);
        str << "db/Oct";
        stringArray.add(str);
    }

	layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope","LowCut Slope",stringArray,0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Project_EEAVAudioProcessor();
}
