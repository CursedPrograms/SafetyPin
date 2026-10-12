#include "PluginEditor.h"

using namespace juce;

namespace
{
struct KnobInfo
{
    const char* id;
    const char* label;
    Colour colour;
};

const KnobInfo knobInfo[SafetyPinEditor::numKnobs] = {
    { "anger", "ANGER", punk::pink },
    { "cello", "CELLO", punk::paper },
    { "guts", "GUTS", punk::acid },
    { "bottom", "BOTTOM", punk::orange },
    { "snarl", "SNARL", punk::paper },
    { "basement", "BASEMENT", punk::pink },
    { "decay", "DECAY", punk::acid },
    { "loud", "LOUD", punk::red },
};

constexpr int editorWidth = 880, editorHeight = 450;
constexpr int knobTop = 168, knobSize = 100, knobGap = 104, knobLeft = 26;
}

SafetyPinEditor::SafetyPinEditor (SafetyPinProcessor& p)
    : AudioProcessorEditor (p)
{
    setLookAndFeel (&look);
    for (int i = 0; i < numKnobs; ++i)
    {
        auto& k = knobs[i];
        k.setName (knobInfo[i].label);
        k.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
        k.setPopupDisplayEnabled (true, true, this);
        k.setColour (Slider::rotarySliderFillColourId, knobInfo[i].colour);
        addAndMakeVisible (k);
        knobAttachments[i] = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.params, knobInfo[i].id, k);
    }
    addAndMakeVisible (noFuture);
    noFutureAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.params, "nofuture", noFuture);

    grain = punk::grain (editorWidth, editorHeight);
    setSize (editorWidth, editorHeight);
}

SafetyPinEditor::~SafetyPinEditor()
{
    setLookAndFeel (nullptr);
}

void SafetyPinEditor::resized()
{
    for (int i = 0; i < numKnobs; ++i)
        knobs[i].setBounds (knobLeft + i * knobGap, knobTop, knobSize, knobSize);
    noFuture.setBounds (editorWidth - 270, editorHeight - 92, 240, 72);
}

void SafetyPinEditor::paint (Graphics& g)
{
    g.fillAll (punk::yellow);
    g.drawImageAt (grain, 0, 0);
    punk::anarchy (g, { 70.0f, (float) editorHeight - 62.0f }, 34.0f, punk::black.withAlpha (0.8f));

    // title: ransom letters on a torn pink strip, pinned on
    {
        Random rnd (77);
        Graphics::ScopedSaveState state (g);
        g.addTransform (AffineTransform::rotation (-0.025f, editorWidth * 0.5f, 70.0f));
        const auto strip = punk::torn ({ 20.0f, 18.0f, (float) editorWidth - 40.0f, 104.0f }, rnd, 7.0f);
        g.setColour (punk::black);
        g.fillPath (strip, AffineTransform::translation (6.0f, 6.0f));
        g.setColour (punk::pink);
        g.fillPath (strip);
        punk::ransom (g, "SAFETY PIN", { 150.0f, 26.0f, 560.0f, 84.0f }, 1976);
    }
    punk::safetyPin (g, { 34.0f, 40.0f }, { 120.0f, 22.0f }, 1.3f);
    punk::safetyPin (g, { (float) editorWidth - 40.0f, 104.0f }, { (float) editorWidth - 126.0f, 118.0f }, 1.1f);

    // what's what, scrawled above the knobs
    g.setColour (punk::black);
    g.setFont (punk::font ("Ink Free", 22.0f, true));
    const auto above = (float) knobTop - 34.0f;
    g.drawText ("< make it a cello / make it heavy >", Rectangle<float> ((float) knobLeft, above, 4.0f * knobGap, 28.0f), Justification::centred);
    g.drawText ("< tone >", Rectangle<float> ((float) knobLeft + 4.0f * knobGap, above, (float) knobGap, 28.0f), Justification::centred);
    g.drawText ("< the basement >", Rectangle<float> ((float) knobLeft + 5.0f * knobGap, above, 2.0f * knobGap, 28.0f), Justification::centred);

    for (int i = 0; i < numKnobs; ++i)
    {
        const auto b = knobs[i].getBounds().toFloat();
        punk::tapeLabel (g, { b.getX() + 2.0f, b.getBottom() + 6.0f, b.getWidth() - 4.0f, 26.0f }, knobInfo[i].label, 100 + i);
    }

    g.setColour (punk::black);
    g.setFont (punk::font ("Ink Free", 20.0f, true));
    g.drawText ("electric violin wrecker", Rectangle<float> (130.0f, (float) editorHeight - 92.0f, 470.0f, 30.0f), Justification::centredLeft);
    g.drawText ("no gods, no masters, no aliasing", Rectangle<float> (130.0f, (float) editorHeight - 62.0f, 470.0f, 30.0f), Justification::centredLeft);
}
