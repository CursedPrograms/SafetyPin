#pragma once
#include "PluginProcessor.h"
#include "PunkLook.h"
#include "TunerDisplay.h"

// Knob and switch that hand right-clicks to a callback (the MIDI learn menu) instead of turning
class LearnSlider : public juce::Slider
{
public:
    std::function<void()> onRightClick;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
        Slider::mouseDown (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) Slider::mouseDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override   { if (! e.mods.isPopupMenu()) Slider::mouseUp (e); }
};

class LearnButton : public juce::ToggleButton
{
public:
    using ToggleButton::ToggleButton;
    std::function<void()> onRightClick;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
        ToggleButton::mouseDown (e);
    }
    void mouseUp (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) ToggleButton::mouseUp (e); }
};

class SafetyPinEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SafetyPinEditor (SafetyPinProcessor&);
    ~SafetyPinEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int numKnobs = 8;
    static constexpr int overkillIndex = numKnobs;       // its MIDI learn slot

private:
    void timerCallback() override;
    void showLearnMenu (int parameter);
    juce::String learnSnapshot() const;

    SafetyPinProcessor& pin;
    PunkLook look;
    LearnSlider knobs[numKnobs];
    LearnButton overkill { "OVERKILL" };
    TunerDisplay tunerDisplay;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachments[numKnobs];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> overkillAttachment;
    juce::Image grain;
    juce::String shownLearnState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SafetyPinEditor)
};
