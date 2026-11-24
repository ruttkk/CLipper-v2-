#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Algebraik soft clipper: y = x / (1 + |x|^p)^(1/p)
    // p buyudukce diz (sert) kose, p kuculdukce yumusak kose. Tavan = 1.0
    inline float clipSample (float x, float p) noexcept
    {
        const float a = std::abs (x);
        return x / std::pow (1.0f + std::pow (a, p), 1.0f / p);
    }
}

SoftClipperAudioProcessor::SoftClipperAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      oversampling (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true)
{
    driveParam    = apvts.getRawParameterValue ("drive");
    softnessParam = apvts.getRawParameterValue ("softness");
    ceilingParam  = apvts.getRawParameterValue ("ceiling");
    mixParam      = apvts.getRawParameterValue ("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout SoftClipperAudioProcessor::createParameterLayout()
{
    using namespace juce;
    using Attr = AudioParameterFloatAttributes;

    auto fromText = [] (const String& t) { return t.getFloatValue(); };

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "drive", 1 }, "Drive",
        NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
              .withValueFromStringFunction (fromText)));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "softness", 1 }, "Softness",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return String (v, 0) + " %"; })
              .withValueFromStringFunction (fromText)));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "ceiling", 1 }, "Ceiling",
        NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -0.3f,
        Attr().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
              .withValueFromStringFunction (fromText)));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return String (v, 0) + " %"; })
              .withValueFromStringFunction (fromText)));

    return { params.begin(), params.end() };
}

bool SoftClipperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void SoftClipperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };

    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();

    const float latency = oversampling.getLatencyInSamples();
    setLatencySamples ((int) std::round (latency));

    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    mixer.setWetLatency (latency);
    mixer.reset();

    const double osRate = sampleRate * (double) oversampling.getOversamplingFactor();

    driveSmooth.reset   (osRate, 0.02);
    ceilingSmooth.reset (osRate, 0.02);
    shapeSmooth.reset   (osRate, 0.02);

    driveSmooth.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (driveParam->load()));
    ceilingSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (ceilingParam->load()));
    shapeSmooth.setCurrentAndTargetValue   (12.0f - 10.0f * (softnessParam->load() / 100.0f));
}

void SoftClipperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    driveSmooth.setTargetValue   (juce::Decibels::decibelsToGain (driveParam->load()));
    ceilingSmooth.setTargetValue (juce::Decibels::decibelsToGain (ceilingParam->load()));
    // softness 0% -> p=12 (sert), 100% -> p=2 (yumusak)
    shapeSmooth.setTargetValue   (12.0f - 10.0f * (softnessParam->load() / 100.0f));
    mixer.setWetMixProportion    (mixParam->load() / 100.0f);

    juce::dsp::AudioBlock<float> block (buffer);

    mixer.pushDrySamples (block);

    auto osBlock = oversampling.processSamplesUp (block);

    const size_t numCh      = osBlock.getNumChannels();
    const size_t numSamples = osBlock.getNumSamples();

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float d = driveSmooth.getNextValue();
        const float c = ceilingSmooth.getNextValue();
        const float p = shapeSmooth.getNextValue();

        for (size_t ch = 0; ch < numCh; ++ch)
        {
            float* data = osBlock.getChannelPointer (ch);
            data[i] = clipSample (data[i] * d, p) * c;
        }
    }

    oversampling.processSamplesDown (block);

    mixer.mixWetSamples (block);
}

juce::AudioProcessorEditor* SoftClipperAudioProcessor::createEditor()
{
    return new SoftClipperAudioProcessorEditor (*this);
}

void SoftClipperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SoftClipperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoftClipperAudioProcessor();
}
