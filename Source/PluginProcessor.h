#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OctaveDown.h"
#include "PlateReverb.h"

// Electric violin wrecker: octave down (cello), sub bass, fat distortion, plate reverb.
// Mono in, stereo out.
class SafetyPinProcessor : public juce::AudioProcessor
{
public:
    SafetyPinProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                         { return true; }
    const juce::String getName() const override             { return "Safety Pin"; }
    bool acceptsMidi() const override                       { return false; }
    bool producesMidi() const override                      { return false; }
    double getTailLengthSeconds() const override            { return 8.0; }
    int getNumPrograms() override                           { return 1; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   {}
    const juce::String getProgramName (int) override        { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState params;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();
    float subBass (float x);

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::ArrayCoefficients<float>;

    // before the clipper
    Filter rumble, scratch, fat;
    // pitch tracker (one cycle per armed upward crossing) driving the octave and the sub
    Filter subDetect, subSmooth1, subSmooth2;
    float subEnv = 0.0f, subSign = 1.0f, cycleSign = 1.0f, period = 0.0f;
    int sinceCycle = 0;
    bool subArmed = true;
    OctaveDown octave;
    // after the clipper
    Filter dcBlock, fizz, tone, bottom;
    float lastSnarl = -1.0f, lastBottom = -1.0f;

    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::AudioBuffer<float> mono;
    PlateReverb reverb;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> drive, outGain;
    juce::SmoothedValue<float> filth, guts, wet, cello;

    std::atomic<float>* anger = nullptr;
    std::atomic<float>* celloParam = nullptr;
    std::atomic<float>* gutsParam = nullptr;
    std::atomic<float>* bottomParam = nullptr;
    std::atomic<float>* snarl = nullptr;
    std::atomic<float>* basement = nullptr;
    std::atomic<float>* decay = nullptr;
    std::atomic<float>* loud = nullptr;
    std::atomic<float>* noFuture = nullptr;

    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SafetyPinProcessor)
};
