#include "PluginEditor.h"
#if JucePlugin_Build_Standalone
 #include <juce_audio_utils/juce_audio_utils.h>
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

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

constexpr int editorWidth = 880, editorHeight = 560;
constexpr int knobTop = 168, knobSize = 100, knobGap = 104, knobLeft = 26;
}

SafetyPinEditor::SafetyPinEditor (SafetyPinProcessor& p)
    : AudioProcessorEditor (p), pin (p), tunerDisplay (p.tuner, *p.params.getParameter ("mute"))
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
        k.onRightClick = [this, i] { showLearnMenu (i); };
    }
    overkill.onRightClick = [this] { showLearnMenu (overkillIndex); };
    bypass.onRightClick = [this] { showLearnMenu (bypassIndex); };
    mute.onRightClick = [this] { showLearnMenu (muteIndex); };
    addAndMakeVisible (overkill);
    addAndMakeVisible (bypass);
    addAndMakeVisible (mute);
    bypassAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.params, "bypass", bypass);
    muteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.params, "mute", mute);

   #if JucePlugin_Build_Standalone
    // The standalone app starts with the input muted "to avoid a feedback loop" and a bar across
    // the top; this is an instrument effect, so open with the input live (MUTE is right here).
    if (p.wrapperType == AudioProcessor::wrapperType_Standalone)
        if (auto* holder = StandalonePluginHolder::getInstance())
            holder->getMuteInputValue().setValue (false);
   #endif
    addAndMakeVisible (tunerDisplay);
    overkillAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.params, "overkill", overkill);

    grain = punk::grain (editorWidth, editorHeight);
    setSize (editorWidth, editorHeight);
    shownLearnState = learnSnapshot();
    startTimerHz (15);
}

SafetyPinEditor::~SafetyPinEditor()
{
    pin.midiLearn.cancelLearning();
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------- MIDI learn

juce::String SafetyPinEditor::learnSnapshot() const
{
    auto snapshot = String (pin.midiLearn.learning());
    for (int i = 0; i <= muteIndex; ++i)
        snapshot << "," << pin.midiLearn.ccFor (i);
    return snapshot;
}

void SafetyPinEditor::timerCallback()
{
    const auto now = learnSnapshot();
    if (now != shownLearnState || pin.midiLearn.learning() >= 0)
    {
        shownLearnState = now;
        repaint();                            // also makes the "learning" ring blink
    }
}

void SafetyPinEditor::showLearnMenu (int parameter)
{
    auto& learn = pin.midiLearn;
    const auto cc = learn.ccFor (parameter);
    PopupMenu menu;
    menu.setLookAndFeel (&look);
    if (learn.learning() == parameter)
        menu.addItem ("Cancel MIDI learn", [&learn] { learn.cancelLearning(); });
    else
        menu.addItem ("MIDI learn: turn a controller knob next", [&learn, parameter] { learn.startLearning (parameter); });
    if (cc >= 0)
        menu.addItem ("Forget MIDI (CC " + String (cc) + ")", [&learn, parameter] { learn.forget (parameter); });
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (parameter >= overkillIndex ? static_cast<Component*> (&switchFor (parameter))
                                                                                              : static_cast<Component*> (&knobs[parameter])));
}

Button& SafetyPinEditor::switchFor (int slot)
{
    return slot == bypassIndex ? static_cast<Button&> (bypass) : slot == muteIndex ? static_cast<Button&> (mute) : static_cast<Button&> (overkill);
}

void SafetyPinEditor::resized()
{
    for (int i = 0; i < numKnobs; ++i)
        knobs[i].setBounds (knobLeft + i * knobGap, knobTop, knobSize, knobSize);
    tunerDisplay.setBounds (130, 352, 480, 136);
    overkill.setBounds (editorWidth - 262, 358, 240, 72);
    bypass.setBounds (editorWidth - 262, 494, 118, 54);
    mute.setBounds (editorWidth - 140, 494, 118, 54);
}

void SafetyPinEditor::paint (Graphics& g)
{
    g.fillAll (punk::yellow);
    g.drawImageAt (grain, 0, 0);
    punk::anarchy (g, { 66.0f, 420.0f }, 34.0f, punk::black.withAlpha (0.8f));

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

    // MIDI learn: which controller knob drives what, and a blinking ring while learning
    {
        const auto learning = pin.midiLearn.learning();
        const auto blink = (Time::getMillisecondCounter() / 300) % 2 == 0;
        g.setFont (punk::font ("Ink Free", 16.0f, true));
        for (int i = 0; i <= muteIndex; ++i)
        {
            const auto isKnob = i < numKnobs;
            const auto b = (isKnob ? knobs[i].getBounds() : switchFor (i).getBounds()).toFloat();
            const auto note = isKnob ? Rectangle<float> (b.getX() - 6.0f, b.getBottom() + 34.0f, b.getWidth() + 12.0f, 18.0f)
                            : i == overkillIndex ? Rectangle<float> (b.getX(), b.getY() - 20.0f, b.getWidth(), 18.0f)
                                                 : Rectangle<float> (b.getX(), b.getBottom() - 8.0f, b.getWidth(), 16.0f);
            if (i == learning)
            {
                if (blink)
                {
                    g.setColour (punk::pink);
                    if (isKnob)
                        g.drawEllipse (b.reduced (2.0f), 4.0f);
                    else
                        g.drawRect (b.reduced (4.0f), 4.0f);
                }
                g.setColour (punk::black);
                g.drawText ("turn a knob...", note, Justification::centred);
            }
            else if (const auto cc = pin.midiLearn.ccFor (i); cc >= 0)
            {
                g.setColour (punk::black);
                g.drawText ("CC " + String (cc), note, Justification::centred);
            }
        }
        g.setColour (punk::black.withAlpha (0.75f));
        g.setFont (punk::font ("Ink Free", 15.0f, true));
        g.drawText ("right-click a knob = MIDI learn", Rectangle<float> (130.0f, 326.0f, 300.0f, 18.0f), Justification::centredLeft);
    }

    // credits, typed on a scrap of paper
    {
        Random rnd (2026);
        const auto scrap = Rectangle<float> ((float) editorWidth - 262.0f, 440.0f, 240.0f, 48.0f);
        const auto paperPath = punk::torn (scrap, rnd, 3.0f);
        g.setColour (punk::black.withAlpha (0.3f));
        g.fillPath (paperPath, AffineTransform::translation (3.0f, 3.0f));
        g.setColour (punk::paper);
        g.fillPath (paperPath);
        g.setColour (punk::black);
        g.setFont (punk::font ("Courier New", 13.0f, true));
        g.drawText (String (CharPointer_UTF8 ("Farica\xe2\x98\x86Kimora")), scrap.withTrimmedBottom (22.0f).withTrimmedTop (4.0f), Justification::centred);
        g.drawText (String::fromUTF8 ("© Cursed Entertainment"), scrap.withTrimmedTop (22.0f).withTrimmedBottom (4.0f), Justification::centred);
    }
}
