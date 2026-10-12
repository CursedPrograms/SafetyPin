#pragma once
#include <vector>

// Plate reverb after Jon Dattorro, "Effect Design Part 1" (JAES 1997): four input diffusers into
// a figure-of-eight tank of modulated allpasses and delays, stereo taken from taps in the tank.
class PlateReverb
{
public:
    void prepare (double sampleRate);
    void reset();
    void setDecay (float decay01);           // 0 = short room, 1 = huge hall
    void process (float in, float& outL, float& outR);

private:
    struct Delay
    {
        void setSize (int length);
        void clear();
        void push (float x)                  { buf[(size_t) w] = x; w = (w + 1) & mask; }
        float at (int d) const               { return buf[(size_t) ((w - 1 - d) & mask)]; }
        float atFrac (float d) const;
        std::vector<float> buf;
        int mask = 0, w = 0;
    };

    struct Allpass
    {
        float process (float x, float g, float mod = 0.0f);
        Delay line;
        int length = 1;
    };

    int scaled (int samplesAt29k) const;

    double sr = 48000.0;
    Delay preDelay;
    int preDelayLength = 1;
    float bandwidthState = 0.0f;
    Allpass diffusers[4];
    Allpass tankAp1[2], tankAp2[2];
    Delay tankD1[2], tankD2[2];
    int tankD1Len[2] {}, tankD2Len[2] {};
    float damp[2] {}, tankOut[2] {};
    float decay = 0.6f;
    float lfoPhase = 0.0f, lfoInc = 0.0f, excursion = 0.0f;
    int tapL[7] {}, tapR[7] {};
};
