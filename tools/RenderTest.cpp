// Proba sen interface das nove ferramentas de Barullo Enxebres: que fai cada mando (medido), o
// teito, a independencia do tamaño de bloque, o bypass, o estado e o mono; tamén capturas.
//   BERenderTest [saida.wav]                          proba enteira (devolve o número de fallos)
//   BERenderTest --snapshot saida.png ferramenta      a interface dunha
//   BERenderTest --galeria saida.png                  as nove xuntas
//   BERenderTest --render saida.wav ferramenta [mando %] [selector]   un son de proba pola ferramenta
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/Todas.h"
#include <cstdio>
#include <cstring>

using escoita::U;
static constexpr double SR = 48000;

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
}

struct Cabeza : juce::AudioPlayHead
{
    double bpm = 120, ppq = 0; bool tocando = false;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (tocando); return p; }
};

static juce::AudioBuffer<float> procesar (EnxProcessor& p, const juce::AudioBuffer<float>& in, double len, int bloque, bool preparar = true)
{
    if (preparar) { p.setPlayConfigDetails (2, 2, SR, bloque); p.prepareToPlay (SR, bloque); }
    const int total = (int) (len * SR);
    juce::AudioBuffer<float> out (2, total), buf (2, bloque);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += bloque)
    {
        const int n = juce::jmin (bloque, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                if (pos + i < in.getNumSamples()) buf.setSample (c, i, in.getSample (c % in.getNumChannels(), pos + i));
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    return out;
}

static void setParam (EnxProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static juce::AudioBuffer<float> seno (double f, double db, double len)
{
    juce::AudioBuffer<float> b (2, (int) (len * SR));
    const float a = (float) enx::dbAGanancia (db);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float x = a * (float) std::sin (2.0 * enx::PI * f * i / SR);
        b.setSample (0, i, x); b.setSample (1, i, x);
    }
    return b;
}
static juce::AudioBuffer<float> ruido (double db, double len, int semente = 3)
{
    juce::AudioBuffer<float> b (2, (int) (len * SR));
    juce::Random rnd (semente);
    const float a = (float) enx::dbAGanancia (db) * 1.732f;   // ruído branco uniforme co RMS pedido
    for (int c = 0; c < 2; ++c) for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (c, i, a * (rnd.nextFloat() * 2 - 1));
    return b;
}
// o son de proba: golpes de ruído e un acorde
static juce::AudioBuffer<float> sonDeProba (double len = 2.0)
{
    juce::AudioBuffer<float> b (2, (int) (len * SR));
    b.clear();
    juce::Random rnd (1984);
    for (double t0 : { 0.0, 0.25, 0.5, 0.875 })
        for (int i = 0; i < (int) (0.12 * SR); ++i)
        {
            const float e = 0.5f * std::exp (-(float) i / (float) (0.025 * SR));
            const int k = (int) (t0 * SR) + i;
            if (k >= b.getNumSamples()) break;
            for (int c = 0; c < 2; ++c) b.setSample (c, k, b.getSample (c, k) + e * (rnd.nextFloat() * 2 - 1));
        }
    for (double f : { 65.41, 261.63, 329.63, 392.0, 523.25 })
        for (int i = 0; i < (int) (0.8 * SR); ++i)
        {
            const int k = (int) (1.1 * SR) + i;
            if (k >= b.getNumSamples()) break;
            const float x = 0.08f * (float) std::sin (2 * enx::PI * f * i / SR) * std::exp (-(float) i / (float) (0.3 * SR)) * juce::jmin (1.0f, (float) i / 100.0f);
            for (int c = 0; c < 2; ++c) b.setSample (c, k, b.getSample (c, k) + x);
        }
    return b;
}

static double rmsDb (const juce::AudioBuffer<float>& b, double t0, double t1, int canle = -1)
{
    double e = 0; int n = 0;
    for (int c = 0; c < 2; ++c)
    {
        if (canle >= 0 && c != canle) continue;
        for (int i = (int) (t0 * SR); i < juce::jmin (b.getNumSamples(), (int) (t1 * SR)); ++i) { e += (double) b.getSample (c, i) * b.getSample (c, i); ++n; }
    }
    return 10.0 * std::log10 (e / juce::jmax (1, n) + 1e-30);
}
static float pico (const juce::AudioBuffer<float>& b) { return juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples())); }
static bool finito (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < 2; ++c) for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}
static float diferenza (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float d = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < juce::jmin (a.getNumSamples(), b.getNumSamples()); ++i) d = juce::jmax (d, std::abs (a.getSample (c, i) - b.getSample (c, i)));
    return d;
}
// a ganancia (dB) dun seno a través da ferramenta (no estado estable)
static double ganancia (int tipo, float mando, int sel, double f, double db = -24.0)
{
    EnxProcessor p (tipo);
    setParam (p, "mando", mando); setParam (p, "selector", (float) sel);
    const auto o = procesar (p, seno (f, db, 1.0), 1.0, 512);
    return rmsDb (o, 0.5, 1.0) - (db - 3.0103);
}
static uint64_t pegada (const juce::AudioBuffer<float>& b)
{
    uint64_t h = 1469598103934665603ull;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            uint32_t u; std::memcpy (&u, &x, 4);
            for (int k = 0; k < 4; ++k) { h ^= (u >> (8 * k)) & 0xFF; h *= 1099511628211ull; }
        }
    return h;
}

static juce::Image captura (int tipo, float mando = -1000.0f)
{
    EnxProcessor p (tipo);
    if (mando > -999.0f) setParam (p, "mando", mando);
    p.setPlayConfigDetails (2, 2, SR, 512);
    p.prepareToPlay (SR, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* ce = dynamic_cast<EnxEditor*> (ed.get());
    ed->setSize (ui::MainView::W, ui::MainView::H);
    const auto son = sonDeProba();
    juce::AudioBuffer<float> buf (2, 512);
    for (int blk = 0; blk < 70; ++blk)
    {
        buf.clear();
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, son, c, (blk * 512) % (son.getNumSamples() - 512), 512);
        juce::MidiBuffer m; p.processBlock (buf, m);
        ce->view().timerCallback();
    }
    return ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
}

static void gardarPng (const juce::Image& img, const juce::File& png)
{
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "--snapshot" && argc > 3) { gardarPng (captura (atoi (argv[3])), juce::File (juce::String (argv[2]))); return 0; }
    if (mode == "--galeria" && argc > 2)
    {
        const int cw = ui::MainView::W * 2, ch = ui::MainView::H * 2, gap = 40, marxe = 60;
        juce::Image g (juce::Image::RGB, 3 * cw + 2 * gap + 2 * marxe, 3 * ch + 2 * gap + 2 * marxe, true);
        juce::Graphics gg (g);
        gg.fillAll (juce::Colour (0xff0b101e));
        const float mandos[enx::NUM_FERRAMENTAS] = { 62, 68, 55, 64, 72, 58, 70, 42, 75 };
        for (int t = 0; t < enx::NUM_FERRAMENTAS; ++t)
            gg.drawImageAt (captura (t, mandos[t]), marxe + (t % 3) * (cw + gap), marxe + (t / 3) * (ch + gap));
        gardarPng (g, juce::File (juce::String (argv[2])));
        return 0;
    }
    if (mode == "--render" && argc > 3)
    {
        EnxProcessor p (atoi (argv[3]));
        if (argc > 4) setParam (p, "mando", (float) atof (argv[4]));
        if (argc > 5) setParam (p, "selector", (float) atoi (argv[5]));
        writeWav (juce::File (juce::String (argv[2])), procesar (p, sonDeProba (3.0), 3.0 + juce::jmin (6.0, p.getTailLengthSeconds()), 512), SR);
        return 0;
    }

    int fails = 0;
    auto check = [&fails] (bool ok, const juce::String& what, const juce::String& extra = {})
    {
        std::printf ("%-56s %s %s\n", what.toRawUTF8(), ok ? "OK" : "FALLO", extra.toRawUTF8());
        fails += ok ? 0 : 1;
    };

    // ---- todas: valores de partida, finitas e baixo o teito, bloques, bypass, estado e mono
    for (int t = 0; t < enx::NUM_FERRAMENTAS; ++t)
    {
        const auto nome = U (enx::FERRAMENTAS[t].nome);
        {
            EnxProcessor p (t);
            juce::String mal;
            for (auto* prm : p.getParameters())
                if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                    if (std::abs (r->getValue() - r->getDefaultValue()) > 1e-6f) mal << r->getParameterID() << " ";
            check (mal.isEmpty() && p.getName() == nome, nome + U (": ao abrir, nos valores de partida"), mal);
        }
        {   // o mando ao máximo, cun son forte: finito e sen pasar de −1 dBFS
            EnxProcessor p (t);
            setParam (p, "mando", 100.0f);
            auto son = sonDeProba (2.0);
            son.applyGain (1.8f);
            const auto o = procesar (p, son, 2.0 + juce::jmin (3.0, p.getTailLengthSeconds()), 512);
            check (finito (o) && pico (o) <= 0.8913f, nome + U (": ao máximo, finito e baixo o teito"), juce::String::formatted ("(pico %.3f)", pico (o)));
        }
        {   // bloques de 512 e de 37: o mesmo, mostra a mostra
            auto render = [&] (int bloque)
            {
                EnxProcessor p (t);
                setParam (p, "mando", t == enx::FILTRO ? -55.0f : 65.0f);
                return procesar (p, sonDeProba (2.0), 2.5, bloque);
            };
            const auto x = render (512), y = render (37);
            // o sobremostrado de JUCE (Saturador e Chorus) leva a cero, ao final de cada bloque, os
            // estados por debaixo de 1e-8 (só en Intel): aí a diferenza pode ser dunha milmillonésima
            const bool sobremostra = t == enx::SATURACION || t == enx::CHORUS;
            const float d = diferenza (x, y);
            check (sobremostra ? d < 1e-5f : enx::cero (d), nome + U (": mesmo son con bloques de 512 e de 37"),
                   juce::String::formatted ("(%016llx; %.1e)", (unsigned long long) pegada (x), (double) d));
        }
        {   // bypass: o son tal cal, atrasado o que di a latencia
            EnxProcessor p (t);
            setParam (p, "mando", 80.0f); setParam (p, "bypass", 1.0f);
            const auto son = sonDeProba (1.0);
            const auto o = procesar (p, son, 1.0, 512);
            const int L = p.getLatencySamples();
            float d = 0;
            for (int c = 0; c < 2; ++c)
                for (int i = L; i < o.getNumSamples(); ++i) d = juce::jmax (d, std::abs (o.getSample (c, i) - son.getSample (c, i - L)));
            check (enx::cero (d), nome + U (": bypass, o son tal cal (coa latencia)"), "(" + juce::String (L) + " mostras)");
        }
        {   // estado
            EnxProcessor a (t);
            setParam (a, "mando", t == enx::FILTRO ? 33.0f : 41.0f); setParam (a, "selector", 2.0f);
            a.uiScale = 0.8f;
            juce::MemoryBlock mb; a.getStateInformation (mb);
            EnxProcessor b (t); b.setStateInformation (mb.getData(), (int) mb.getSize());
            const auto x = procesar (a, sonDeProba (1.0), 1.5, 512), y = procesar (b, sonDeProba (1.0), 1.5, 512);
            check (diferenza (x, y) < 1e-6f && b.selector() == 2 && std::abs (b.uiScale - 0.8f) < 1e-6f, nome + U (": estado gardado e recuperado"));
        }
        {   // mono → estéreo
            EnxProcessor p (t);
            juce::AudioProcessor::BusesLayout l;
            l.inputBuses.add (juce::AudioChannelSet::mono()); l.outputBuses.add (juce::AudioChannelSet::stereo());
            const bool ok = p.setBusesLayout (l);
            p.prepareToPlay (SR, 512);
            juce::AudioBuffer<float> buf (2, 512);
            bool fin = true;
            const auto son = sonDeProba (0.5);
            for (int pos = 0; pos + 512 <= son.getNumSamples(); pos += 512)
            {
                buf.clear(); buf.copyFrom (0, 0, son, 0, pos, 512);
                juce::MidiBuffer m; p.processBlock (buf, m);
                fin = fin && std::isfinite (buf.getMagnitude (0, 512)) && std::isfinite (buf.getMagnitude (1, 512));
            }
            check (ok && fin, nome + U (": mono a estéreo"));
        }
    }

    // ---- a 0, o son tal cal (as que non sobremostran)
    for (int t : { (int) enx::AMBIENTE, (int) enx::BRILLO, (int) enx::GRAVES, (int) enx::COMPRESOR, (int) enx::SIDECHAIN, (int) enx::FILTRO })
    {
        EnxProcessor p (t);
        setParam (p, "mando", 0.0f);
        const auto son = sonDeProba (1.0);
        check (enx::cero (diferenza (procesar (p, son, 1.0, 512), son)), U (enx::FERRAMENTAS[t].nome) + U (": a 0, o son tal cal"));
    }

    // ---- o que fai cada unha, medido
    {   // Ambiente: ao máximo, a cola segue despois do son; a sala é máis longa ca o cuarto
        auto cola = [&] (int sel)
        {
            EnxProcessor p (enx::AMBIENTE);
            setParam (p, "mando", 100.0f); setParam (p, "selector", (float) sel);
            juce::AudioBuffer<float> golpe = ruido (-12.0, 0.05);
            return rmsDb (procesar (p, golpe, 3.0, 512), 1.0, 1.5);
        };
        const double cuarto = cola (0), sala = cola (1);
        check (sala > -60.0 && sala > cuarto + 10.0, U ("Ambiente: a cola, máis longa na sala ca no cuarto"), juce::String::formatted ("(%.0f / %.0f dB)", cuarto, sala));
    }
    {   // Saturador: o volume queda parello (±3 dB) e aparecen harmónicos
        bool ok = true; juce::String que;
        for (int sel = 0; sel < 3; ++sel)
            for (float m : { 30.0f, 70.0f, 100.0f })
            {
                const double g = ganancia (enx::SATURACION, m, sel, 220.0, -18.0);
                ok = ok && std::abs (g) < 3.5;
                que << juce::String (g, 1) << " ";
            }
        // harmónicos: o terceiro (660 Hz) dun seno de 220 Hz, coa cinta ao 70 %
        EnxProcessor p (enx::SATURACION);
        setParam (p, "mando", 70.0f); setParam (p, "selector", 0.0f);
        const auto o = procesar (p, seno (220.0, -12.0, 1.0), 1.0, 512);
        double re = 0, im = 0;
        for (int i = (int) (0.5 * SR); i < (int) SR; ++i) { const double a = 2 * enx::PI * 660.0 * i / SR; re += o.getSample (0, i) * std::cos (a); im += o.getSample (0, i) * std::sin (a); }
        const double h3 = 20.0 * std::log10 (2.0 * std::sqrt (re * re + im * im) / (0.5 * SR)) - (rmsDb (o, 0.5, 1.0, 0) + 3.0103);
        check (ok && h3 > -40.0, U ("Saturador: volume parello e harmónicos"), "(" + que.trim() + U (" dB; 3.º harmónico ") + juce::String (juce::roundToInt (h3)) + " dB)");
    }
    {   // Brillo e Graves: suben onde din e non tocan o resto
        const double b15 = ganancia (enx::BRILLO, 100.0f, 1, 15000.0), b100 = ganancia (enx::BRILLO, 100.0f, 1, 100.0);
        check (b15 > 9.0 && b15 < 14.0 && std::abs (b100) < 0.5, U ("Brillo: +12 dB arriba, nada abaixo"), juce::String::formatted ("(%.1f / %.1f dB)", b15, b100));
        const double g50 = ganancia (enx::GRAVES, 100.0f, 1, 50.0), g3k = ganancia (enx::GRAVES, 100.0f, 1, 3000.0);
        check (g50 > 7.0 && g50 < 12.0 && std::abs (g3k) < 0.5, U ("Graves: +10 dB abaixo, nada arriba"), juce::String::formatted ("(%.1f / %.1f dB)", g50, g3k));
    }
    {   // Volume: un son baixo sobe 14 dB; un forte non pasa do teito
        const double baixo = ganancia (enx::VOLUME, 100.0f, 0, 440.0, -30.0);
        EnxProcessor p (enx::VOLUME);
        setParam (p, "mando", 100.0f);
        const float pk = pico (procesar (p, seno (440.0, -3.0, 1.0), 1.0, 512));
        check (std::abs (baixo - 14.0) < 0.5 && pk <= 0.8913f && pk > 0.85f, U ("Volume: +14 dB e o teito a −1 dBFS"), juce::String::formatted ("(%.1f dB; pico %.4f)", baixo, pk));
    }
    {   // Compresor: un son forte baixa; un baixo queda (coa compensación)
        EnxProcessor p (enx::COMPRESOR);
        setParam (p, "mando", 100.0f);
        procesar (p, seno (440.0, -6.0, 1.0), 1.0, 512);
        const float gr = p.reducion.load();
        const double forte = ganancia (enx::COMPRESOR, 100.0f, 1, 440.0, -6.0), baixo = ganancia (enx::COMPRESOR, 100.0f, 1, 440.0, -50.0);
        check (gr < -12.0f && forte < baixo - 10.0, U ("Compresor: aperta os fortes"), U ("(redución ") + juce::String (gr, 1) + juce::String::formatted (" dB; %.1f / %.1f dB)", forte, baixo));
    }
    {   // Sidechain: a 120 BPM en negras, baixa 40 dB en cada tempo e volve; desprazar move a baixada
        auto envolvente = [&] (float desp)
        {
            EnxProcessor p (enx::SIDECHAIN);
            setParam (p, "mando", 100.0f); setParam (p, "selector", 1.0f); setParam (p, "desprazar", desp);
            return procesar (p, seno (1000.0, -6.0, 2.0), 2.0, 512);
        };
        auto minimo = [&] (const juce::AudioBuffer<float>& o, double t0, double t1, int& onde)
        {
            double mn = 1e9;
            for (int k = (int) (t0 * SR); k < (int) (t1 * SR); k += 48)
            {
                const double db = rmsDb (o, k / SR, (k + 48) / SR);
                if (db < mn) { mn = db; onde = k; }
            }
            return mn;
        };
        const auto a = envolvente (0.0f), b = envolvente (40.0f);
        int oa = 0, ob = 0;
        const double fondo = minimo (a, 0.45, 0.95, oa) - (-6.0 - 3.0103);
        minimo (b, 0.45, 0.95, ob);
        const double volta = rmsDb (a, 0.95, 0.99) - (-6.0 - 3.0103);
        const double msA = (oa - 0.5 * SR) * 1000.0 / SR, msB = (ob - 0.5 * SR) * 1000.0 / SR;
        check (fondo < -30.0 && volta > -2.0 && std::abs (msA) < 6.0 && std::abs (msB - msA - 40.0) < 3.0, U ("Sidechain: baixa en cada tempo e volve; desprazar"),
               juce::String::formatted ("(%.0f dB; ao final %.1f dB; %.0f / %.0f ms)", fondo, volta, msA, msB));
    }
    {   // Filtro: á esquerda pecha (o Juno), á dereita abre por arriba
        const double lpAlto = ganancia (enx::FILTRO, -100.0f, 1, 5000.0), lpBaixo = ganancia (enx::FILTRO, -100.0f, 1, 40.0);
        const double hpBaixo = ganancia (enx::FILTRO, 100.0f, 1, 200.0), hpAlto = ganancia (enx::FILTRO, 100.0f, 1, 15000.0);
        check (lpAlto < -40.0 && std::abs (lpBaixo) < 4.0 && hpBaixo < -30.0 && std::abs (hpAlto) < 3.0, U ("Filtro: paso baixo á esquerda, paso alto á dereita"),
               juce::String::formatted ("(PB %.0f / %.1f dB; PA %.0f / %.1f dB)", lpAlto, lpBaixo, hpBaixo, hpAlto));
    }
    {   // Chorus: ao máximo, as dúas canles distintas (os BBD en contrafase); I+II, iguais
        auto lado = [&] (int sel)
        {
            EnxProcessor p (enx::CHORUS);
            setParam (p, "mando", 100.0f); setParam (p, "selector", (float) sel);
            const auto o = procesar (p, seno (440.0, -12.0, 2.0), 2.0, 512);
            double d = 0, s = 0;
            for (int i = (int) (0.5 * SR); i < o.getNumSamples(); ++i) { const double x = o.getSample (0, i) - o.getSample (1, i), y = o.getSample (0, i) + o.getSample (1, i); d += x * x; s += y * y; }
            return 10.0 * std::log10 ((d + 1e-30) / (s + 1e-30));
        };
        const double i = lado (0), ii = lado (2);
        check (i > -20.0 && ii < -60.0, U ("Chorus: estéreo nos modos I e II, mono no I+II"), juce::String::formatted ("(lados %.0f / %.0f dB)", i, ii));
    }
    return fails;
}
