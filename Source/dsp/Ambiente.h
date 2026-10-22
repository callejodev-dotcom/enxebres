// Ambiente.h — Unha reverb dun só mando, coa de Barullo Verb (verb::Espazo, o seu repo nunha
// versión fixada). O mando sobe xuntos o tamaño, a mestura e o predelay, e escurece un pouco a cola.
#pragma once
#include "Comun.h"
#include "barulloverb/Source/Espazo.h"

namespace enx {

class Ambiente : public Ferramenta
{
public:
    // o que fai o mando (tamén para a pantalla)
    static verb::Axustes axustes (double m, int selector)
    {
        m = std::clamp (m, 0.0, 1.0);
        verb::Axustes a;
        a.tipo = selector == 0 ? verb::CUARTO : selector == 2 ? verb::PLACA : verb::SALA;
        const double c0 = selector == 0 ? 0.3 : selector == 2 ? 0.5 : 0.6, c1 = selector == 0 ? 1.4 : selector == 2 ? 3.0 : 3.4;
        a.caida = c0 + (c1 - c0) * m * m;
        a.predelay = (selector == 0 ? 2.0 : 6.0) + (selector == 2 ? 24.0 : 18.0) * m;
        a.brillo = 0.62 - 0.18 * m;
        a.difusion = 0.78;
        a.modulacion = 0.3;
        a.graves = 1.0 + 0.15 * m;
        a.reflexions = selector == 0 ? 0.7 : selector == 2 ? 0.0 : 0.45;
        a.anchura = 1.0 + 0.2 * m;
        a.corteGraves = 170.0 - 80.0 * m;
        a.corteAgudos = 13000.0;
        return a;
    }
    // a parte de reverb (0..1, con potencia constante): a 0, o son tal cal
    static double mestura (double m) { return 0.44 * std::pow (std::clamp (m, 0.0, 1.0), 0.8); }

    void preparar (double f, int) override
    {
        fs = f;
        espazo.preparar (fs);
        mest.preparar (fs, 40.0, mestura (m));
    }
    void limpar() override { espazo.limpar(); }
    void axustar (const Contexto& c) override
    {
        m = c.mando; sel = c.selector;
        espazo.axustar (axustes (m, sel));
        mest.obx = mestura (m);
    }
    void procesar (float* l, float* r, int n) override
    {
        for (int i = 0; i < n; ++i)
        {
            const double x = mest.paso();
            if (cero (x) && mest.quedo()) continue;   // a 0: o son tal cal
            float wl = l[i], wr = r[i];
            espazo.procesar (wl, wr);
            const double gs = std::cos (x * PI / 2), gm = std::sin (x * PI / 2);
            l[i] = (float) (l[i] * gs + wl * gm);
            r[i] = (float) (r[i] * gs + wr * gm);
        }
    }
    double cola() const override { return std::min (20.0, axustes (m, sel).caida * 1.5 + 0.2); }

private:
    double fs = 48000, m = 0;
    int sel = 1;
    verb::Espazo espazo;
    Suave mest;
};

} // namespace enx
