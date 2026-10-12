// Offline check without Cubase: renders the GUI to a PNG, and a synthetic electric violin (open G
// string up to A, with vibrato and bowing swells) through the effect at a few settings.
//   SafetyPinRender <out dir>
#include "../Source/PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

constexpr int SafetyPinEditorOverkill = 8;     // MIDI learn slot of the OVERKILL switch

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

    // MIDI learn: learn, play, the switch, forget, and survive a save + reload
    {
        const auto pump = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (120); };
        const auto send = [] (SafetyPinProcessor& p, int cc, int value)
        {
            juce::AudioBuffer<float> audio (2, 64);
            audio.clear();
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
            p.processBlock (audio, midi);
        };
        const auto value = [] (SafetyPinProcessor& p, const char* id) { return p.params.getParameter (id)->getValue(); };
        int failures = 0;
        const auto check = [&failures] (bool ok, const char* what)
        {
            std::cout << (ok ? "  ok   " : "  FAIL ") << what << '\n';
            failures += ok ? 0 : 1;
        };

        SafetyPinProcessor p;
        p.setPlayConfigDetails (1, 2, rate, 64);
        p.prepareToPlay (rate, 64);
        std::cout << "midi learn:\n";
        send (p, 21, 100); pump();
        check (p.midiLearn.ccFor (0) < 0 && std::abs (value (p, "anger") - 0.6f) < 1e-6f, "unlearned CC does nothing");
        p.midiLearn.startLearning (0);
        send (p, 21, 100); pump();
        check (p.midiLearn.ccFor (0) == 21 && p.midiLearn.learning() < 0, "ANGER learned CC 21");
        check (std::abs (value (p, "anger") - 100.0f / 127.0f) < 1e-4f, "CC 21 = 100 moves ANGER to 100/127");
        send (p, 21, 0); pump();
        check (value (p, "anger") < 1e-6f, "CC 21 = 0 turns ANGER down");
        p.midiLearn.startLearning (SafetyPinEditorOverkill);
        send (p, 64, 127); pump();
        check (p.midiLearn.ccFor (SafetyPinEditorOverkill) == 64 && value (p, "overkill") > 0.5f, "OVERKILL learned CC 64, pedal down = on");
        send (p, 64, 0); pump();
        check (value (p, "overkill") < 0.5f, "pedal up = off");
        p.midiLearn.startLearning (7);
        send (p, 22, 64); pump();
        p.midiLearn.startLearning (7);
        send (p, 23, 30); pump();
        check (p.midiLearn.ccFor (7) == 23, "relearning LOUD moves it to the new CC");
        send (p, 22, 127); pump();
        const auto* loudParam = p.params.getParameter ("loud");                 // snaps to 0.1 dB steps
        const auto snapped = loudParam->convertTo0to1 (loudParam->convertFrom0to1 (30.0f / 127.0f));
        check (std::abs (value (p, "loud") - snapped) < 1e-5f, "old CC 22 no longer moves LOUD");

        juce::MemoryBlock state;
        p.getStateInformation (state);
        SafetyPinProcessor reloaded;
        reloaded.setStateInformation (state.getData(), (int) state.getSize());
        check (reloaded.midiLearn.ccFor (0) == 21 && reloaded.midiLearn.ccFor (SafetyPinEditorOverkill) == 64
                   && reloaded.midiLearn.ccFor (7) == 23, "links survive save + reload");
        reloaded.midiLearn.forget (0);
        pump();
        check (reloaded.midiLearn.ccFor (0) < 0, "forget removes the link");
        juce::MemoryBlock state2;
        reloaded.getStateInformation (state2);
        SafetyPinProcessor reloaded2;
        reloaded2.setStateInformation (state2.getData(), (int) state2.getSize());
        check (reloaded2.midiLearn.ccFor (0) < 0 && reloaded2.midiLearn.ccFor (7) == 23, "forgetting is saved too");
        std::cout << (failures == 0 ? "midi learn: all passed\n" : "midi learn: FAILURES\n");
    }

    // tuner: feed it each violin note and see what it hears
    {
        Tuner t;
        t.prepare (rate);
        const auto notes = violin (8.0);
        for (int k = 0; k < 4; ++k)
        {
            t.update();                                   // drain
            t.push (notes.getReadPointer (0, (int) ((k * 2 + 1.0) * rate)), 4096);
            const auto r = t.update();
            std::cout << "tuner note " << k << ": " << (r.valid ? juce::String (r.hz, 2) + " Hz" : juce::String ("none")) << "\n";
        }
    }

    {
        SafetyPinProcessor p;
        // a slightly sharp A in the tuner, so the snapshot shows the needle
        p.tuner.prepare (rate);
        std::vector<float> a (4096);
        for (size_t i = 0; i < a.size(); ++i)
            a[i] = 0.3f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 443.0 * (double) i / rate);
        p.tuner.push (a.data(), (int) a.size());
        p.setPlayConfigDetails (1, 2, rate, 64);
        p.prepareToPlay (rate, 64);
        p.midiLearn.startLearning (0);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 21, 76), 0);
        juce::AudioBuffer<float> silence (2, 64);
        silence.clear();
        p.processBlock (silence, midi);
        p.midiLearn.startLearning (SafetyPinEditorOverkill);
        midi.clear();
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
        p.processBlock (silence, midi);
        p.midiLearn.startLearning (5);                 // BASEMENT left learning: blinking ring
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const auto file = outDir.getChildFile ("gui.png");
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat().writeImageToStream (image, stream);
    }

    const auto input = violin (8.0);
    writeWav (outDir.getChildFile ("in_violin.wav"), input);

    struct Setting { const char* name; float anger, cello, guts, overkill; };
    const Setting settings[] = {
        { "clean_cello", 0.0f, 1.0f, 0.0f, 0.0f },
        { "default", 0.6f, 0.5f, 0.25f, 0.0f },
        { "overkill", 1.0f, 0.7f, 0.6f, 1.0f },
    };
    for (const auto& s : settings)
    {
        SafetyPinProcessor p;
        set (p, "anger", s.anger);
        set (p, "cello", s.cello);
        set (p, "guts", s.guts);
        set (p, "overkill", s.overkill);
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
