// Offline check without Cubase: renders the GUI to a PNG, and a synthetic electric violin (open G
// string up to A, with vibrato and bowing swells) through the effect at a few settings.
//   SafetyPinRender <out dir>
#include "../Source/PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

namespace
{
constexpr double rate = 48000.0;
constexpr int blockSize = 256;

juce::AudioBuffer<float> violin (double seconds)
{
    const int n = (int) (seconds * rate);
    juce::AudioBuffer<float> out (1, n);
    const double notes[] = { 196.0, 293.66, 440.0, 392.0 };     // G3 D4 A4 G4
    double phase = 0.0;
    float body1 = 0.0f, body2 = 0.0f;
    juce::Random rnd (5);
    for (int i = 0; i < n; ++i)
    {
        const auto t = i / rate;
        const auto note = std::min (3, (int) (t / (seconds / 4.0)));
        const auto noteT = t - note * seconds / 4.0;
        const auto f = notes[note] * (1.0 + 0.006 * std::sin (2.0 * juce::MathConstants<double>::pi * 5.5 * noteT));
        phase += f / rate;
        phase -= std::floor (phase);
        const auto bow = (float) std::min (1.0, noteT * 8.0) * (float) std::min (1.0, (seconds / 4.0 - noteT) * 10.0);
        auto x = (float) (2.0 * phase - 1.0) + 0.02f * (rnd.nextFloat() - 0.5f);   // sawtooth + bow noise
        body1 += 0.25f * (x - body1);
        body2 += 0.5f * (body1 - body2);
        out.setSample (0, i, 0.25f * bow * body2);
    }
    return out;
}

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer)
{
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
    auto writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                                    .withSampleRate (rate)
                                                                    .withNumChannels (buffer.getNumChannels())
                                                                    .withBitsPerSample (24));
    writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}

void set (SafetyPinProcessor& p, const char* id, float value)
{
    auto* param = p.params.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    {
        SafetyPinProcessor p;
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const auto file = outDir.getChildFile ("gui.png");
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat().writeImageToStream (image, stream);
    }

    const auto input = violin (8.0);
    writeWav (outDir.getChildFile ("in_violin.wav"), input);

    struct Setting { const char* name; float anger, cello, guts, nofuture; };
    const Setting settings[] = {
        { "clean_cello", 0.0f, 1.0f, 0.0f, 0.0f },
        { "default", 0.6f, 0.5f, 0.25f, 0.0f },
        { "no_future", 1.0f, 0.7f, 0.6f, 1.0f },
    };
    for (const auto& s : settings)
    {
        SafetyPinProcessor p;
        set (p, "anger", s.anger);
        set (p, "cello", s.cello);
        set (p, "guts", s.guts);
        set (p, "nofuture", s.nofuture);
        p.setPlayConfigDetails (1, 2, rate, blockSize);
        p.prepareToPlay (rate, blockSize);

        juce::AudioBuffer<float> out (2, input.getNumSamples());
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        for (int pos = 0; pos < input.getNumSamples(); pos += blockSize)
        {
            const auto n = std::min (blockSize, input.getNumSamples() - pos);
            block.setSize (2, n, false, false, true);
            block.clear();
            block.copyFrom (0, 0, input, 0, pos, n);
            p.processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
        }
        bool finite = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < out.getNumSamples(); ++i)
                finite = finite && std::isfinite (out.getSample (ch, i));
        std::cout << s.name << ": peak " << juce::Decibels::gainToDecibels (out.getMagnitude (0, out.getNumSamples()))
                  << " dBFS, rms " << juce::Decibels::gainToDecibels (out.getRMSLevel (0, 0, out.getNumSamples()))
                  << " dBFS, latency " << p.getLatencySamples() << ", " << (finite ? "finite" : "NaN/INF!") << "\n";
        writeWav (outDir.getChildFile (juce::String ("out_") + s.name + ".wav"), out);
    }
    return 0;
}
