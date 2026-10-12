#pragma once
#include <vector>

// Real octave down for one melodic line (the violin turned into a cello): two read heads sweep
// through a delay line at half speed and crossfade. The sweep length is a whole number of the
// note's periods, so when a head jumps back it lands on the same point of the waveform - no
// warble or clicks, the timbre and bowing stay.
class OctaveDown
{
public:
    void prepare (double sampleRate);
    void reset();
    void setPeriod (float samples);          // from the pitch tracker; 0 = unknown
    float process (float x);

private:
    float windowFor (float period) const;
    float read (float delay) const;

    std::vector<float> buf;
    int mask = 0, w = 0;
    double sr = 48000.0;
    float period = 0.0f;
    float phase[2] { 0.0f, 0.5f };
    float window[2] { 1920.0f, 1920.0f };
};
