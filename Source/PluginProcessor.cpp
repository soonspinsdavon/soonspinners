#include "PluginProcessor.h"
#include "PluginEditor.h"

SoonSpinnerAudioProcessor::SoonSpinnerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SoonSpinnerAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::speed, 1 }, "Speed",
        NormalisableRange<float> (0.25f, 4.0f, 0.001f, 0.4f), 1.0f,
        AudioParameterFloatAttributes().withLabel ("x")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::glide, 1 }, "Glide Time",
        NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.4f), 200.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::spinDownAmt, 1 }, "Spin Down Amount",
        NormalisableRange<float> (0.1f, 1.0f, 0.001f), 0.5f,
        AudioParameterFloatAttributes().withLabel ("x")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::spinUpAmt, 1 }, "Spin Up Amount",
        NormalisableRange<float> (1.0f, 4.0f, 0.001f), 2.0f,
        AudioParameterFloatAttributes().withLabel ("x")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::mix, 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::wobble, 1 }, "Wobble",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f,
        AudioParameterFloatAttributes().withLabel ("%")));

    // Momentary triggers - exposed as automatable/MIDI-mappable bool
    // params so they can be driven from a DAW automation lane or a MIDI
    // controller, same as Vari-Fi's MIDI trigger option.
    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::spinDownBtn, 1 }, "Spin Down", false));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::spinUpBtn, 1 }, "Spin Up", false));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::halfTimeBtn, 1 }, "Half Time", false));

    return { params.begin(), params.end() };
}

void SoonSpinnerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

bool SoonSpinnerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void SoonSpinnerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any extra output channels a host might have allocated beyond
    // what we declared inputs for.
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.setBaseSpeed  (apvts.getRawParameterValue (ParamIDs::speed)->load());
    engine.setGlideTimeMs (apvts.getRawParameterValue (ParamIDs::glide)->load());
    engine.setMix         (apvts.getRawParameterValue (ParamIDs::mix)->load());
    engine.setWobbleAmount (apvts.getRawParameterValue (ParamIDs::wobble)->load());

    const bool downHeld = apvts.getRawParameterValue (ParamIDs::spinDownBtn)->load() > 0.5f;
    const bool upHeld   = apvts.getRawParameterValue (ParamIDs::spinUpBtn)->load() > 0.5f;

    engine.setSpinDown (downHeld, apvts.getRawParameterValue (ParamIDs::spinDownAmt)->load());
    engine.setSpinUp   (upHeld,   apvts.getRawParameterValue (ParamIDs::spinUpAmt)->load());

    const bool halfTimeOn = apvts.getRawParameterValue (ParamIDs::halfTimeBtn)->load() > 0.5f;
    engine.setHalfTimeEnabled (halfTimeOn);

    // Pull tempo/beat position from the host so Half Time can lock to the
    // beat grid. Falls back to a free-running 120bpm clock (handled inside
    // the engine) if the host doesn't report a playhead position.
    double bpm = 120.0;
    double ppq = 0.0;
    double cycleLengthBeats = 4.0; // one bar in 4/4, refined below if the host reports otherwise
    bool hostProvidesPosition = false;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpmOpt = position->getBpm())
                bpm = *bpmOpt;

            if (auto ppqOpt = position->getPpqPosition())
            {
                ppq = *ppqOpt;
                hostProvidesPosition = true;
            }

            if (auto timeSig = position->getTimeSignature())
                cycleLengthBeats = timeSig->numerator * (4.0 / (double) timeSig->denominator);
        }
    }

    engine.updateTempoSync (bpm, ppq, hostProvidesPosition, cycleLengthBeats);

    engine.process (buffer);
}

juce::AudioProcessorEditor* SoonSpinnerAudioProcessor::createEditor()
{
    return new SoonSpinnerAudioProcessorEditor (*this);
}

void SoonSpinnerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SoonSpinnerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoonSpinnerAudioProcessor();
}
