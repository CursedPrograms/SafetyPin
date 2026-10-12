#include "PunkLook.h"

using namespace juce;

namespace punk
{
Font font (const String& name, float height, bool bold)
{
    return Font (FontOptions (name, height, bold ? Font::bold : Font::plain));
}

// A rectangle with ripped edges, like paper torn off a flyer
Path torn (Rectangle<float> r, Random& rnd, float jag)
{
    Path p;
    p.startNewSubPath (r.getX(), r.getY() + rnd.nextFloat() * jag);
    for (auto x = r.getX() + 6.0f; x < r.getRight(); x += 5.0f + rnd.nextFloat() * 7.0f)
        p.lineTo (x, r.getY() + rnd.nextFloat() * jag);
    p.lineTo (r.getRight(), r.getY() + rnd.nextFloat() * jag);
    for (auto y = r.getY() + 8.0f; y < r.getBottom(); y += 6.0f + rnd.nextFloat() * 6.0f)
        p.lineTo (r.getRight() - rnd.nextFloat() * jag * 0.6f, y);
    for (auto x = r.getRight(); x > r.getX(); x -= 5.0f + rnd.nextFloat() * 7.0f)
        p.lineTo (x, r.getBottom() - rnd.nextFloat() * jag);
    p.lineTo (r.getX(), r.getBottom() - rnd.nextFloat() * jag);
    for (auto y = r.getBottom() - 8.0f; y > r.getY(); y -= 6.0f + rnd.nextFloat() * 6.0f)
        p.lineTo (r.getX() + rnd.nextFloat() * jag * 0.6f, y);
    p.closeSubPath();
    return p;
}

void tapeLabel (Graphics& g, Rectangle<float> r, const String& text, int seed)
{
    Random rnd (seed);
    Graphics::ScopedSaveState state (g);
    g.addTransform (AffineTransform::rotation ((rnd.nextFloat() - 0.5f) * 0.12f, r.getCentreX(), r.getCentreY()));
    auto strip = torn (r, rnd, 3.0f);
    g.setColour (black.withAlpha (0.25f));
    g.fillPath (strip, AffineTransform::translation (2.0f, 2.0f));
    g.setColour (tape);
    g.fillPath (strip);
    g.setColour (black);
    g.setFont (font ("Courier New", r.getHeight() * 0.62f, true));
    g.drawText (text, r, Justification::centred, false);
}

// Every letter cut out of a different magazine
void ransom (Graphics& g, const String& text, Rectangle<float> area, int seed)
{
    static const char* fonts[] = { "Impact", "Arial Black", "Times New Roman", "Courier New", "Georgia", "Verdana" };
    const Colour backs[] = { paper, black, yellow, newsprint, paper, acid };

    Random rnd (seed);
    const auto letters = text.length();
    const auto cell = area.getWidth() / (float) letters;
    for (int i = 0; i < letters; ++i)
    {
        const auto c = text[i];
        if (c == ' ')
            continue;
        const auto h = area.getHeight() * (0.78f + rnd.nextFloat() * 0.22f);
        auto box = Rectangle<float> (area.getX() + cell * (float) i, area.getCentreY() - h * 0.5f, cell * 0.94f, h)
                       .translated (0.0f, (rnd.nextFloat() - 0.5f) * 8.0f);
        const auto back = backs[rnd.nextInt (6)];
        const auto* face = fonts[rnd.nextInt (6)];

        Graphics::ScopedSaveState state (g);
        g.addTransform (AffineTransform::rotation ((rnd.nextFloat() - 0.5f) * 0.32f, box.getCentreX(), box.getCentreY()));
        g.setColour (black.withAlpha (0.35f));
        g.fillRect (box.translated (3.0f, 3.0f));
        g.setColour (back);
        g.fillRect (box);
        g.setColour (back == black ? (rnd.nextBool() ? pink : paper) : (rnd.nextInt (3) == 0 ? red : black));
        g.setFont (font (face, h * 0.92f, rnd.nextBool()));
        g.drawFittedText (String::charToString (c), box.toNearestInt(), Justification::centred, 1, 0.5f);
    }
}

void safetyPin (Graphics& g, Point<float> from, Point<float> to, float s)
{
    const auto len = from.getDistanceFrom (to);
    const auto gap = 7.0f * s;
    const auto t = AffineTransform::rotation (std::atan2 (to.y - from.y, to.x - from.x)).translated (from);

    Path wires;
    wires.addCentredArc (0.0f, 0.0f, gap * 0.9f, gap * 0.9f, 0.0f, 0.0f, MathConstants<float>::twoPi, true);
    wires.startNewSubPath (0.0f, gap * 0.9f);
    wires.lineTo (len - 6.0f * s, gap * 0.5f);
    wires.startNewSubPath (0.0f, -gap * 0.9f);
    wires.lineTo (len - 6.0f * s, -gap * 0.4f);
    wires.applyTransform (t);

    Path head;
    head.addRoundedRectangle (len - 16.0f * s, -gap * 0.9f, 20.0f * s, gap * 1.8f, 4.0f * s);
    head.applyTransform (t);

    g.setColour (black);
    g.strokePath (wires, PathStrokeType (4.2f * s, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xffb9bec4));
    g.strokePath (wires, PathStrokeType (2.4f * s, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::white.withAlpha (0.8f));
    g.strokePath (wires, PathStrokeType (0.8f * s), AffineTransform::translation (-0.6f * s, -0.6f * s));
    g.setColour (Colour (0xff9aa0a8));
    g.fillPath (head);
    g.setColour (black);
    g.strokePath (head, PathStrokeType (2.0f * s));
}

// Circle-A, sprayed with a marker
void anarchy (Graphics& g, Point<float> c, float r, Colour colour)
{
    Path p;
    p.addEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    p.startNewSubPath (c.x - r * 0.75f, c.y + r * 0.95f);
    p.lineTo (c.x, c.y - r * 1.25f);
    p.lineTo (c.x + r * 0.75f, c.y + r * 0.95f);
    p.startNewSubPath (c.x - r * 1.15f, c.y + r * 0.25f);
    p.lineTo (c.x + r * 1.15f, c.y + r * 0.05f);
    g.setColour (colour);
    g.strokePath (p, PathStrokeType (r * 0.16f, PathStrokeType::curved, PathStrokeType::rounded));
}

// Photocopier dirt: specks, smears and a halftone corner
Image grain (int width, int height)
{
    Image img (Image::ARGB, width, height, true);
    Graphics g (img);
    Random rnd (1977);
    for (int i = 0; i < width * height / 140; ++i)
    {
        g.setColour (black.withAlpha (0.05f + rnd.nextFloat() * 0.25f));
        const auto s = 0.6f + rnd.nextFloat() * 1.6f;
        g.fillEllipse ((float) rnd.nextInt (width), (float) rnd.nextInt (height), s, s);
    }
    for (int i = 0; i < 14; ++i)
    {
        g.setColour (black.withAlpha (0.04f + rnd.nextFloat() * 0.05f));
        const auto x = (float) rnd.nextInt (width), y = (float) rnd.nextInt (height);
        g.fillRect (x, y, 40.0f + rnd.nextFloat() * 200.0f, 1.0f + rnd.nextFloat() * 3.0f);
    }
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 12; ++x)
        {
            const auto r = 4.5f * (1.0f - (float) (x + y) / 19.0f);
            g.setColour (black.withAlpha (0.55f));
            g.fillEllipse ((float) (width - 12 - x * 13) - r, (float) (height - 12 - y * 13) - r, 2 * r, 2 * r);
        }
    return img;
}
}

PunkLook::PunkLook()
{
    setColour (BubbleComponent::backgroundColourId, punk::black);
    setColour (BubbleComponent::outlineColourId, punk::pink);
    setColour (TooltipWindow::textColourId, punk::paper);
}

Font PunkLook::getSliderPopupFont (Slider&)
{
    return punk::font ("Courier New", 17.0f, true);
}

// A badge on a studded leather ring
void PunkLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                 float startAngle, float endAngle, Slider& s)
{
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const auto radius = jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    Random rnd (s.getName().hashCode());

    g.setColour (punk::black);
    for (int i = 0; i <= 10; ++i)
    {
        const auto a = startAngle + (endAngle - startAngle) * (float) i / 10.0f + (rnd.nextFloat() - 0.5f) * 0.05f;
        const auto big = i % 5 == 0;
        g.drawLine ({ c.getPointOnCircumference (radius * 0.84f, a), c.getPointOnCircumference (radius * (big ? 1.0f : 0.94f), a) },
                    big ? 3.2f : 2.0f);
    }

    const auto ring = radius * 0.78f;
    g.setColour (punk::black);
    g.fillEllipse (Rectangle<float> (2 * ring, 2 * ring).withCentre (c).translated (4.0f, 4.0f));
    g.fillEllipse (Rectangle<float> (2 * ring, 2 * ring).withCentre (c));
    for (int i = 0; i < 10; ++i)
    {
        const auto p = c.getPointOnCircumference (ring * 0.84f, MathConstants<float>::twoPi * (float) i / 10.0f);
        const auto stud = Rectangle<float> (ring * 0.2f, ring * 0.2f).withCentre (p);
        g.setGradientFill (ColourGradient (Colours::white, stud.getTopLeft(), Colour (0xff6d737a), stud.getBottomRight(), false));
        g.fillEllipse (stud);
    }

    const auto badge = Rectangle<float> (ring * 1.3f, ring * 1.3f).withCentre (c);
    g.setColour (s.findColour (Slider::rotarySliderFillColourId));
    g.fillEllipse (badge);
    g.setColour (punk::black);
    g.drawEllipse (badge, 2.5f);

    Path pointer;
    pointer.addRoundedRectangle (-3.5f, -ring * 0.66f, 7.0f, ring * 0.5f, 2.0f);
    pointer.applyTransform (AffineTransform::rotation (startAngle + pos * (endAngle - startAngle)).translated (c));
    g.setColour (punk::black);
    g.fillPath (pointer);
    g.setColour (punk::paper);
    g.strokePath (pointer, PathStrokeType (1.5f));
}

// A stamp: dead grey when off, screaming red when on
void PunkLook::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    const auto on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (8.0f);
    Graphics::ScopedSaveState state (g);
    g.addTransform (AffineTransform::rotation (on ? -0.06f : 0.03f, r.getCentreX(), r.getCentreY()));

    g.setColour (punk::black);
    g.fillRect (r.translated (4.0f, 4.0f));
    g.setColour (on ? punk::red : (highlighted ? punk::paper : punk::newsprint));
    g.fillRect (r);
    g.setColour (punk::black);
    g.drawRect (r, 3.0f);
    g.setColour (on ? punk::paper : punk::black);
    g.setFont (punk::font ("Impact", r.getHeight() * 0.62f));
    g.drawText (b.getButtonText(), r, Justification::centred, false);
    if (! on)
    {
        g.setColour (punk::black.withAlpha (0.5f));
        g.drawLine (r.getX() + 8, r.getBottom() - 8, r.getRight() - 8, r.getY() + 8, 2.0f);
    }
}
