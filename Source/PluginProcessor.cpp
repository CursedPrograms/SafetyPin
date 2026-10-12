#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioParameterFloatAttributes zeroToTen()
{
    return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (v * 10.0f, 1); });
}

constexpr float clipBias = 0.15f;            // a little asymmetry: even harmonics, warmer
}

SafetyPinProcessor::SafetyPinProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::mono(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "SafetyPin", createLayout())
{
    anger = params.getRawParameterValue ("anger");
    celloParam = params.getRawParameterValue ("cello");
    gutsParam = params.getRawParameterValue ("guts");
    bottomParam = params.getRawParameterValue ("bottom");
    snarl = params.getRawParameterValue ("snarl");
    basement = params.getRawParameterValue ("basement");
    decay = params.getRawParameterValue ("decay");
    loud = params.getRawParameterValue ("loud");
    noFuture = params.getRawParameterValue ("nofuture");
}

juce::AudioProcessorValueTreeState::ParameterLayout SafetyPinProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (juce::ParameterID { "anger", 1 }, "Anger (drive)", juce::NormalisableRange<float> (0, 1), 0.6f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "cello", 1 }, "Cello (octave down)", juce::NormalisableRange<float> (0, 1), 0.5f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "guts", 1 }, "Guts (sub, 2 octaves down)", juce::NormalisableRange<float> (0, 1), 0.25f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "bottom", 1 }, "Bottom (bass)", juce::NormalisableRange<float> (0, 1), 0.5f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "snarl", 1 }, "Snarl (tone)", juce::NormalisableRange<float> (0, 1), 0.45f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "basement", 1 }, "Basement (reverb mix)", juce::NormalisableRange<float> (0, 1), 0.3f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "decay", 1 }, "Decay (reverb length)", juce::NormalisableRange<float> (0, 1), 0.55f, zeroToTen()));
    layout.add (std::make_unique<P> (juce::ParameterID { "loud", 1 }, "Loud (output)", juce::NormalisableRange<float> (-30, 12, 0.1f), -6.0f,
                                     juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "nofuture", 1 }, "No Future", false));
    return layout;
}

bool SafetyPinProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    const auto monoOrStereo = [] (const juce::AudioChannelSet& s)
        { return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo(); };
    return monoOrStereo (in) && monoOrStereo (out);
}

void SafetyPinProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    mono.setSize (1, samplesPerBlock);

    // Violin through a fuzz is mostly fizz and bow scratch: cut that before clipping and push
    // the low mids, so the distortion has something fat to chew on.
    *rumble.coefficients = Coeffs::makeHighPass (sr, 70.0f, 0.707f);
    *scratch.coefficients = Coeffs::makeLowPass (sr, 4500.0f, 0.707f);
    *fat.coefficients = Coeffs::makePeakFilter (sr, 330.0f, 0.8f, 1.7f);
    *subDetect.coefficients = Coeffs::makeLowPass (sr, 900.0f, 0.707f);
    *subSmooth1.coefficients = Coeffs::makeLowPass (sr, 450.0f, 0.707f);
    *subSmooth2.coefficients = Coeffs::makeLowPass (sr, 450.0f, 0.707f);
    *dcBlock.coefficients = Coeffs::makeHighPass (sr, 20.0f, 0.707f);
    *fizz.coefficients = Coeffs::makePeakFilter (sr, 3500.0f, 1.0f, 0.5f);
    lastSnarl = lastBottom = -1.0f;
    updateFilters();

    for (auto* f : { &rumble, &scratch, &fat, &subDetect, &subSmooth1, &subSmooth2, &dcBlock, &fizz, &tone, &bottom })
        f->reset();
    subEnv = period = 0.0f;
    subSign = cycleSign = 1.0f;
    sinceCycle = 0;
    subArmed = true;
    octave.prepare (sr);

    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()));

    reverb.prepare (sr);
    tuner.prepare (sr);

    const auto upRate = sr * (double) oversampling.getOversamplingFactor();
    drive.reset (upRate, 0.03);
    filth.reset (upRate, 0.02);
    guts.reset (sr, 0.05);
    cello.reset (sr, 0.05);
    wet.reset (sr, 0.05);
    outGain.reset (sr, 0.05);
    drive.setCurrentAndTargetValue (1.0f);
    outGain.setCurrentAndTargetValue (1.0f);
    filth.setCurrentAndTargetValue (noFuture->load() > 0.5f ? 1.0f : 0.0f);
    guts.setCurrentAndTargetValue (gutsParam->load());
    cello.setCurrentAndTargetValue (celloParam->load());
    wet.setCurrentAndTargetValue (basement->load());
}

void SafetyPinProcessor::updateFilters()
{
    const auto s = snarl->load();
    if (s != lastSnarl)
    {
        lastSnarl = s;
        *tone.coefficients = Coeffs::makeLowPass (sr, 1200.0f * std::pow (7.0f, s), 0.6f);
    }
    const auto b = bottomParam->load();
    if (b != lastBottom)
    {
        lastBottom = b;
        *bottom.coefficients = Coeffs::makeLowShelf (sr, 120.0f, 0.7f, juce::Decibels::decibelsToGain (b * 14.0f));
    }
}

// Pitch tracker + sub bass, like the old analog octavers: every cycle of the (lowpassed) input
// is counted (that also gives the period for the cello octave), and a flip-flop that toggles
// every second cycle gives a square two octaves down, shaped by the input's envelope and
// rounded off into something like a bass synth.
float SafetyPinProcessor::subBass (float x)
{
    const auto env = std::abs (x);
    subEnv += (env > subEnv ? 0.02f : 0.0004f) * (env - subEnv);

    ++sinceCycle;
    const auto d = subDetect.processSample (x);
    const auto threshold = 0.15f * subEnv;
    if (subArmed && d > threshold)
    {
        subArmed = false;
        // a believable violin period (150 Hz - 3 kHz) and smoothed, else the last good one
        const auto p = (float) sinceCycle;
        if (p > (float) (sr / 3000.0) && p < (float) (sr / 150.0))
            period = period > 0.0f && std::abs (p - period) < 0.3f * period ? 0.7f * period + 0.3f * p : p;
        sinceCycle = 0;
        cycleSign = -cycleSign;
        if (cycleSign > 0.0f)
            subSign = -subSign;
    }
    else if (! subArmed && d < -threshold)
    {
        subArmed = true;
    }
    if (sinceCycle > (int) (sr / 100.0))
        period = 0.0f;                       // silence or noise: no pitch
    octave.setPeriod (period);

    const auto gate = juce::jlimit (0.0f, 1.0f, (subEnv - 0.002f) * 200.0f);   // silent when you are
    const auto square = subSign * subEnv * gate * 2.0f;
    return subSmooth2.processSample (subSmooth1.processSample (square));
}

void SafetyPinProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    if (numSamples == 0 || numIn == 0)
        return;
    if (mono.getNumSamples() < numSamples)
        mono.setSize (1, numSamples, false, false, true);      // host sent a bigger block than promised

    const auto a = anger->load();
    const auto nf = noFuture->load() > 0.5f;
    drive.setTargetValue (juce::Decibels::decibelsToGain (a * 36.0f + (nf ? 12.0f : 0.0f)));
    filth.setTargetValue (nf ? 1.0f : 0.0f);
    guts.setTargetValue (gutsParam->load());
    cello.setTargetValue (celloParam->load());
    wet.setTargetValue (basement->load());
    outGain.setTargetValue (juce::Decibels::decibelsToGain (loud->load() - a * 12.0f));
    reverb.setDecay (decay->load());
    updateFilters();

    // ---- mono in: the tuner hears it untouched
    auto* m = mono.getWritePointer (0);
    for (int i = 0; i < numSamples; ++i)
    {
        auto x = 0.0f;
        for (int ch = 0; ch < numIn; ++ch)
            x += buffer.getSample (ch, i);
        m[i] = x / (float) numIn;
    }
    tuner.push (m, numSamples);

    // ---- cleaned up, octave and sub mixed in
    for (int i = 0; i < numSamples; ++i)
    {
        auto x = rumble.processSample (m[i]);
        const auto sub = subBass (x);
        const auto oct = octave.process (x);
        const auto c = cello.getNextValue() * juce::MathConstants<float>::halfPi;
        x = std::cos (c) * x + std::sin (c) * oct;           // full CELLO = only the octave
        x = fat.processSample (scratch.processSample (x));
        m[i] = x + guts.getNextValue() * 2.5f * sub;
    }

    // ---- the clipper, 4x oversampled so the fizz doesn't fold back down as aliasing
    auto block = juce::dsp::AudioBlock<float> (mono).getSubBlock (0, (size_t) numSamples);
    auto up = oversampling.processSamplesUp (block);
    auto* u = up.getChannelPointer (0);
    const auto soft0 = std::tanh (clipBias);
    for (size_t i = 0; i < up.getNumSamples(); ++i)
    {
        const auto x = u[i] * drive.getNextValue() + clipBias;
        const auto soft = std::tanh (x) - soft0;
        const auto hard = juce::jlimit (-1.0f, 1.0f, x) - clipBias;
        u[i] = soft + filth.getNextValue() * (hard - soft);
    }
    oversampling.processSamplesDown (block);

    // ---- cab-ish shaping, bass, reverb, out
    for (int i = 0; i < numSamples; ++i)
    {
        auto x = dcBlock.processSample (m[i]);
        x = bottom.processSample (tone.processSample (fizz.processSample (x)));

        float wl, wr;
        reverb.process (x, wl, wr);
        const auto w = wet.getNextValue();
        const auto dry = x * (1.0f - 0.4f * w);
        const auto g = outGain.getNextValue();
        const auto l = (dry + w * wl) * g, r = (dry + w * wr) * g;
        if (numOut == 1)
        {
            buffer.setSample (0, i, 0.5f * (l + r));
        }
        else
        {
            buffer.setSample (0, i, l);
            buffer.setSample (1, i, r);
        }
    }
    if (tuner.mute)
        buffer.clear();
}

juce::AudioProcessorEditor* SafetyPinProcessor::createEditor()
{
    return new SafetyPinEditor (*this);
}

void SafetyPinProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = params.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void SafetyPinProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (params.state.getType()))
            params.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SafetyPinProcessor();
}
