// Brillo.h — Un realce de agudos dun só mando: un andel desde a frecuencia escollida (ata +12 dB)
// e un chisco de harmónicos xerados só nos agudos (un paso alto, unha curva suave e de volta),
// que dan brillo sen soar a ecualizador. A 0, o son tal cal.
#pragma once
#include "Comun.h"

namespace enx {

class Brillo : public Ferramenta
{
public:
    static constexpr double FREC[3] = { 8000.0, 12000.0, 16000.0 };
    static double realce (double m) { return 12.0 * std::pow (std::clamp (m, 0.0, 1.0), 1.1); }   // dB
    static double harmonicos (double m) { return 0.22 * std::clamp (m, 0.0, 1.0); }

    void preparar (double f, int) override { fs = f; mS.preparar (fs, 40.0, m); cont = 0; recalcular(); limpar(); }
    void limpar() override { for (int c = 0; c < 2; ++c) { andel[c].reset(); hp[c].reset(); } }
    void axustar (const Contexto& c) override { m = c.mando; sel = std::clamp (c.selector, 0, 2); mS.obx = m; }
    void procesar (float* l, float* r, int n) override
    {
        for (int i = 0; i < n; ++i)
        {
            if (cont-- <= 0) { cont = 31; recalcular(); }   // os coeficientes, cada 32 mostras (igual con calquera bloque)
            const double v = mS.paso();
            if (cero (v) && mS.quedo()) continue;
            float* xs[2] = { l + i, r + i };
            for (int c = 0; c < 2; ++c)
            {
                const double x = *xs[c];
                const double h = hp[c].run (x);
                const double ex = std::tanh (3.0 * h) / 3.0 - h;   // só o que a curva engade: os harmónicos
                *xs[c] = (float) (andel[c].run (x) + ex * kh);
            }
        }
    }
    // para a pantalla
    double db (double f) const { return andel[0].db (fs, f); }

private:
    void recalcular()
    {
        const double v = mS.v;
        const double fc = std::min (FREC[sel], fs * 0.42);
        for (int c = 0; c < 2; ++c)
        {
            const double z1 = andel[c].z1, z2 = andel[c].z2;
            andel[c].andelAgudos (fs, fc, realce (v), 0.9);
            andel[c].z1 = z1; andel[c].z2 = z2;
            const double h1 = hp[c].z1, h2 = hp[c].z2;
            hp[c].pasoAlto (fs, fc * 0.5, 0.7071);
            hp[c].z1 = h1; hp[c].z2 = h2;
        }
        kh = harmonicos (v) * 6.0;
    }

    double fs = 48000, m = 0, kh = 0;
    int sel = 1, cont = 0;
    Suave mS;
    Biquad andel[2], hp[2];
};

} // namespace enx
