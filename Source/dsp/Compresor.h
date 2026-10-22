// Compresor.h — Un compresor dun só mando: o limiar baixa (ata −30 dB) e a proporción sobe (de 1,5:1
// a 8:1) xuntos, cun xeonllo de 6 dB e o volume compensado. As dúas canles van ligadas. A 0, o son tal cal.
#pragma once
#include "Comun.h"

namespace enx {

class Compresor : public Ferramenta
{
public:
    static double limiar (double m) { return -30.0 * std::clamp (m, 0.0, 1.0); }
    static double proporcion (double m) { return 1.5 + 6.5 * std::clamp (m, 0.0, 1.0); }
    static double compensacion (double m) { return -limiar (m) * (1.0 - 1.0 / proporcion (m)) * 0.55; }
    // a curva (dB → dB), co xeonllo
    static double curva (double x, double m)
    {
        const double t = limiar (m), r = proporcion (m), w = 6.0;
        if (x < t - w / 2) return x;
        if (x > t + w / 2) return t + (x - t) / r;
        const double z = x - t + w / 2;
        return x + (1.0 / r - 1.0) * z * z / (2.0 * w);
    }

    void preparar (double f, int) override { fs = f; mS.preparar (fs, 40.0, m); velocidades(); limpar(); }
    void limpar() override { grDb = 0; gr = 0; }
    void axustar (const Contexto& c) override
    {
        m = c.mando; mS.obx = m;
        if (c.selector != sel) { sel = std::clamp (c.selector, 0, 2); velocidades(); }
    }
    void procesar (float* l, float* r, int n) override
    {
        double maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            const double v = mS.paso();
            if (cero (v) && mS.quedo() && cero (grDb)) continue;
            const double x = gananciaADb (std::max (std::abs (l[i]), std::abs (r[i])));
            const double obx = curva (x, v) - x;   // ≤ 0
            grDb += (obx - grDb) * (obx < grDb ? at : so);
            if (std::abs (grDb) < 1e-7 && cero (v)) grDb = 0.0;
            const double g = dbAGanancia (grDb + compensacion (v));
            l[i] = (float) (l[i] * g); r[i] = (float) (r[i] * g);
            maxGr = std::min (maxGr, grDb);
        }
        gr = maxGr;
    }
    double reducion() const override { return gr; }

private:
    void velocidades()
    {
        const double a = sel == 0 ? 30.0 : sel == 2 ? 1.0 : 10.0, s = sel == 0 ? 400.0 : sel == 2 ? 60.0 : 150.0;
        at = 1.0 - std::exp (-1.0 / (a * 0.001 * fs));
        so = 1.0 - std::exp (-1.0 / (s * 0.001 * fs));
    }

    double fs = 48000, m = 0, grDb = 0, gr = 0, at = 0.01, so = 0.001;
    int sel = 1;
    Suave mS;
};

} // namespace enx
