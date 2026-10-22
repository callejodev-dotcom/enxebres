// Graves.h — Un realce de graves dun só mando: un andel na frecuencia escollida (ata +10 dB) e os
// harmónicos dos graves (o dobre e o triplo), para que o peso se oia tamén nun móbil. A 0, o son tal cal.
#pragma once
#include "Comun.h"

namespace enx {

class Graves : public Ferramenta
{
public:
    static constexpr double FREC[3] = { 40.0, 60.0, 100.0 };
    static double realce (double m) { return 10.0 * std::pow (std::clamp (m, 0.0, 1.0), 1.1); }   // dB
    static double harmonicos (double m) { return 0.6 * std::clamp (m, 0.0, 1.0); }

    void preparar (double f, int) override { fs = f; mS.preparar (fs, 40.0, m); cont = 0; recalcular(); limpar(); }
    void limpar() override { for (int c = 0; c < 2; ++c) { andel[c].reset(); lp[c].reset(); bp[c].reset(); } }
    void axustar (const Contexto& c) override { m = c.mando; sel = std::clamp (c.selector, 0, 2); mS.obx = m; }
    void procesar (float* l, float* r, int n) override
    {
        for (int i = 0; i < n; ++i)
        {
            if (cont-- <= 0) { cont = 31; recalcular(); }
            const double v = mS.paso();
            if (cero (v) && mS.quedo()) continue;
            float* xs[2] = { l + i, r + i };
            for (int c = 0; c < 2; ++c)
            {
                const double x = *xs[c];
                // os graves, rectificados a media onda suave e saturados: aparecen o dobre e o triplo
                const double g = lp[c].run (x) * 4.0;
                const double h = std::tanh (g + 0.35 * g * g) / 4.0;
                *xs[c] = (float) (andel[c].run (x) + bp[c].run (h) * kh);
            }
        }
    }
    double db (double f) const { return andel[0].db (fs, f); }

private:
    void recalcular()
    {
        const double v = mS.v, f0 = FREC[sel];
        for (int c = 0; c < 2; ++c)
        {
            andel[c].andelGraves (fs, f0 * 1.6, realce (v), 0.9);
            lp[c].pasoBaixo (fs, f0 * 1.4, 0.7071);
            bp[c].pasoBanda (fs, f0 * 2.6, 0.9);
        }
        kh = harmonicos (v);
    }

    double fs = 48000, m = 0, kh = 0;
    int sel = 1, cont = 0;
    Suave mS;
    Biquad andel[2], lp[2], bp[2];
};

} // namespace enx
