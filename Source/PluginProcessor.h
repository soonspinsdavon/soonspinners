#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "VariSpinEngine.h"

namespace ParamIDs
{
    constexpr auto speed       = "speed";
    constexpr auto glide       = "glide";
    constexpr auto spinDownAmt = "spinDownAmt";
    constexpr auto spinUpAmt   = "spinUpAmt";
    constexpr auto mix         = "mix";
    constexpr auto wobble      = "wobble";
    constexpr auto spinDownBtn = "spinDownBtn";
    constexpr auto spinUpBtn   = "spinUpBtn";
    constexpr auto halfTimeBtn = "halfTimeBtn";
}

class SoonSpinnerAudioProcessor : public juce::AudioProcessor
{
public:
    SoonSpinnerAudioProcessor();
    ~SoonSpinnerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SoonSpinner"; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    VariSpinEngine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoonSpinnerAudioProcessor)
};
