#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

namespace SoonspinsColours
{
    // Pulled from the halftone-cloud reference art / soonspins.com's vinyl
    // logo. Update these if the real brand kit lands with different hexes.
    const juce::Colour background   { 0xff0d0e12 };
    const juce::Colour panel        { 0xff1b1c22 };
    const juce::Colour accent       { 0xffff9e42 }; // soonspins orange
    const juce::Colour accentDim    { 0xff8a4a1a };
    const juce::Colour text         { 0xffe8e6e1 };
}

//==============================================================================
SoonSpinsLookAndFeel::SoonSpinsLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, SoonspinsColours::background);
    setColour (juce::Slider::rotarySliderFillColourId, SoonspinsColours::accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, SoonspinsColours::panel);
    setColour (juce::Slider::thumbColourId, SoonspinsColours::accent);
    setColour (juce::Slider::textBoxTextColourId, SoonspinsColours::text);
    setColour (juce::Label::textColourId, SoonspinsColours::text);
    setColour (juce::TextButton::buttonColourId, SoonspinsColours::panel);
    setColour (juce::TextButton::buttonOnColourId, SoonspinsColours::accent);
    setColour (juce::TextButton::textColourOffId, SoonspinsColours::text);
    setColour (juce::TextButton::textColourOnId, SoonspinsColours::background);
}

juce::Font SoonSpinsLookAndFeel::getBrandFont (float height, juce::Font::FontStyleFlags style)
{
    // Placeholder: clean system monospace, matches the "lowercase,
    // no-frills" feel of soonspins.com until we confirm their actual
    // webfont and embed it as BinaryData.
    return juce::Font (juce::Font::getDefaultMonospacedFontName(), height, style);
}

juce::Font SoonSpinsLookAndFeel::getLabelFont (juce::Label& label)
{
    return getBrandFont (juce::jmax (12.0f, label.getFont().getHeight()));
}

juce::Font SoonSpinsLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return getBrandFont (juce::jmin (16.0f, (float) buttonHeight * 0.5f), juce::Font::bold);
}

//==============================================================================
SoonSpinnerAudioProcessorEditor::SoonSpinnerAudioProcessorEditor (SoonSpinnerAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    cloudImage = juce::ImageCache::getFromMemory (BinaryData::background_clouds_orange_png,
                                                   BinaryData::background_clouds_orange_pngSize);

    configureKnob (speedKnob, speedLabel, "speed");
    configureKnob (glideKnob, glideLabel, "glide");
    configureKnob (spinDownAmtKnob, spinDownAmtLabel, "down amt");
    configureKnob (spinUpAmtKnob, spinUpAmtLabel, "up amt");
    configureKnob (mixKnob, mixLabel, "mix");
    configureKnob (wobbleKnob, wobbleLabel, "wobble");

    for (auto* button : { &spinDownButton, &halfTimeButton, &spinUpButton })
    {
        button->setClickingTogglesState (true);
        addAndMakeVisible (button);
    }

    speedAttachment       = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::speed, speedKnob);
    glideAttachment       = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::glide, glideKnob);
    spinDownAmtAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::spinDownAmt, spinDownAmtKnob);
    spinUpAmtAttachment   = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::spinUpAmt, spinUpAmtKnob);
    mixAttachment         = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::mix, mixKnob);
    wobbleAttachment       = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::wobble, wobbleKnob);

    spinDownAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, ParamIDs::spinDownBtn, spinDownButton);
    spinUpAttachment   = std::make_unique<ButtonAttachment> (processorRef.apvts, ParamIDs::spinUpBtn, spinUpButton);
    halfTimeAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, ParamIDs::halfTimeBtn, halfTimeButton);

    setResizable (false, false);
    setSize (560, 360);

    startTimerHz (30); // drives the left-to-right background drift
}

SoonSpinnerAudioProcessorEditor::~SoonSpinnerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SoonSpinnerAudioProcessorEditor::timerCallback()
{
    scrollPixels += 1.0; // one "tick" of drift per frame; per-layer speed scales this in drawParallaxBackground
    repaint();
}

// mouseMove/mouseExit intentionally removed - the background used to also
// shift with the cursor, which felt like it was "following" the mouse.
// Depth now comes purely from each layer's fixed scroll speed below.

juce::Slider& SoonSpinnerAudioProcessorEditor::configureKnob (juce::Slider& knob, juce::Label& label, const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (knob);

    label.setText (text.toLowerCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&knob, false);
    addAndMakeVisible (label);

    return knob;
}

void SoonSpinnerAudioProcessorEditor::drawParallaxBackground (juce::Graphics& g)
{
    if (! cloudImage.isValid())
        return;

    auto bounds = getLocalBounds().toFloat();

    // Three copies of the same cloud art at increasing scale/opacity/scroll
    // speed, so the further "layers" drift further/faster - a classic
    // side-scrolling parallax illusion (depth comes purely from relative
    // scroll speed, not from mouse position). The art itself
    // (Source/Assets/background_clouds_orange.png) is generated to tile
    // seamlessly left-to-right, so each layer can scroll continuously
    // (like soonspins.com's background) without a visible seam - we just
    // draw repeated copies across the width.
    struct Layer { float scale; float scrollSpeed; float alpha; };
    const Layer layers[] = {
        { 1.15f, 0.12f, 0.35f },
        { 1.35f, 0.28f, 0.55f },
        { 1.6f,  0.50f, 0.85f },
    };

    for (auto& layer : layers)
    {
        auto tileW = (float) cloudImage.getWidth()  * layer.scale;
        auto h     = (float) cloudImage.getHeight() * layer.scale;
        auto y     = (bounds.getHeight() - h) * 0.5f;

        auto drift  = (float) (scrollPixels * layer.scrollSpeed);
        auto offset = std::fmod (drift, tileW);
        if (offset < 0.0f)
            offset += tileW;

        g.setOpacity (layer.alpha);

        for (float x = offset - tileW; x < bounds.getWidth(); x += tileW)
            g.drawImage (cloudImage, x, y, tileW, h,
                         0, 0, cloudImage.getWidth(), cloudImage.getHeight());
    }

    g.setOpacity (1.0f);

    // Darken toward the edges so the knobs/labels stay readable over the
    // busy cloud art.
    juce::ColourGradient vignette (juce::Colours::transparentBlack, bounds.getCentre(),
                                    SoonspinsColours::background.withAlpha (0.9f), bounds.getBottomRight(), true);
    g.setGradientFill (vignette);
    g.fillRect (bounds);
}

void SoonSpinnerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (SoonspinsColours::background);
    drawParallaxBackground (g);

    g.setColour (SoonspinsColours::accent);
    g.setFont (SoonSpinsLookAndFeel::getBrandFont (28.0f, juce::Font::bold));
    g.drawText ("soonspinner", getLocalBounds().removeFromTop (48), juce::Justification::centred);

    g.setColour (SoonspinsColours::text.withAlpha (0.5f));
    g.setFont (SoonSpinsLookAndFeel::getBrandFont (12.0f));
    g.drawText ("soonspins", getLocalBounds().removeFromBottom (18), juce::Justification::centred);
}

void SoonSpinnerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (40); // space for title

    auto knobRow = area.removeFromTop (140);
    const int knobWidth = knobRow.getWidth() / 6;

    for (auto* knob : { &speedKnob, &glideKnob, &spinDownAmtKnob, &spinUpAmtKnob, &mixKnob, &wobbleKnob })
        knob->setBounds (knobRow.removeFromLeft (knobWidth).reduced (6, 20));

    area.removeFromTop (24); // room for knob labels above buttons

    auto buttonRow = area.removeFromTop (60);
    const int buttonWidth = buttonRow.getWidth() / 3;
    spinDownButton.setBounds (buttonRow.removeFromLeft (buttonWidth).reduced (8, 4));
    halfTimeButton.setBounds (buttonRow.removeFromLeft (buttonWidth).reduced (8, 4));
    spinUpButton.setBounds (buttonRow.reduced (8, 4));
}
