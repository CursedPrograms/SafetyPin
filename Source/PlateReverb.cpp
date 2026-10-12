#include "PlateReverb.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr double dattorroRate = 29761.0;     // the rate the paper's delay lengths are given at
constexpr float twoPi = 6.283185307f;
constexpr float bandwidth = 0.9995f;
constexpr float damping = 0.35f;             // darker than the paper's 0.0005: kinder to violin
}

void PlateReverb::Delay::setSize (int length)
{
    int size = 1;
    while (size < length + 2)
        size <<= 1;
    buf.assign ((size_t) size, 0.0f);
    mask = size - 1;
    w = 0;
}

void PlateReverb::Delay::clear()
{
    std::fill (buf.begin(), buf.end(), 0.0f);
}

float PlateReverb::Delay::atFrac (float d) const
{
    const auto i = (int) d;
    const auto f = d - (float) i;
    const auto a = at (i);
    return a + (at (i + 1) - a) * f;
}

float PlateReverb::Allpass::process (float x, float g, float mod)
{
    const auto z = mod != 0.0f ? line.atFrac ((float) (length - 1) + mod) : line.at (length - 1);
    const auto v = x - g * z;
    line.push (v);
    return z + g * v;
}

int PlateReverb::scaled (int samplesAt29k) const
{
    return std::max (1, (int) std::lround (samplesAt29k * sr / dattorroRate));
}

void PlateReverb::prepare (double sampleRate)
{
    sr = sampleRate;

    preDelayLength = std::max (1, (int) (0.018 * sr));
    preDelay.setSize (preDelayLength);

    const int diffuserLengths[4] = { 142, 107, 379, 277 };
    for (int i = 0; i < 4; ++i)
    {
        diffusers[i].length = scaled (diffuserLengths[i]);
        diffusers[i].line.setSize (diffusers[i].length);
    }

    excursion = (float) (16.0 * sr / dattorroRate);
    const int ap1[2] = { 672, 908 }, d1[2] = { 4453, 4217 }, ap2[2] = { 1800, 2656 }, d2[2] = { 3720, 3163 };
    for (int s = 0; s < 2; ++s)
    {
        tankAp1[s].length = scaled (ap1[s]);
        tankAp1[s].line.setSize (tankAp1[s].length + (int) excursion + 2);
        tankAp2[s].length = scaled (ap2[s]);
        tankAp2[s].line.setSize (tankAp2[s].length);
        tankD1Len[s] = scaled (d1[s]);
        tankD1[s].setSize (tankD1Len[s]);
        tankD2Len[s] = scaled (d2[s]);
        tankD2[s].setSize (tankD2Len[s]);
    }

    // output taps from the paper's table (left: yL, right: yR)
    const int l[7] = { 266, 2974, 1913, 1996, 1990, 187, 1066 };
    const int r[7] = { 353, 3627, 1228, 2673, 2111, 335, 121 };
    for (int i = 0; i < 7; ++i)
    {
        tapL[i] = scaled (l[i]);
        tapR[i] = scaled (r[i]);
    }

    lfoInc = (float) (twoPi * 0.9 / sr);
    reset();
}

void PlateReverb::reset()
{
    preDelay.clear();
    for (auto& d : diffusers)
        d.line.clear();
    for (int s = 0; s < 2; ++s)
    {
        tankAp1[s].line.clear();
        tankAp2[s].line.clear();
        tankD1[s].clear();
        tankD2[s].clear();
        damp[s] = tankOut[s] = 0.0f;
    }
    bandwidthState = 0.0f;
    lfoPhase = 0.0f;
}

void PlateReverb::setDecay (float decay01)
{
    decay = 0.25f + 0.72f * std::clamp (decay01, 0.0f, 1.0f);
}

void PlateReverb::process (float in, float& outL, float& outR)
{
    auto x = preDelay.at (preDelayLength - 1);
    preDelay.push (in);

    bandwidthState += bandwidth * (x - bandwidthState);
    x = diffusers[0].process (bandwidthState, 0.75f);
    x = diffusers[1].process (x, 0.75f);
    x = diffusers[2].process (x, 0.625f);
    x = diffusers[3].process (x, 0.625f);

    lfoPhase += lfoInc;
    if (lfoPhase > twoPi)
        lfoPhase -= twoPi;
    const float mods[2] = { excursion * (1.0f + std::sin (lfoPhase)) * 0.5f,
                            excursion * (1.0f + std::cos (lfoPhase)) * 0.5f };
    const auto diffusion2 = std::clamp (decay + 0.15f, 0.25f, 0.5f);

    const float feedIn[2] = { x + decay * tankOut[1], x + decay * tankOut[0] };
    for (int s = 0; s < 2; ++s)
    {
        auto a = tankAp1[s].process (feedIn[s], -0.7f, mods[s]);
        const auto d = tankD1[s].at (tankD1Len[s] - 1);
        tankD1[s].push (a);
        damp[s] += (1.0f - damping) * (d - damp[s]);
        a = tankAp2[s].process (damp[s] * decay, diffusion2);
        tankOut[s] = tankD2[s].at (tankD2Len[s] - 1);
        tankD2[s].push (a);
    }

    // tank halves: 0 = left (672 / 4453 / 1800 / 3720), 1 = right (908 / 4217 / 2656 / 3163)
    outL = 0.6f * (tankD1[1].at (tapL[0]) + tankD1[1].at (tapL[1]) - tankAp2[1].line.at (tapL[2])
                   + tankD2[1].at (tapL[3]) - tankD1[0].at (tapL[4]) - tankAp2[0].line.at (tapL[5])
                   - tankD2[0].at (tapL[6]));
    outR = 0.6f * (tankD1[0].at (tapR[0]) + tankD1[0].at (tapR[1]) - tankAp2[0].line.at (tapR[2])
                   + tankD2[0].at (tapR[3]) - tankD1[1].at (tapR[4]) - tankAp2[1].line.at (tapR[5])
                   - tankD2[1].at (tapR[6]));
}
