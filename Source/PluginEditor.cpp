#include "PluginEditor.h"
#include "BinaryData.h"
#include "dsp/Todas.h"
#include <complex>

namespace
{
    using escoita::U;
    using escoita::numero;
}

namespace ui
{
// ================================================================ tipografía
// Montserrat (títulos e rótulos, en maiúsculas espaciadas coma na web de Escoita) e Inter
// (texto corrido): as da familia Barullo. As dúas, licenza OFL.
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font semi (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }
juce::Font caps (float s, float tracking) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s).withKerningFactor (tracking)); }
juce::String upper (const juce::String& t) { return escoita::maiusculas (t); }

void drawCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float tracking)
{
    g.setFont (caps (size, tracking));
    g.drawText (upper (t), r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
{
    g.setColour (c.withAlpha (0.16f));
    for (const auto d : { juce::Point<float> (-0.8f, 0.0f), { 0.8f, 0.0f }, { 0.0f, -0.8f }, { 0.0f, 0.8f } })
        g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

// ================================================================ pezas do aparello
void paintPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f * (float) i).expanded ((float) i), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, r.getY(), col::panel2, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2.0f * rad, 1.0f);
}

void paintWell (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    g.setColour (col::visor.brighter (0.06f));
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 5.0f, false));
    g.fillRect (r.withHeight (6.0f));
    g.restoreState();
}

void paintGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    juce::ColourGradient gr (col::cristal1, r.getCentreX(), r.getY(), col::cristal2, r.getCentreX(), r.getBottom(), false);
    gr.addColour (0.55, juce::Colour (0xff0a1129));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f - 0.03f * (float) i));
        g.drawRoundedRectangle (r.reduced ((float) i + 0.5f), rad, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect (r);
    g.restoreState();
}

void paintKey (juce::Graphics& g, juce::Rectangle<float> r, bool over, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    if (down) r.translate (0.0f, 1.0f);
    const float lift = over && ! down ? 0.10f : 0.0f;
    g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (lift), 0.0f, r.getY(), col::tecla2.brighter (lift * 0.6f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.10f));
    g.fillRect (r.getX() + 6.0f, r.getY() + 0.5f, r.getWidth() - 12.0f, 1.0f);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour luz)
{
    if (on)
    {
        g.setColour (luz.withAlpha (0.16f)); g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (luz.withAlpha (0.34f)); g.fillEllipse (c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f);
        g.setColour (luz); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - 1.3f, c.y - 2.0f, 2.2f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3326)); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }
}

// rótulo dun grupo: na súa cor e cunha liña debaixo
void groupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{
    g.setColour (c);
    drawCaps (g, t, r.withHeight (14.0f), juce::Justification::centred, 9.0f, 0.28f);
    g.setColour (c.withAlpha (0.25f));
    g.fillRect (r.getX() + 8.0f, r.getY() + 18.0f, r.getWidth() - 16.0f, 1.0f);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::noite);
    setColour (juce::Label::textColourId, col::crema);
    setColour (juce::TextEditor::backgroundColourId, col::visor);
    setColour (juce::TextEditor::textColourId, col::ouro);
    setColour (juce::TextEditor::outlineColourId, col::lina);
    setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::crema);
    setColour (juce::PopupMenu::headerTextColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedTextColourId, col::noite);
    setColour (juce::TextButton::buttonColourId, col::tecla2);
    setColour (juce::TextButton::textColourOffId, col::crema2);
    setColour (juce::TextButton::textColourOnId, col::crema);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::crema);
    setColour (juce::AlertWindow::outlineColourId, col::lina);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

// as teclas: en relevo, cun LED (aceso se está escollida; «led» = a cor da capa) e o texto en
// maiúsculas espaciadas; «gold» = un visor dourado (o selector da combinación)
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        paintWell (g, r, 8.0f);
        if (over) { g.setColour (col::laranxa.withAlpha (0.08f)); g.fillRoundedRectangle (r, 8.0f); }
        return;
    }
    paintKey (g, r, over, down);
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed"))
    {
        const auto luz = b.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["led"]) : col::laranxa;
        paintLed (g, { r.getX() + 12.0f, r.getCentreY() }, b.getToggleState(), luz);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        g.setFont (caps (12.0f, 0.1f));
        glowText (g, upper (b.getButtonText()), r.reduced (14.0f, 0.0f).withTrimmedRight (16.0f), juce::Justification::centredLeft, col::ouro);
        juce::Path arrow;
        const float ax = r.getRight() - 18.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (col::ouro); g.fillPath (arrow);
        return;
    }
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed")) r.removeFromLeft (14.0f);
    juce::Colour c = b.getToggleState() || over ? col::crema : col::crema2;
    if (! b.isEnabled()) c = col::crema3.withAlpha (0.6f);
    g.setColour (c);
    const int curve = b.getProperties()["curve"];   // desfacer (−1) e refacer (+1): unha frecha en arco
    if (curve != 0)
    {
        const auto ctr = r.getCentre(); const float rad = 6.5f;
        const float a0 = curve > 0 ? -2.4f : 2.4f, a1 = curve > 0 ? 1.2f : -1.2f;
        juce::Path arc; arc.addCentredArc (ctr.x, ctr.y + 1.0f, rad, rad, 0.0f, a0, a1, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const juce::Point<float> tip (ctr.x + rad * std::sin (a1), ctr.y + 1.0f - rad * std::cos (a1));
        juce::Path head; head.addTriangle (tip.x + (float) curve * 4.0f, tip.y, tip.x - (float) curve * 1.5f, tip.y - 4.0f, tip.x - (float) curve * 1.5f, tip.y + 4.0f);
        g.fillPath (head);
        return;
    }
    g.setFont (caps ((float) b.getProperties().getWithDefault ("fontSize", 9.5f), 0.15f));
    g.drawFittedText (upper (b.getButtonText()), r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

// os mandos: disco mecanizado, anel de LED laranxa e o valor nun visor debaixo
void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    auto lcd = area.removeFromBottom (17.0f);
    lcd = lcd.withSizeKeepingCentre (juce::jmin (66.0f, area.getWidth() - 2.0f), 17.0f);
    area.removeFromBottom (3.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.withSizeKeepingCentre (d, d).getCentre();
    const float ring = d * 0.5f - 4.0f;
    const auto luz = s.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["led"]) : col::laranxa;

    juce::Path track; track.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
    const double zeroAt = bip ? 0.0 : s.getMinimum();
    const float zero = a0 + (float) s.valueToProportionOfLength (zeroAt) * (a1 - a0);
    const float ang = a0 + pos * (a1 - a0);
    if (std::abs (ang - zero) > 0.01f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, ring, ring, 0.0f, juce::jmin (zero, ang), juce::jmax (zero, ang), true);
        const auto lit = s.isEnabled() ? luz : col::crema3;
        g.setColour (lit.withAlpha (0.12f)); g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit.withAlpha (0.26f)); g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit); g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = ring - 7.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disc (juce::Colour (0xff4a5577), c.x - kr * 0.3f, c.y - kr * 0.45f, juce::Colour (0xff0e1323), c.x + kr * 0.55f, c.y + kr * 1.05f, true);
    disc.addColour (0.55, juce::Colour (0xff232b45));
    g.setGradientFill (disc);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);
    const juce::Point<float> tip (c.x + kr * 0.82f * std::sin (ang), c.y - kr * 0.82f * std::cos (ang));
    g.setColour (col::crema.withAlpha (s.isEnabled() ? 0.92f : 0.4f));
    g.drawLine ({ c, tip }, 2.0f);

    paintWell (g, lcd, 5.0f);
    g.setFont (caps (9.0f, 0.04f));
    glowText (g, upper (s.getTextFromValue (s.getValue())), lcd, juce::Justification::centred, s.isEnabled() ? col::ouro : col::crema3);
}

// os deslizadores: a pista afundida, a luz laranxa desde abaixo ata o capuchón e o capuchón mecanizado
void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y, bot = (float) (y + h);
    auto slot = juce::Rectangle<float> (cx - 4.5f, top - 7.0f, 9.0f, bot - top + 14.0f);
    paintWell (g, slot, 4.5f);
    const int steps = s.getMaximum() - s.getMinimum() <= 3.5 ? (int) (s.getMaximum() - s.getMinimum()) : 10;
    g.setColour (col::crema3.withAlpha (0.55f));
    for (int t = 0; t <= steps; ++t)
    {
        const float ty = bot - (bot - top) * (float) t / (float) juce::jmax (1, steps);
        const float len = (t % 5 == 0 || steps <= 3) ? 5.0f : 2.5f;
        g.fillRect (cx - 9.0f - len, ty - 0.5f, len, 1.0f);
        g.fillRect (cx + 9.0f, ty - 0.5f, len, 1.0f);
    }
    if (pos < bot - 0.5f)
    {
        auto lit = juce::Rectangle<float> (cx - 1.5f, pos, 3.0f, bot + 4.0f - pos);
        g.setColour (col::laranxa.withAlpha (0.14f)); g.fillRoundedRectangle (lit.expanded (3.0f, 0.0f), 3.0f);
        g.setColour (col::laranxa.withAlpha (0.30f)); g.fillRoundedRectangle (lit.expanded (1.5f, 0.0f), 2.0f);
        g.setColour (col::laranxa); g.fillRoundedRectangle (lit, 1.5f);
    }
    auto cap = juce::Rectangle<float> (cx - 13.0f, pos - 7.0f, 26.0f, 14.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (cap.translated (0.0f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5577), cap.getX(), cap.getY(), juce::Colour (0xff141a2e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (cap.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (col::crema); g.fillRect (cap.getX() + 4.0f, pos - 0.8f, cap.getWidth() - 8.0f, 1.6f);
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (13.5f), col::crema);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (13.5f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2);
    g.setColour (col::lina); g.drawRect (0, 0, w, h);
}

// ================================================================ utilidades
static juce::Colour corDe (uint32_t c) { return juce::Colour ((juce::uint32) (0xff000000u | c)); }
static juce::String tempoTxt (double s) { return s < 0.995 ? juce::String (juce::roundToInt (s * 1000.0)) + " ms" : numero (s, s < 9.95 ? 1 : 0) + " s"; }
static juce::String hzTxt (double f) { return f < 999.5 ? juce::String (juce::roundToInt (f)) + " Hz" : numero (f / 1000.0, f < 9950 ? 1 : 0) + " kHz"; }
static juce::String enteiro (double v) { return juce::String (juce::roundToInt (v)); }
static juce::String dbTxt (double db, int dec = 0)
{
    const double lim = dec == 0 ? 0.5 : 0.05;
    return (db >= lim ? "+" : db <= -lim ? U ("−") : juce::String()) + (dec == 0 ? enteiro (std::abs (db)) : numero (std::abs (db), dec)) + " dB";
}
static juce::Point<float> polar (juce::Point<float> c, float r, float a) { return { c.x + r * std::sin (a), c.y - r * std::cos (a) }; }
constexpr float A0 = juce::MathConstants<float>::pi * 1.25f, A1 = juce::MathConstants<float>::pi * 2.75f;

// un trazo con luz: un halo ancho e transparente e a liña por riba
static void trazo (juce::Graphics& g, const juce::Path& p, juce::Colour c, float ancho = 1.8f)
{
    g.setColour (c.withAlpha (0.18f)); g.strokePath (p, juce::PathStrokeType (ancho + 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (ancho + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.brighter (0.25f)); g.strokePath (p, juce::PathStrokeType (ancho, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
// o recheo baixo un trazo, que se apaga cara abaixo
static void recheo (juce::Graphics& g, juce::Path p, juce::Rectangle<float> a, juce::Colour c)
{
    p.lineTo (p.getCurrentPosition().x, a.getBottom());
    auto ini = p.getBounds();
    p.lineTo (ini.getX(), a.getBottom());
    p.closeSubPath();
    g.setGradientFill (juce::ColourGradient (c.withAlpha (0.30f), 0.0f, a.getY(), c.withAlpha (0.02f), 0.0f, a.getBottom(), false));
    g.fillPath (p);
}
static void raiado (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour c)
{
    juce::Path p; p.startNewSubPath (a); p.lineTo (b);
    juce::Path d; const float dash[] = { 3.0f, 3.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (d, p, dash, 2);
    g.setColour (c); g.fillPath (d);
}

// ================================================================ mando grande
MandoGrande::MandoGrande()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (A0, A1, true);
    setMouseDragSensitivity (260);
    setVelocityBasedMode (false);
}

void MandoGrande::paint (juce::Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float pos = (float) valueToProportionOfLength (getValue());
    // o anel de LED: 31 puntos
    for (int k = 0; k < 31; ++k)
    {
        const float u = (float) k / 30.0f;
        const bool on = bipolar ? (u >= juce::jmin (0.5f, pos) - 1e-4f && u <= juce::jmax (0.5f, pos) + 1e-4f && std::abs (pos - 0.5f) > 0.004f)
                                : (u <= pos + 1e-4f && pos > 0.002f);
        const auto p = polar (c, 86.0f, A0 + u * (A1 - A0));
        if (on)
        {
            g.setColour (cor.withAlpha (0.16f)); g.fillEllipse (p.x - 6.5f, p.y - 6.5f, 13.0f, 13.0f);
            g.setColour (cor.withAlpha (0.35f)); g.fillEllipse (p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
            g.setColour (cor.brighter (0.35f)); g.fillEllipse (p.x - 2.4f, p.y - 2.4f, 4.8f, 4.8f);
        }
        else
        {
            g.setColour (juce::Colour (0xff242c4a)); g.fillEllipse (p.x - 2.3f, p.y - 2.3f, 4.6f, 4.6f);
        }
    }
    // sombra, saia moleteada e tapa metálica
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (c.x - 72.0f, c.y - 66.0f, 144.0f, 150.0f);
    g.setColour (juce::Colour (0xff0e1326)); g.fillEllipse (c.x - 70.0f, c.y - 70.0f, 140.0f, 140.0f);
    for (int k = 0; k < 72; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::twoPi / 72.0f;
        g.setColour (k % 2 == 0 ? juce::Colour (0xff2c3658) : juce::Colour (0xff161c32));
        g.drawLine ({ polar (c, 62.0f, a), polar (c, 69.0f, a) }, 1.6f);
    }
    juce::ColourGradient tapa (juce::Colour (0xff66739e), c.x - 18.0f, c.y - 24.0f, juce::Colour (0xff1e2541), c.x + 40.0f, c.y + 52.0f, true);
    tapa.addColour (0.45, juce::Colour (0xff36406a));
    g.setGradientFill (tapa);
    g.fillEllipse (c.x - 58.0f, c.y - 58.0f, 116.0f, 116.0f);
    g.setColour (juce::Colour (0xff10152a)); g.drawEllipse (c.x - 58.0f, c.y - 58.0f, 116.0f, 116.0f, 1.2f);
    juce::Path brillo; brillo.addCentredArc (c.x, c.y, 56.0f, 56.0f, 0.0f, -1.9f, -0.35f, true);
    g.setColour (juce::Colours::white.withAlpha (0.18f)); g.strokePath (brillo, juce::PathStrokeType (1.0f));
    // o indicador, na cor
    const float a = A0 + pos * (A1 - A0);
    juce::Path ind; ind.startNewSubPath (polar (c, 20.0f, a)); ind.lineTo (polar (c, 50.0f, a));
    g.setColour (cor.withAlpha (0.25f)); g.strokePath (ind, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (cor.brighter (0.4f)); g.strokePath (ind, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ================================================================ mando pequeno
MandoPequeno::MandoPequeno()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (A0, A1, true);
    setMouseDragSensitivity (160);
}

void MandoPequeno::paint (juce::Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float pos = (float) valueToProportionOfLength (getValue());
    juce::Path pista; pista.addCentredArc (c.x, c.y, 16.0f, 16.0f, 0.0f, A0, A1, true);
    g.setColour (juce::Colour (0xff1e2642)); g.strokePath (pista, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float mid = A0 + 0.5f * (A1 - A0), a = A0 + pos * (A1 - A0);
    if (std::abs (a - mid) > 0.01f)
    {
        juce::Path v; v.addCentredArc (c.x, c.y, 16.0f, 16.0f, 0.0f, juce::jmin (a, mid), juce::jmax (a, mid), true);
        trazo (g, v, cor, 2.2f);
    }
    g.setColour (juce::Colour (0xff3a4570)); g.fillEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f);
    g.setColour (juce::Colour (0xff10152a)); g.drawEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f, 1.0f);
    g.setColour (col::crema); g.drawLine ({ polar (c, 3.0f, a), polar (c, 9.0f, a) }, 2.0f);
}

// ================================================================ luz
void Luz::paint (juce::Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    if (acesa)
    {
        g.setColour (cor.withAlpha (0.18f)); g.fillEllipse (c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f);
        g.setColour (cor.withAlpha (0.40f)); g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 11.0f, 11.0f);
        g.setColour (cor.brighter (0.4f)); g.fillEllipse (c.x - 3.6f, c.y - 3.6f, 7.2f, 7.2f);
        g.setColour (juce::Colours::white.withAlpha (0.6f)); g.fillEllipse (c.x - 1.6f, c.y - 2.4f, 2.6f, 2.2f);
    }
    else
    {
        g.setColour (juce::Colour (0xff2a2f44)); g.fillEllipse (c.x - 3.6f, c.y - 3.6f, 7.2f, 7.2f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.6f, c.y - 3.6f, 7.2f, 7.2f, 0.8f);
    }
}

// ================================================================ a pantalla
void Pantalla::tick()
{
    auto caer = [] (float& v, float novo) { v = juce::jmax (novo, v * 0.86f); };
    caer (entrada, proc.nivelEntrada.load()); caer (saida, proc.nivelSaida.load());
    red = juce::jmin (proc.reducion.load(), red * 0.85f);   // (negativa: baixa de golpe e volve amodo)
    historia[(size_t) hPos] = proc.nivelSaida.load();
    hPos = (hPos + 1) % (int) historia.size();
    repaint();
}

void Pantalla::rotulos (juce::Graphics& g, const juce::String& esq, const juce::String& der, juce::Colour c)
{
    auto top = getLocalBounds().toFloat().reduced (12.0f, 0.0f).withTrimmedTop (8.0f).withHeight (12.0f);
    g.setColour (col::crema3); drawCaps (g, esq, top, juce::Justification::centredLeft, 6.8f, 0.22f);
    g.setColour (c); drawCaps (g, der, top, juce::Justification::centredRight, 6.8f, 0.14f);
}

void Pantalla::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    paintGlass (g, r, 10.0f);
    const auto c = corDe (proc.descricion().cor);
    auto a = r.reduced (12.0f, 0.0f).withTrimmedTop (24.0f).withTrimmedBottom (10.0f);
    g.setColour (col::lina.withAlpha (0.55f));
    for (int k = 1; k < 4; ++k) g.fillRect (a.getX(), a.getY() + a.getHeight() * (float) k / 4.0f, a.getWidth(), 0.8f);
    for (int k = 1; k < 6; ++k) g.fillRect (a.getX() + a.getWidth() * (float) k / 6.0f, a.getY(), 0.8f, a.getHeight());
    switch (proc.tipo())
    {
        case enx::AMBIENTE:   ambiente (g, a, c); break;
        case enx::SATURACION: saturacion (g, a, c); break;
        case enx::BRILLO:     eq (g, a, c, true); break;
        case enx::GRAVES:     eq (g, a, c, false); break;
        case enx::VOLUME:     volume (g, a, c); break;
        case enx::COMPRESOR:  compresor (g, a, c); break;
        case enx::SIDECHAIN:  sidechain (g, a, c); break;
        case enx::FILTRO:     filtro (g, a, c); break;
        default:              chorus (g, a, c); break;
    }
}

void Pantalla::ambiente (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    const double m = proc.mando();
    const auto ax = enx::Ambiente::axustes (m, proc.selector());
    const double mest = enx::Ambiente::mestura (m);
    const double tM = verb::Espazo::tempoBanda (ax, 1), pre = ax.predelay / 1000.0;
    const double tmax = juce::jmax (0.6, (pre + tM) * 1.05);
    auto x = [&] (double t) { return a.getX() + (float) (t / tmax) * a.getWidth(); };
    auto y = [&] (double db) { return a.getBottom() - (float) juce::jlimit (0.0, 1.0, (db + 48.0) / 48.0) * a.getHeight(); };
    rotulos (g, U ("Caída"), mest > 0 ? tempoTxt (tM) : U ("Seco"), c);
    if (mest <= 0) return;
    const double nivel = enx::gananciaADb (std::sin (mest * enx::PI / 2) / std::sin (0.44 * enx::PI / 2));   // máis mestura, máis alta
    for (int k = 0; k < verb::Espazo::numReflexions(); ++k)
    {
        const double n = verb::Espazo::reflexionNivel (ax, k);
        if (n <= 0.001) continue;
        const float px = x (pre + verb::Espazo::reflexionMs (ax, k) / 1000.0), py = y (nivel + enx::gananciaADb (n) - 2.0);
        g.setColour (col::crema.withAlpha (0.7f)); g.fillRect (px, py, 1.2f, a.getBottom() - py);
    }
    juce::Path p;
    juce::Random rnd (17);
    p.startNewSubPath (x (pre), y (nivel));
    for (int k = 1; k <= 160; ++k)
    {
        const double t = pre + tM * k / 160.0;
        const double db = nivel - 60.0 * (t - pre) / tM - 4.0 * rnd.nextFloat();
        p.lineTo (x (t), y (db));
    }
    recheo (g, p, a, c);
    trazo (g, p, c, 1.4f);
}

void Pantalla::saturacion (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    const double m = proc.mando();
    const int sel = proc.selector();
    const double gi = enx::dbAGanancia (enx::Saturacion::ganancia (m, sel));
    const double top = std::abs (enx::Saturacion::curva (gi, sel));
    rotulos (g, U ("Curva"), dbTxt (enx::Saturacion::ganancia (m, sel)), c);
    raiado (g, { a.getX(), a.getBottom() }, { a.getRight(), a.getY() }, col::crema3.withAlpha (0.6f));
    juce::Path p;
    for (int k = 0; k <= 120; ++k)
    {
        const double xin = -1.0 + 2.0 * k / 120.0;
        const double yo = enx::Saturacion::curva (xin * gi, sel) / juce::jmax (1e-9, top);
        const juce::Point<float> pt (a.getX() + (float) (xin + 1.0) * 0.5f * a.getWidth(), a.getBottom() - (float) (juce::jlimit (-1.1, 1.1, yo) + 1.0) * 0.5f * a.getHeight());
        if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    trazo (g, p, c, 1.8f);
}

void Pantalla::eq (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c, bool agudos)
{
    const double m = proc.mando(), fs = proc.frecuencia();
    const int sel = proc.selector();
    enx::Biquad b;
    double ganancia, f0;
    if (agudos) { f0 = enx::Brillo::FREC[sel]; ganancia = enx::Brillo::realce (m); b.andelAgudos (fs, juce::jmin (f0, fs * 0.42), ganancia, 0.9); }
    else { f0 = enx::Graves::FREC[sel]; ganancia = enx::Graves::realce (m); b.andelGraves (fs, f0 * 1.6, ganancia, 0.9); }
    rotulos (g, "EQ", dbTxt (ganancia) + U (" · ") + hzTxt (f0), c);
    auto x = [&] (double f) { return a.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * a.getWidth(); };
    auto y = [&] (double db) { return a.getBottom() - (float) juce::jlimit (0.0, 1.0, (db + 3.0) / 18.0) * a.getHeight(); };
    juce::Path p;
    for (int k = 0; k <= 160; ++k)
    {
        const double f = 20.0 * std::pow (1000.0, k / 160.0);
        const auto pt = juce::Point<float> (x (f), y (f < fs * 0.49 ? b.db (fs, f) : ganancia));
        if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    recheo (g, p, a, c);
    if (agudos)
    {
        juce::Random rnd (5);
        const int n = (int) (m * 16);
        const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
        for (int k = 0; k < n; ++k)
        {
            const float px = x (f0 * (0.7 + rnd.nextFloat() * 1.3)), py = a.getY() + a.getHeight() * (0.05f + 0.35f * rnd.nextFloat());
            const float tw = 0.5f + 0.5f * (float) std::sin (t * (2.0 + rnd.nextFloat() * 3.0) + k);
            g.setColour (c.withAlpha (0.35f + 0.6f * tw));
            const float rr = 0.8f + 1.4f * tw;
            g.fillEllipse (px - rr, py - rr, 2 * rr, 2 * rr);
        }
    }
    else
    {
        const double h = enx::Graves::harmonicos (m) / 0.6;
        for (int k = 2; k <= 3; ++k)
        {
            const float px = x (f0 * k), alto = (float) h * a.getHeight() * (k == 2 ? 0.45f : 0.3f);
            g.setColour (c.withAlpha (0.45f));
            g.fillRoundedRectangle (px - 3.0f, a.getBottom() - alto, 6.0f, alto, 2.0f);
        }
    }
    trazo (g, p, c, 1.6f);
}

void Pantalla::volume (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    rotulos (g, U ("Teito −1 dB"), dbTxt (enx::Volume::ganancia (proc.mando()), 1), c);
    auto plot = a.withTrimmedRight (16.0f);
    auto y = [&] (double db) { return plot.getBottom() - (float) juce::jlimit (0.0, 1.0, (db + 36.0) / 36.0) * plot.getHeight(); };
    const int n = (int) historia.size();
    const float w = plot.getWidth() / (float) n;
    for (int k = 0; k < n; ++k)
    {
        const float v = historia[(size_t) ((hPos + k) % n)];
        const double db = enx::gananciaADb (v);
        const float top = y (db);
        g.setColour ((db > -1.6 ? c : c.interpolatedWith (col::lina, 0.45f)).withAlpha (0.9f));
        g.fillRoundedRectangle (plot.getX() + (float) k * w + 0.6f, top, juce::jmax (1.0f, w - 1.2f), plot.getBottom() - top, 1.0f);
    }
    raiado (g, { plot.getX(), y (-1.0) }, { plot.getRight(), y (-1.0) }, col::crema.withAlpha (0.85f));
    // a redución, desde arriba
    auto gr = juce::Rectangle<float> (a.getRight() - 9.0f, a.getY(), 7.0f, a.getHeight());
    g.setColour (col::visor); g.fillRoundedRectangle (gr, 2.0f);
    const float alto = (float) juce::jlimit (0.0, 1.0, -red / 12.0) * gr.getHeight();
    g.setColour (c); g.fillRoundedRectangle (gr.withHeight (alto), 2.0f);
}

void Pantalla::compresor (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    const double m = proc.mando();
    rotulos (g, numero (enx::Compresor::proporcion (m), 1) + ":1", dbTxt (red, 1), c);
    auto plot = a.withTrimmedRight (16.0f);
    auto x = [&] (double db) { return plot.getX() + (float) juce::jlimit (0.0, 1.0, (db + 48.0) / 48.0) * plot.getWidth(); };
    auto y = [&] (double db) { return plot.getBottom() - (float) juce::jlimit (0.0, 1.0, (db + 48.0) / 48.0) * plot.getHeight(); };
    raiado (g, { x (-48), y (-48) }, { x (0), y (0) }, col::crema3.withAlpha (0.6f));
    juce::Path p;
    for (int k = 0; k <= 120; ++k)
    {
        const double xin = -48.0 + 48.0 * k / 120.0;
        const auto pt = juce::Point<float> (x (xin), y (enx::Compresor::curva (xin, m)));
        if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    trazo (g, p, c, 1.8f);
    const double ent = enx::gananciaADb (entrada);
    if (ent > -48.0)
    {
        const auto pt = juce::Point<float> (x (ent), y (enx::Compresor::curva (ent, m)));
        g.setColour (c.withAlpha (0.3f)); g.fillEllipse (pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);
        g.setColour (col::crema); g.fillEllipse (pt.x - 2.6f, pt.y - 2.6f, 5.2f, 5.2f);
    }
    auto gr = juce::Rectangle<float> (a.getRight() - 9.0f, a.getY(), 7.0f, a.getHeight());
    g.setColour (col::visor); g.fillRoundedRectangle (gr, 2.0f);
    g.setColour (c); g.fillRoundedRectangle (gr.withHeight ((float) juce::jlimit (0.0, 1.0, -red / 20.0) * gr.getHeight()), 2.0f);
}

void Pantalla::sidechain (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    const double m = proc.mando(), bpm = proc.tempoDAW();
    const int sel = proc.selector();
    const double fondo = enx::Sidechain::fondura (m), cicloS = enx::Sidechain::NEGRAS[sel] * 60.0 / bpm;
    const float desp = proc.apvts.getRawParameterValue ("desprazar")->load();
    rotulos (g, U (enx::FERRAMENTAS[enx::SIDECHAIN].opcions[sel]) + U (" · ") + enteiro (bpm) + " BPM", dbTxt (-fondo), c);
    const double escala = juce::jmax (12.0, fondo);
    auto y = [&] (double db) { return a.getY() + 4.0f + (float) (-db / escala) * (a.getHeight() - 4.0f); };
    const double off = desp / 1000.0 / cicloS;   // en ciclos
    juce::Path p;
    for (int k = 0; k <= 320; ++k)
    {
        const double u = 4.0 * k / 320.0;
        double f = u - off; f -= std::floor (f);
        const double db = -fondo * (1.0 - enx::Sidechain::forma (f * cicloS, cicloS));
        const auto pt = juce::Point<float> (a.getX() + (float) (u / 4.0) * a.getWidth(), y (db));
        if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    recheo (g, p, a, c);
    trazo (g, p, c, 1.6f);
    for (int k = 0; k < 4; ++k)   // os tempos (o bombo)
    {
        const float px = a.getX() + (float) k / 4.0f * a.getWidth();
        juce::Path t; t.addTriangle (px, a.getBottom() - 7.0f, px - 4.0f, a.getBottom(), px + 4.0f, a.getBottom());
        g.setColour (col::crema.withAlpha (0.85f)); g.fillPath (t);
    }
    // por onde vai
    const double fz = proc.fase.load();
    const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const int volta = (int) std::fmod (std::floor (t / cicloS), 4.0);
    const double u = ((double) volta + fz + off) / 4.0;
    const double db = -fondo * (1.0 - enx::Sidechain::forma (fz * cicloS, cicloS));
    const auto pt = juce::Point<float> (a.getX() + (float) (u - std::floor (u)) * a.getWidth(), y (db));
    g.setColour (c.withAlpha (0.3f)); g.fillEllipse (pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);
    g.setColour (col::crema); g.fillEllipse (pt.x - 2.6f, pt.y - 2.6f, 5.2f, 5.2f);
}

void Pantalla::filtro (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    const double v = proc.mando();
    const int sel = proc.selector();
    auto x = [&] (double f) { return a.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * a.getWidth(); };
    auto y = [&] (double db) { return a.getBottom() - (float) juce::jlimit (0.0, 1.0, (db + 36.0) / 54.0) * a.getHeight(); };
    if (std::abs (v) < 1e-4)
    {
        rotulos (g, "Plano", "", c);
        juce::Path p; p.startNewSubPath (a.getX(), y (0)); p.lineTo (a.getRight(), y (0));
        trazo (g, p, c, 1.6f);
        return;
    }
    const double fc = enx::Filtro::corte (v), wet = juce::jmin (1.0, std::abs (v) / enx::Filtro::FUNDIDO);
    rotulos (g, v < 0 ? U ("Paso baixo") : U ("Paso alto"), hzTxt (fc), c);
    juce::Path p;
    for (int k = 0; k <= 200; ++k)
    {
        const double f = 20.0 * std::pow (1000.0, k / 200.0);
        const std::complex<double> s (0.0, f / fc);
        std::complex<double> h;
        if (v < 0)
        {
            const double kr = enx::Filtro::resonanciaJuno (v, sel);
            const auto G = 1.0 / (1.0 + s), G4 = G * G * G * G;
            h = G4 / (1.0 + kr * G4) * (1.0 + 0.5 * kr);
        }
        else
        {
            const double q = enx::Filtro::qPasoAlto (v, sel);
            h = s * s / (s * s + s / q + 1.0);
        }
        h = (1.0 - wet) + wet * h;
        const auto pt = juce::Point<float> (x (f), y (20.0 * std::log10 (std::abs (h) + 1e-9)));
        if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    recheo (g, p, a, c);
    trazo (g, p, c, 1.8f);
}

void Pantalla::chorus (juce::Graphics& g, juce::Rectangle<float> a, juce::Colour c)
{
    const int sel = proc.selector();
    const double vel = enx::Chorus::VEL[sel], fondo = enx::Chorus::FONDO[sel] * 1000.0;
    rotulos (g, U ("Modo ") + U (enx::FERRAMENTAS[enx::CHORUS].opcions[sel]) + U (" · ") + numero (vel, 2) + " Hz",
             U ("±") + numero (fondo, 2) + " ms", c);
    const double voltas = sel == 2 ? 6.0 : 2.0;
    auto y = [&] (double ms) { return a.getCentreY() - (float) ((ms - 3.3) / 2.4) * a.getHeight() * 0.5f; };
    for (int linha = 0; linha < 2; ++linha)
    {
        juce::Path p;
        for (int k = 0; k <= 240; ++k)
        {
            const double u = voltas * k / 240.0;
            double f = u; f -= std::floor (f);
            double tri = 1.0 - 4.0 * std::abs (f - 0.5);
            if (linha == 1 && sel != 2) tri = -tri;
            const auto pt = juce::Point<float> (a.getX() + (float) (u / voltas) * a.getWidth(), y (3.3 + fondo * tri));
            if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        trazo (g, p, linha == 0 ? c : c.interpolatedWith (col::crema, 0.5f), 1.4f);
    }
    const double fz = proc.fase.load();
    const double m = proc.mando();
    for (int linha = 0; linha < 2; ++linha)
    {
        double tri = 1.0 - 4.0 * std::abs (fz - 0.5);
        if (linha == 1 && sel != 2) tri = -tri;
        const auto pt = juce::Point<float> (a.getX() + (float) (fz / voltas) * a.getWidth(), y (3.3 + fondo * tri));
        g.setColour (c.withAlpha (0.2f + 0.3f * (float) m)); g.fillEllipse (pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);
        g.setColour (col::crema); g.fillEllipse (pt.x - 2.4f, pt.y - 2.4f, 4.8f, 4.8f);
    }
}

// ================================================================ vista principal
MainView::MainView (EnxProcessor& p) : proc (p), pantalla (p)
{
    setLookAndFeel (&look);
    const auto& d = proc.descricion();
    cor = corDe (d.cor);

    mando.cor = cor;
    mando.bipolar = d.bipolar;
    mando.setTooltip (U (d.axudaMando));
    mandoAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mando", mando);
    mando.setDoubleClickReturnValue (true, d.bipolar ? 0.0 : d.mandoInicial);
    mando.onValueChange = [this] { pantalla.repaint(); repaint(); };
    addAndMakeVisible (mando);

    for (int i = 0; i < 3; ++i)
    {
        opcions[i].setButtonText (U (d.opcions[i]));
        opcions[i].getProperties().set ("led", (juce::int64) cor.getARGB());
        opcions[i].getProperties().set ("fontSize", 8.5f);
        opcions[i].setTooltip (U (d.axudaSelector));
        opcions[i].setWantsKeyboardFocus (false);
        opcions[i].onClick = [this, i] { selectorAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (opcions[i]);
    }
    auto* sel = proc.apvts.getParameter ("selector");
    selectorAtt = std::make_unique<juce::ParameterAttachment> (*sel, [this, sel] (float v)
    {
        const int k = (int) std::lround (sel->convertFrom0to1 (sel->convertTo0to1 (v)));
        for (int i = 0; i < 3; ++i) opcions[i].setToggleState (i == k, juce::dontSendNotification);
        pantalla.repaint();
    });
    selectorAtt->sendInitialUpdate();

    if (proc.tipo() == enx::SIDECHAIN)
    {
        desprazar.cor = cor;
        desprazar.setTooltip (U ("Move a baixada respecto ao tempo: cara atrás (antes do golpe) ou cara adiante (despois), ata 50 ms."));
        desprazarAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "desprazar", desprazar);
        desprazar.setDoubleClickReturnValue (true, 0.0);
        desprazar.onValueChange = [this] { pantalla.repaint(); repaint(); };
        addAndMakeVisible (desprazar);
    }

    luz.cor = cor;
    luz.setTooltip (U ("Acendido. Un clic: apágase (bypass) e o son pasa tal cal."));
    auto* byp = proc.apvts.getParameter ("bypass");
    bypassAtt = std::make_unique<juce::ParameterAttachment> (*byp, [this] (float v)
    {
        luz.acesa = v < 0.5f;
        luz.setTooltip (luz.acesa ? U ("Acendido. Un clic: apágase (bypass) e o son pasa tal cal.") : U ("Apagado (bypass). Un clic: acéndese."));
        luz.repaint(); repaint();
    });
    bypassAtt->sendInitialUpdate();
    luz.onClick = [this] { bypassAtt->setValueAsCompleteGesture (luz.acesa ? 1.0f : 0.0f); };
    addAndMakeVisible (luz);

    pantalla.setTooltip (U (d.descricion));
    addAndMakeVisible (pantalla);

    newVersion.getProperties().set ("noLed", true);
    newVersion.getProperties().set ("fontSize", 7.5f);
    newVersion.onClick = [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); };
    newVersion.setWantsKeyboardFocus (false);
    addChildComponent (newVersion);

    setSize (W, H);
    startTimerHz (30);
}

MainView::~MainView()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::String MainView::lectura() const
{
    const double m = proc.mando();
    if (proc.descricion().bipolar)
    {
        if (std::abs (m) < 1e-4) return U ("Plano");
        return (m < 0 ? U ("Escuro ") : U ("Fino ")) + juce::String (juce::roundToInt (std::abs (m) * 100.0)) + " %";
    }
    return juce::String (juce::roundToInt (m * 100.0)) + " %";
}

void MainView::timerCallback()
{
    pantalla.tick();
    auto caer = [] (float& v, float novo) { v = juce::jmax (novo, v * 0.86f); };
    caer (entrada, proc.nivelEntrada.load()); caer (saida, proc.nivelSaida.load());
    repaint (0, 230, 60, 160);
    repaint (W - 60, 230, 60, 160);
    const auto nv = versions->newVersion();
    if (nv.isNotEmpty() != newVersion.isVisible())
    {
        newVersion.setButtonText (U ("Versión nova: ") + nv + U (" · descargar"));
        newVersion.setVisible (nv.isNotEmpty());
        repaint();
    }
}

void MainView::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    // o corpo: azul mariño, coa luz da ferramenta detrás do mando
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f2947), 0.0f, 0.0f, juce::Colour (0xff0f152a), 0.0f, r.getBottom(), false));
    g.fillAll();
    {
        juce::ColourGradient halo (cor.withAlpha (luz.acesa ? 0.13f : 0.04f), (float) W * 0.5f, 300.0f, cor.withAlpha (0.0f), (float) W * 0.5f + 130.0f, 300.0f, true);
        g.setGradientFill (halo);
        g.fillEllipse ((float) W * 0.5f - 130.0f, 170.0f, 260.0f, 260.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (20.0f, 0.5f, (float) W - 40.0f, 1.0f);
    g.setColour (juce::Colour (0xff303c62)); g.drawRect (r, 1.0f);
    for (auto p : { juce::Point<float> (13, 13), juce::Point<float> ((float) W - 13, 13), juce::Point<float> (13, (float) H - 13), juce::Point<float> ((float) W - 13, (float) H - 13) })
    {
        g.setColour (juce::Colour (0xff0a0e1c)); g.fillEllipse (p.x - 3.2f, p.y - 3.2f, 6.4f, 6.4f);
        g.setColour (juce::Colour (0xff3c4870)); g.drawLine (p.x - 2.0f, p.y - 2.0f, p.x + 2.0f, p.y + 2.0f, 0.8f);
    }

    // a cabeceira
    g.setColour (col::crema3);
    drawCaps (g, "BARULLO", { 24.0f, 24.0f, 120.0f, 12.0f }, juce::Justification::centredLeft, 7.5f, 0.42f);
    g.setColour (col::crema);
    g.setFont (cond (21.0f));
    g.drawText (U (proc.descricion().curto), juce::Rectangle<float> (22.0f, 36.0f, 260.0f, 26.0f), juce::Justification::centredLeft, false);

    // a escala do mando
    const juce::Point<float> c ((float) W * 0.5f, 300.0f);
    for (int k = 0; k <= 10; ++k)
    {
        const float a = A0 + (float) k / 10.0f * (A1 - A0);
        g.setColour (col::crema3);
        g.drawLine ({ polar (c, 98.0f, a), polar (c, k % 5 == 0 ? 105.0f : 102.0f, a) }, 1.1f);
    }
    const bool bip = proc.descricion().bipolar;
    const char* marcas[3] = { bip ? "−" : "0", bip ? "0" : "5", bip ? "+" : "10" };
    for (int k = 0; k < 3; ++k)
    {
        const auto p = polar (c, 115.0f, A0 + (float) k * 0.5f * (A1 - A0));
        g.setFont (caps (8.0f, 0.0f));
        g.drawText (U (marcas[k]), juce::Rectangle<float> (p.x - 12.0f, p.y - 6.0f, 24.0f, 12.0f), juce::Justification::centred, false);
    }

    // os medidores: entrada e saída
    for (int lado = 0; lado < 2; ++lado)
    {
        const float mx = lado == 0 ? 30.0f : (float) W - 30.0f;
        const float db = juce::Decibels::gainToDecibels (lado == 0 ? entrada : saida, -60.0f);
        const float nivel = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        for (int k = 0; k < 14; ++k)
        {
            const float y = 352.0f - (float) k * 8.0f;
            const bool on = (float) k / 13.0f <= nivel && nivel > 0.0f;
            const auto cc = on ? juce::Colour (0xff50d48e).interpolatedWith (cor, (float) k / 13.0f) : juce::Colour (0xff1e2642);
            if (on) { g.setColour (cc.withAlpha (0.25f)); g.fillRoundedRectangle (mx - 6.0f, y - 4.0f, 12.0f, 8.0f, 2.0f); }
            g.setColour (cc); g.fillRoundedRectangle (mx - 4.0f, y - 2.4f, 8.0f, 4.8f, 1.2f);
        }
        g.setColour (col::crema3);
        drawCaps (g, lado == 0 ? "ENT" : U ("SAÍ"), { mx - 16.0f, 362.0f, 32.0f, 10.0f }, juce::Justification::centred, 6.0f, 0.18f);
    }

    // o valor
    {
        const auto v = juce::Rectangle<float> ((float) W * 0.5f - 50.0f, 401.0f, 100.0f, 22.0f);
        paintWell (g, v, 6.0f);
        g.setFont (cond (11.0f));
        glowText (g, upper (lectura()), v, juce::Justification::centred, cor.brighter (0.2f));
    }

    // o selector (e, no sidechain, desprazar)
    {
        const auto ka = opcions[0].getBounds().getUnion (opcions[2].getBounds()).toFloat();
        paintWell (g, ka.expanded (4.0f), 9.0f);
        g.setColour (col::crema3);
        drawCaps (g, U (proc.descricion().rotuloSelector), ka.withY (ka.getY() - 20.0f).withHeight (12.0f), juce::Justification::centred, 6.8f, 0.3f);
        if (desprazar.isVisible())
        {
            const auto b = desprazar.getBounds().toFloat();
            const float ms = proc.apvts.getRawParameterValue ("desprazar")->load();
            drawCaps (g, "DESPRAZAR", { b.getCentreX() - 50.0f, ka.getY() - 20.0f, 100.0f, 12.0f }, juce::Justification::centred, 6.8f, 0.3f);
            g.setColour (cor);
            drawCaps (g, (ms > 0.05f ? "+" : ms < -0.05f ? U ("−") : juce::String()) + enteiro (std::abs (ms)) + " ms",
                      { b.getRight() + 2.0f, b.getCentreY() - 6.0f, 50.0f, 12.0f }, juce::Justification::centredLeft, 7.0f, 0.1f);
        }
    }

    // o pé
    g.setColour (col::crema3);
    if (! newVersion.isVisible())
    {
        const auto f = juce::Rectangle<float> (22.0f, (float) H - 24.0f, (float) W - 44.0f, 12.0f);
        const float we = juce::GlyphArrangement::getStringWidth (caps (7.0f, 0.3f), "ESCOITA");
        drawCaps (g, "ESCOITA", f, juce::Justification::centredLeft, 7.0f, 0.3f);
        g.setColour (col::laranxa);
        drawCaps (g, ".", f.withTrimmedLeft (we - 2.0f), juce::Justification::centredLeft, 7.0f, 0.3f);
        g.setColour (col::crema3);
        drawCaps (g, U ("Enxebres · v") + escoita::Actualizacions::currentVersion(), f, juce::Justification::centredRight, 7.0f, 0.3f);
    }
}

void MainView::resized()
{
    luz.setBounds (W - 44, 28, 26, 26);
    pantalla.setBounds (18, 72, W - 36, 116);
    mando.setBounds (W / 2 - 100, 200, 200, 200);
    const int ky = 452, kh = 26;
    if (proc.tipo() == enx::SIDECHAIN)
    {
        const int kw = 54, gap = 5;
        for (int i = 0; i < 3; ++i) opcions[i].setBounds (26 + i * (kw + gap), ky, kw, kh);
        desprazar.setBounds (W - 112, ky - 6, 38, 38);
    }
    else
    {
        const int kw = 92, gap = 6, x0 = W / 2 - (3 * kw + 2 * gap) / 2;
        for (int i = 0; i < 3; ++i) opcions[i].setBounds (x0 + i * (kw + gap), ky, kw, kh);
    }
    newVersion.setBounds (W / 2 - 120, H - 30, 240, 20);
}
} // namespace ui

// ================================================================ xanela (escalable)
EnxEditor::EnxEditor (EnxProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    addAndMakeVisible (main);
    setResizable (true, true);
    setResizeLimits (ui::MainView::W * 7 / 10, ui::MainView::H * 7 / 10, ui::MainView::W * 2, ui::MainView::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::MainView::W / ui::MainView::H);
    float s = proc.uiScale;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto ua = d->userArea;
        const float fit = juce::jmin ((float) (ua.getWidth() - 40) / (float) ui::MainView::W, (float) (ua.getHeight() - 90) / (float) ui::MainView::H);
        if (s > fit && fit > 0.4f) s = fit;
    }
    setSize (juce::roundToInt (ui::MainView::W * s), juce::roundToInt (ui::MainView::H * s));
}

void EnxEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::MainView::W;
    main.setBounds (0, 0, ui::MainView::W, ui::MainView::H);
    main.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}
