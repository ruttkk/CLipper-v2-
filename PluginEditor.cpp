#include "PluginEditor.h"

#if HAS_BACKGROUND
 #include "BinaryData.h"
#endif

// ---------------------------------------------------------------
//  Knob cizimi
// ---------------------------------------------------------------
void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre  = bounds.getCentre();
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float lineW = 5.0f;
    const float arcR  = radius - lineW * 0.5f;
    const juce::PathStrokeType stroke (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    // arka iz
    juce::Path trackPath;
    trackPath.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Theme::track);
    g.strokePath (trackPath, stroke);

    // deger yayi
    juce::Path valuePath;
    valuePath.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (Theme::accent);
    g.strokePath (valuePath, stroke);

    // govde
    const float bodyR = radius - 12.0f;
    g.setColour (Theme::knobBody);
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (Theme::track);
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

    // ibre
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -bodyR + 3.0f, 3.0f, bodyR * 0.45f, 1.5f);
    g.setColour (Theme::accent);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

// ---------------------------------------------------------------
//  Editor
// ---------------------------------------------------------------
SoftClipperAudioProcessorEditor::SoftClipperAudioProcessorEditor (SoftClipperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
#if HAS_BACKGROUND
    background = juce::ImageCache::getFromMemory (BinaryData::background_png,
                                                  BinaryData::background_pngSize);
#endif

    setupKnob (drive,    "drive",    "DRIVE");
    setupKnob (softness, "softness", "SOFTNESS");
    setupKnob (ceiling,  "ceiling",  "CEILING");
    setupKnob (mix,      "mix",      "MIX");

    setSize (Theme::editorWidth, Theme::editorHeight);
}

SoftClipperAudioProcessorEditor::~SoftClipperAudioProcessorEditor()
{
    for (auto* k : { &drive, &softness, &ceiling, &mix })
        k->slider.setLookAndFeel (nullptr);
}

void SoftClipperAudioProcessorEditor::setupKnob (Knob& knob, const juce::String& paramID, const juce::String& name)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    knob.slider.setLookAndFeel (&laf);
    knob.slider.setColour (juce::Slider::textBoxTextColourId, Theme::text);
    knob.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    knob.slider.setRotaryParameters (juce::degreesToRadians (225.0f),
                                     juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    knob.label.setColour (juce::Label::textColourId, Theme::text);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramID, knob.slider);
}

void SoftClipperAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
    {
        g.drawImage (background, getLocalBounds().toFloat(),
                     juce::RectanglePlacement::fillDestination);

        // knob'lar okunsun diye hafif koyu panel (istersen sil)
        auto panel = getLocalBounds().toFloat().reduced (20.0f).removeFromBottom (190.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (panel, 12.0f);
    }
    else
    {
        g.fillAll (Theme::background);

        g.setColour (Theme::accent);
        g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
        g.drawText ("SOFT CLIPPER", getLocalBounds().removeFromTop (70).reduced (24, 0),
                    juce::Justification::centredLeft);
    }
}

void SoftClipperAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);
    area.removeFromTop (area.getHeight() - 190);   // knob'lar altta

    const int colW = area.getWidth() / 4;

    for (auto* k : { &drive, &softness, &ceiling, &mix })
    {
        auto col = area.removeFromLeft (colW);
        k->label.setBounds (col.removeFromTop (22));
        k->slider.setBounds (col);
    }
}
