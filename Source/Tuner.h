#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>

// Violin tuner fed with the clean input (before any effect). The audio thread only copies samples
// into a FIFO; the pitch detection (YIN) runs on the GUI thread when the window asks for it.
class Tuner
{
public:
    struct Reading
    {
        bool valid = false;
        float hz = 0.0f;
    };

    void prepare (double sampleRate);
    void push (const float* samples, int numSamples);    // audio thread
    Reading update();                                     // GUI thread


private:
    float detect (double sr);

    juce::AbstractFifo fifo { 1 << 15 };
    std::vector<float> ring = std::vector<float> (1 << 15);
    std::atomic<double> sampleRate { 0.0 };
    std::vector<float> history, diff;
};
