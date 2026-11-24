#pragma once

#include "PluginProcessor.h"

// ---------------------------------------------------------------
//  TEMA: Gorselini gonderince bu renkleri ona gore degistiririz
// ---------------------------------------------------------------
namespace Theme
{
    inline const juce::Colour background { 0xff0d0d10 };
    inline const juce::Colour accent     { 0xffff3b3b };
    inline const juce::Colour track      { 0xff2a2a30 };
    inline const juce::Colour knobBody   { 0xff17171c };
    inline const juce::Colour text       { 0xffe8e8ee };

    constexpr int editorWidth  = 640;
    constexpr int editorHeight = 360;
}

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

class SoftClipperAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SoftClipperAudioProcessorEditor (SoftClipperAudioProcessor&);
    ~SoftClipperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (Knob& knob, const juce::String& paramID, const juce::String& name);

    SoftClipperAudioProcessor& audioProcessor;
    KnobLookAndFeel laf;
    Knob drive, softness, ceiling, mix;
    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftClipperAudioProcessorEditor)
};
