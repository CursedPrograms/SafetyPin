#pragma once
#include "PluginProcessor.h"
#include "PunkLook.h"

class SafetyPinEditor : public juce::AudioProcessorEditor
{
public:
    explicit SafetyPinEditor (SafetyPinProcessor&);
    ~SafetyPinEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int numKnobs = 8;

private:
    PunkLook look;
    juce::Slider knobs[numKnobs];
    juce::ToggleButton noFuture { "NO FUTURE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachments[numKnobs];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> noFutureAttachment;
    juce::Image grain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SafetyPinEditor)
};
