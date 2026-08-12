#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
    Placeholder Soonspins theme: lowercase everything (matches
    soonspins.com's nav/labels), dark base with the site's orange accent,
    and a monospaced placeholder font. Swap SoonSpinsLookAndFeel::brandFont
    for an embedded brand typeface once we know the exact one soonspins.com
    uses - see the TODO on getBrandFont() below.
*/
class SoonSpinsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SoonSpinsLookAndFeel();

    // TODO: this is a placeholder (system monospace) until we confirm the
    // real soonspins.com font. Once we have a name or a .ttf/.otf, embed it
    // as BinaryData (like the background asset) and swap the body here.
    static juce::Font getBrandFont (float height, juce::Font::FontStyleFlags style = juce::Font::plain);

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

class SoonSpinnerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit SoonSpinnerAudioProcessorEditor (SoonSpinnerAudioProcessor&);
    ~SoonSpinnerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider& configureKnob (juce::Slider& knob, juce::Label& label, const juce::String& text);
    void drawParallaxBackground (juce::Graphics&);
    void timerCallback() override;

    SoonSpinnerAudioProcessor& processorRef;
    SoonSpinsLookAndFeel lookAndFeel;

    juce::Image cloudImage;
    double scrollPixels = 0.0; // continuously increasing, drives the left-to-right drift - mouse has no effect on this

    juce::Slider speedKnob, glideKnob, spinDownAmtKnob, spinUpAmtKnob, mixKnob, wobbleKnob;
    juce::Label  speedLabel, glideLabel, spinDownAmtLabel, spinUpAmtLabel, mixLabel, wobbleLabel;

    juce::TextButton spinDownButton { "spin down" };
    juce::TextButton halfTimeButton { "half time" };
    juce::TextButton spinUpButton   { "spin up" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> speedAttachment, glideAttachment, spinDownAmtAttachment,
                                       spinUpAmtAttachment, mixAttachment, wobbleAttachment;
    std::unique_ptr<ButtonAttachment> spinDownAttachment, spinUpAttachment, halfTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoonSpinnerAudioProcessorEditor)
};
