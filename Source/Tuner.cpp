#include "Tuner.h"
#include <cmath>

void Tuner::prepare (double sr)
{
    sampleRate = sr;
}

void Tuner::push (const float* samples, int numSamples)
{
    // when nobody is looking at the tuner the FIFO fills up and the rest is simply dropped
    const auto scope = fifo.write (juce::jmin (numSamples, fifo.getFreeSpace()));
    if (scope.blockSize1 > 0)
        std::copy (samples, samples + scope.blockSize1, ring.begin() + scope.startIndex1);
    if (scope.blockSize2 > 0)
        std::copy (samples + scope.blockSize1, samples + scope.blockSize1 + scope.blockSize2, ring.begin() + scope.startIndex2);
}

Tuner::Reading Tuner::update()
{
    const auto sr = sampleRate.load();
    if (sr <= 0.0)
        return {};

    const auto window = (int) (0.025 * sr), maxLag = (int) (sr / 150.0);
    const auto needed = (size_t) (window + maxLag);
    if (history.size() != needed)
        history.assign (needed, 0.0f);

    // slide the newest samples in at the end
    const auto scope = fifo.read (fifo.getNumReady());
    const auto add = [this] (const float* src, int n)
    {
        if (n <= 0)
            return;
        if ((size_t) n >= history.size())
        {
            std::copy (src + n - (int) history.size(), src + n, history.begin());
            return;
        }
        std::move (history.begin() + n, history.end(), history.begin());
        std::copy (src, src + n, history.end() - n);
    };
    add (ring.data() + scope.startIndex1, scope.blockSize1);
    add (ring.data() + scope.startIndex2, scope.blockSize2);

    auto energy = 0.0f;
    for (auto x : history)
        energy += x * x;
    if (std::sqrt (energy / (float) history.size()) < 0.003f)         // about -50 dBFS: not playing
        return {};

    const auto hz = detect (sr);
    return { hz > 0.0f, hz };
}

// YIN (de Cheveigne & Kawahara, 2002): the lag where the signal best matches itself, with the
// difference normalised so the octave above isn't picked by mistake.
float Tuner::detect (double sr)
{
    const auto window = (int) (0.025 * sr), maxLag = (int) (sr / 150.0), minLag = (int) (sr / 3000.0);
    diff.assign ((size_t) maxLag + 2, 0.0f);

    for (int lag = 1; lag <= maxLag + 1; ++lag)
    {
        auto sum = 0.0f;
        for (int j = 0; j < window; ++j)
        {
            const auto d = history[(size_t) j] - history[(size_t) (j + lag)];
            sum += d * d;
        }
        diff[(size_t) lag] = sum;
    }

    // cumulative mean normalised difference
    auto running = 0.0f;
    diff[0] = 1.0f;
    for (int lag = 1; lag <= maxLag + 1; ++lag)
    {
        running += diff[(size_t) lag];
        diff[(size_t) lag] = running > 0.0f ? diff[(size_t) lag] * (float) lag / running : 1.0f;
    }

    int best = -1;
    for (int lag = juce::jmax (2, minLag); lag <= maxLag; ++lag)
        if (diff[(size_t) lag] < 0.12f)
        {
            while (lag < maxLag && diff[(size_t) lag + 1] < diff[(size_t) lag])
                ++lag;
            best = lag;
            break;
        }
    if (best < 0)
        return 0.0f;

    // parabola through the three points around the dip, for fractions of a sample
    const auto a = diff[(size_t) best - 1], b = diff[(size_t) best], c = diff[(size_t) best + 1];
    const auto denom = a - 2.0f * b + c;
    const auto lag = (float) best + (std::abs (denom) > 1e-9f ? 0.5f * (a - c) / denom : 0.0f);
    return (float) sr / lag;
}
