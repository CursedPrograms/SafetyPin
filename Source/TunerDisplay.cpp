#include "TunerDisplay.h"
#include "PunkLook.h"

using namespace juce;

namespace
{
const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const char* stringNames[] = { "G", "D", "A", "E" };
const float stringHz[] = { 196.00f, 293.66f, 440.00f, 659.26f };   // A = 440
constexpr float inTune = 3.0f;                                     // cents
}

TunerDisplay::TunerDisplay (Tuner& t)
    : tuner (t)
{
    setMouseCursor (MouseCursor::PointingHandCursor);
    refresh();
    startTimerHz (30);
}

void TunerDisplay::timerCallback()
{
    refresh();
    repaint();
}

void TunerDisplay::refresh()
{
    const auto r = tuner.update();
    if (! r.valid)
    {
        ++silentFrames;
        if (silentFrames > 20)                // hold the last note a moment after you stop
            note = -1;
        return;
    }

    const auto midi = 69.0f + 12.0f * std::log2 (r.hz / 440.0f);
    const auto n = (int) std::lround (midi);
    const auto c = (midi - (float) n) * 100.0f;
    // the needle glides instead of jittering; jumps straight to a new note
    cents = (n == note && silentFrames < 3) ? cents + 0.35f * (c - cents) : c;
    hz = r.hz;
    note = n;
    silentFrames = 0;

    string = 0;
    for (int s = 1; s < 4; ++s)
        if (std::abs (std::log2 (hz / stringHz[s])) < std::abs (std::log2 (hz / stringHz[string])))
            string = s;
}

void TunerDisplay::mouseDown (const MouseEvent&)
{
    tuner.mute = ! tuner.mute.load();
    repaint();
}

void TunerDisplay::paint (Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto card = bounds.reduced (6.0f);
    {
        Random rnd (42);
        const auto paperPath = punk::torn (card, rnd, 4.0f);
        g.setColour (punk::black);
        g.fillPath (paperPath, AffineTransform::translation (5.0f, 5.0f));
        g.setColour (punk::paper);
        g.fillPath (paperPath);
    }

    const auto hasNote = note >= 0;
    const auto tuned = hasNote && std::abs (cents) < inTune;

    // the note
    const auto noteBox = Rectangle<float> (card.getX() + 14.0f, card.getY() + 14.0f, 104.0f, card.getHeight() - 28.0f);
    g.setColour (tuned ? punk::acid : punk::black);
    g.fillRect (noteBox);
    g.setColour (tuned ? punk::black : punk::paper);
    if (hasNote)
    {
        const auto name = String (noteNames[((note % 12) + 12) % 12]);
        g.setFont (punk::font ("Impact", 64.0f));
        g.drawText (name, noteBox.withTrimmedRight (22.0f).withTrimmedBottom (10.0f), Justification::centred);
        g.setFont (punk::font ("Impact", 22.0f));
        g.drawText (String (note / 12 - 1), noteBox.withTrimmedLeft (74.0f).withTrimmedTop (60.0f), Justification::centred);
    }
    else
    {
        g.setFont (punk::font ("Impact", 52.0f));
        g.drawText ("--", noteBox, Justification::centred);
    }

    // the meter: -50 .. +50 cents
    const auto meter = Rectangle<float> (noteBox.getRight() + 22.0f, card.getY() + 16.0f,
                                         card.getRight() - noteBox.getRight() - 44.0f, 50.0f);
    const auto xFor = [&] (float c) { return meter.getX() + (c + 50.0f) / 100.0f * meter.getWidth(); };
    g.setColour (punk::black);
    g.drawLine (meter.getX(), meter.getBottom(), meter.getRight(), meter.getBottom(), 2.5f);
    g.setFont (punk::font ("Courier New", 12.0f, true));
    for (int c = -50; c <= 50; c += 10)
    {
        const auto big = c % 25 == 0;
        g.drawLine (xFor ((float) c), meter.getBottom() - (big ? 16.0f : 8.0f), xFor ((float) c), meter.getBottom(), big ? 2.5f : 1.5f);
        if (std::abs (c) == 25)
            g.drawText ((c > 0 ? "+" : "") + String (c), Rectangle<float> (xFor ((float) c) - 20.0f, meter.getY() - 2.0f, 40.0f, 14.0f), Justification::centred);
    }
    g.setColour (punk::red);
    g.fillRect (xFor (0.0f) - 2.0f, meter.getY() + 4.0f, 4.0f, meter.getHeight() - 4.0f);
    g.setFont (punk::font ("Ink Free", 16.0f, true));
    g.setColour (punk::black);
    g.drawText ("flat", meter.withWidth (60.0f).translated (0.0f, -6.0f), Justification::topLeft);
    g.drawText ("sharp", meter.withTrimmedLeft (meter.getWidth() - 60.0f).translated (0.0f, -6.0f), Justification::topRight);

    if (hasNote)
    {
        const auto x = xFor (jlimit (-50.0f, 50.0f, cents));
        Path needle;
        needle.addTriangle (x, meter.getBottom() - 2.0f, x - 9.0f, meter.getY() + 8.0f, x + 9.0f, meter.getY() + 8.0f);
        g.setColour (tuned ? punk::acid : punk::pink);
        g.fillPath (needle);
        g.setColour (punk::black);
        g.strokePath (needle, PathStrokeType (2.0f));
    }

    // the four strings, nearest one lit up
    const auto row = Rectangle<float> (meter.getX(), meter.getBottom() + 14.0f, meter.getWidth(), 34.0f);
    for (int s = 0; s < 4; ++s)
    {
        const auto box = Rectangle<float> (row.getX() + (float) s * 40.0f, row.getY(), 32.0f, 32.0f);
        const auto lit = hasNote && s == string;
        g.setColour (lit ? punk::pink : punk::newsprint);
        g.fillRect (box);
        g.setColour (punk::black);
        g.drawRect (box, 2.0f);
        g.setFont (punk::font ("Arial Black", 18.0f));
        g.drawText (stringNames[s], box, Justification::centred);
    }

    const auto info = row.withTrimmedLeft (168.0f);
    g.setFont (punk::font ("Courier New", 15.0f, true));
    g.setColour (punk::black);
    if (hasNote)
        g.drawText (String (hz, 1) + " Hz  " + (cents >= 0.0f ? "+" : "") + String (roundToInt (cents)) + "c",
                    info.withHeight (16.0f), Justification::centredLeft);
    if (tuner.mute)
    {
        g.setColour (punk::red);
        g.setFont (punk::font ("Impact", 18.0f));
        g.drawText ("MUTED", info.withTrimmedTop (17.0f), Justification::centredLeft);
    }
    else
    {
        g.setFont (punk::font ("Courier New", 12.0f));
        g.drawText ("click = mute", info.withTrimmedTop (17.0f), Justification::centredLeft);
    }

    punk::tapeLabel (g, { card.getX() - 4.0f, card.getY() - 12.0f, 92.0f, 24.0f }, "TUNER", 7);
}
