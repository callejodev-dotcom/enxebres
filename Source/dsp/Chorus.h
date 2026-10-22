// Chorus.h — O chorus do Juno-106 dun só mando: os dous BBD MN3009 de Barullo Xuno (xuno::Bbd, co
// seu reloxo de verdade e os seus filtros), movidos en contrafase por un LFO triangular, a catro
// veces a frecuencia de mostraxe (coma no Xuno, que vai a 190 kHz). Medidas dun Juno (KR-106):
//   I     0,514 Hz, ±2,13 ms arredor de 3,3 ms
//   II    0,863 Hz, ±1,71 ms
//   I+II  9,75 Hz, ±0,35 ms e as dúas liñas xuntas: un vibrato rápido (coma no Juno-60)
// O mando vai do son seco ao chorus enteiro do Juno (directo 0,575 e BBD 0,838) e, no último terzo,
// abre un pouco máis o estéreo. A entrada estéreo segue estéreo no directo; os BBD levan a suma.
#pragma once
#include "Comun.h"
#include <juce_dsp/juce_dsp.h>
#include "barullo-xuno/Source/xuno/Board.h"

namespace enx {

class Chorus : public Ferramenta
{
public:
    static constexpr double CENTRO = 3.30e-3;
    static constexpr double VEL[3] = { 0.514, 0.863, 9.75 };
    static constexpr double FONDO[3] = { 2.13e-3, 1.71e-3, 0.35e-3 };
    static double directo (double m) { return 1.0 - std::clamp (m, 0.0, 1.0) * (1.0 - 0.863 / 1.5); }
    static double bbd (double m) { return std::clamp (m, 0.0, 1.0) * 1.257 / 1.5; }
    static double anchura (double m) { return 1.0 + 0.45 * std::max (0.0, std::clamp (m, 0.0, 1.0) - 0.66) / 0.34; }

    void preparar (double f, int bloque) override
    {
        fs = f;
        // os BBD queren polo menos uns 90 kHz (os seus filtros chegan a 20 kHz): ×4 a 44,1/48 kHz,
        // máis veces por debaixo e menos por riba
        const int etapas = fs < 12000 ? 4 : fs < 24000 ? 3 : fs < 88000 ? 2 : 1;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, etapas, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) std::max (1, bloque));
        f4 = fs * (double) (1 << etapas);
        a.prepare (f4); b.prepare (f4);
        mS.preparar (f4, 40.0, m);
        limpar();
    }
    void limpar() override { if (os) os->reset(); a.reset(); b.reset(); lfo = 0; }
    void axustar (const Contexto& c) override { m = c.mando; sel = std::clamp (c.selector, 0, 2); mS.obx = m; }
    void procesar (float* l, float* r, int n) override
    {
        float* ch[2] = { l, r };
        juce::dsp::AudioBlock<float> bloque (ch, 2, (size_t) n);
        auto alto = os->processSamplesUp (bloque);
        float* L = alto.getChannelPointer (0);
        float* R = alto.getChannelPointer (1);
        const int N = (int) alto.getNumSamples();
        const double vel = VEL[sel], fondo = FONDO[sel];
        for (int i = 0; i < N; ++i)
        {
            lfo += vel / f4;
            if (lfo >= 1.0) lfo -= 1.0;
            const double tri = 1.0 - 4.0 * std::abs (lfo - 0.5);
            const double d0 = CENTRO + fondo * tri, d1 = sel == 2 ? d0 : CENTRO - fondo * tri;
            const double x = 0.5 * ((double) L[i] + (double) R[i]);
            const double w0 = a.run (x, 128.0 / d0 / f4), w1 = b.run (x, 128.0 / d1 / f4);
            const double v = mS.paso();
            const double gd = directo (v), gw = bbd (v), an = anchura (v);
            double yl = L[i] * gd + w0 * gw, yr = R[i] * gd + w1 * gw;
            const double mid = 0.5 * (yl + yr), side = 0.5 * (yl - yr) * an;
            L[i] = (float) (mid + side); R[i] = (float) (mid - side);
        }
        os->processSamplesDown (bloque);
    }
    int latencia() const override { return os ? (int) std::lround (os->getLatencyInSamples()) : 0; }
    double fase() const override { return lfo; }

private:
    double fs = 48000, f4 = 192000, m = 0, lfo = 0;
    int sel = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    xuno::Bbd a, b;
    Suave mS;
};

} // namespace enx
