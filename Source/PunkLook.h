#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// The zine look: photocopied yellow, torn pink paper, ransom-note letters, masking tape,
// studded badge knobs. Everything is drawn in code; the "random" bits use fixed seeds so the
// page looks the same every time it opens.
namespace punk
{
const juce::Colour yellow { 0xfff2e500 }, pink { 0xffff2d8a }, black { 0xff121212 },
                   paper { 0xfff7f3e8 }, tape { 0xffe8dcb0 }, acid { 0xff8cff3a },
                   orange { 0xffff7a00 }, red { 0xffe8112d }, newsprint { 0xffd9d4c7 };

juce::Font font (const juce::String& name, float height, bool bold = false);
juce::Path torn (juce::Rectangle<float> r, juce::Random& rnd, float jag);
void tapeLabel (juce::Graphics&, juce::Rectangle<float> r, const juce::String& text, int seed);
void ransom (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, int seed);
void safetyPin (juce::Graphics&, juce::Point<float> from, juce::Point<float> to, float scale);
void anarchy (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour);
juce::Image grain (int width, int height);
}

class PunkLook : public juce::LookAndFeel_V4
{
public:
    PunkLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Font getSliderPopupFont (juce::Slider&) override;
};
