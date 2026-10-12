#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

// MIDI learn for the standalone app (and any host that sends the plugin MIDI): right-click a knob,
// turn a knob on your controller, and that CC drives it from then on. The links are saved with
// the rest of the plugin's state.
//
// The audio thread only reads CCs and drops the values off; a timer on the message thread hands
// them to the parameters, so the host hears about every change the normal way.
class MidiLearn : private juce::Timer
{
public:
    MidiLearn (juce::AudioProcessorValueTreeState&, const juce::StringArray& parameterIds);
    ~MidiLearn() override;

    void process (const juce::MidiBuffer&);             // audio thread

    void startLearning (int parameter);
    void cancelLearning()                               { learnParameter = -1; }
    int learning() const                                { return learnParameter; }
    void forget (int parameter);
    int ccFor (int parameter) const;                    // -1 if none
    void loadFromState();                               // after the host restores the state

private:
    static constexpr int maxParameters = 16;
    void timerCallback() override;
    void saveToState();

    juce::AudioProcessorValueTreeState& params;
    juce::StringArray ids;
    std::array<std::atomic<int>, 128> ccToParameter;
    std::array<std::atomic<float>, maxParameters> pending;
    std::atomic<int> learnParameter { -1 };
    std::atomic<bool> mappingChanged { false };
};
