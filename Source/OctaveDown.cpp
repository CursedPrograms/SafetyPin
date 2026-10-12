#include "OctaveDown.h"
#include <algorithm>
#include <cmath>

void OctaveDown::prepare (double sampleRate)
{
    sr = sampleRate;
    int size = 1;
    while (size < (int) (0.2 * sr))
        size <<= 1;
    buf.assign ((size_t) size, 0.0f);
    mask = size - 1;
    reset();
}

void OctaveDown::reset()
{
    std::fill (buf.begin(), buf.end(), 0.0f);
    w = 0;
    period = 0.0f;
    phase[0] = 0.0f;
    phase[1] = 0.5f;
    window[0] = window[1] = windowFor (0.0f);
}

void OctaveDown::setPeriod (float samples)
{
    period = samples;
}

// ~40 ms, rounded to an even number of periods: the jump at the end of a sweep (one window)
// and the distance between the two heads (half a window) are then both whole periods.
float OctaveDown::windowFor (float p) const
{
    const auto target = (float) (0.04 * sr);
    if (p <= 0.0f)
        return target;
    const auto pairs = std::max (1.0f, std::round (target / (2.0f * p)));
    return 2.0f * p * pairs;
}

float OctaveDown::read (float delay) const
{
    const auto i = (int) delay;
    const auto f = delay - (float) i;
    const auto a = buf[(size_t) ((w - 1 - i) & mask)];
    const auto b = buf[(size_t) ((w - 2 - i) & mask)];
    return a + (b - a) * f;
}

float OctaveDown::process (float x)
{
    buf[(size_t) w] = x;
    w = (w + 1) & mask;

    auto out = 0.0f;
    for (int h = 0; h < 2; ++h)
    {
        // reading at half speed = the delay grows by half a sample every sample
        phase[h] += 0.5f / window[h];
        if (phase[h] >= 1.0f)
        {
            phase[h] -= 1.0f;
            window[h] = windowFor (period);  // only changes while this head is silent
        }
        const auto gain = std::sin (3.14159265f * phase[h]);
        out += gain * gain * read (1.0f + phase[h] * window[h]);
    }
    return out;
}
