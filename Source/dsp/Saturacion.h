// Saturacion.h — Unha saturación dun só mando, a catro veces a frecuencia de mostraxe (os
// harmónicos que pasarían de Nyquist non volven como ruído). O mando sobe a ganancia antes da curva
// e báixaa despois, para que o volume quede parello.
//   Cinta    tanh, con realce de agudos antes e o contrario despois (satura máis nos agudos, coma a cinta)
//   Válvula  tanh asimétrica (harmónicos pares) e un paso alto que quita o continuo
//   Fuzz     máis ganancia, unha curva moi dura e un filtro de ton escuro
#pragma once
#include "Comun.h"
#include <juce_dsp/juce_dsp.h>

namespace enx {

class Saturacion : public Ferramenta
{
public:
    static constexpr int FACTOR = 4;

    // a ganancia antes da curva e a compensación despois (dB), para o mando e o carácter
    static double ganancia (double m, int sel) { m = std::clamp (m, 0.0, 1.0); return (sel == 2 ? 12.0 + 30.0 * m : 30.0 * m); }
    // a compensación: a que deixa un seno a −18 dBFS (RMS) co mesmo volume despois da curva
    static double compensacion (double m, int sel)
    {
        const double g = dbAGanancia (ganancia (m, sel)), a = 0.178;
        double s = 0, s2 = 0;
        for (int k = 0; k < 64; ++k)
        {
            const double y = curva (a * g * std::sin (2 * PI * (k + 0.5) / 64.0), sel);
            s += y; s2 += y * y;
        }
        const double media = s / 64.0, rms = std::sqrt (std::max (1e-12, s2 / 64.0 - media * media));
        return -gananciaADb (rms / (a / std::sqrt (2.0)));
    }
    // a curva (para a pantalla tamén): x xa coa ganancia
    static double curva (double x, int sel)
    {
        if (sel == 1) { constexpr double b = 0.25; const double t = std::tanh (b); return (std::tanh (x + b) - t) / (1.0 - t * t); }
        if (sel == 2) { const double a = std::abs (x); return x / std::pow (1.0 + a * a * a * a * a * a, 1.0 / 6.0); }
        return std::tanh (x);
    }

    void preparar (double f, int bloque) override
    {
        fs = f;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) std::max (1, bloque));
        const double f4 = fs * FACTOR;
        for (int c = 0; c < 2; ++c)
        {
            pre[c].andelAgudos (f4, 2500.0, 5.0); post[c].andelAgudos (f4, 2500.0, -5.0);
            tono[c].pasoBaixo (f4, 5200.0, 0.6); dc[c].pasoAlto (f4, 12.0, 0.7071);
        }
        gIn.preparar (f4, 30.0, dbAGanancia (ganancia (m, sel)));
        gOut.preparar (f4, 30.0, dbAGanancia (compensacion (m, sel)));
        limpar();
    }
    void limpar() override
    {
        if (os) os->reset();
        for (int c = 0; c < 2; ++c) { pre[c].reset(); post[c].reset(); tono[c].reset(); dc[c].reset(); }
    }
    void axustar (const Contexto& c) override
    {
        if (exacto (c.mando, m) && c.selector == sel) return;
        sel = std::clamp (c.selector, 0, 2);
        m = c.mando;
        gIn.obx = dbAGanancia (ganancia (m, sel));
        gOut.obx = dbAGanancia (compensacion (m, sel));
    }
    void procesar (float* l, float* r, int n) override
    {
        float* ch[2] = { l, r };
        juce::dsp::AudioBlock<float> bloque (ch, 2, (size_t) n);
        auto alto = os->processSamplesUp (bloque);
        float* L = alto.getChannelPointer (0);
        float* R = alto.getChannelPointer (1);
        const int N = (int) alto.getNumSamples();
        for (int i = 0; i < N; ++i)
        {
            const double gi = gIn.paso(), go = gOut.paso();
            float* xs[2] = { L + i, R + i };
            for (int c = 0; c < 2; ++c)
            {
                double x = *xs[c];
                if (sel == 0) x = post[c].run (curva (pre[c].run (x) * gi, 0));
                else if (sel == 1) x = dc[c].run (curva (x * gi, 1));
                else x = tono[c].run (dc[c].run (curva (x * gi, 2)));
                *xs[c] = (float) (x * go);
            }
        }
        os->processSamplesDown (bloque);
    }
    int latencia() const override { return os ? (int) std::lround (os->getLatencyInSamples()) : 0; }

private:
    double fs = 48000, m = -1;   // (−1: aínda sen axustar)
    int sel = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    Biquad pre[2], post[2], tono[2], dc[2];
    Suave gIn, gOut;
};

} // namespace enx
