#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Tuner.h"

// Tuner card: note and cents, a needle, and which violin string (G D A E) you're nearest.
// Click it to mute the output while tuning (the same MUTE as the button).
class TunerDisplay : public juce::Component, private juce::Timer
{
public:
    TunerDisplay (Tuner&, juce::RangedAudioParameter& mute);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void refresh();

    Tuner& tuner;
    juce::RangedAudioParameter& mute;
    float hz = 0.0f, cents = 0.0f;
    int note = -1, string = -1;
    int silentFrames = 1000;
};
