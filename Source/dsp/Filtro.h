// Filtro.h — Un filtro dun só mando, coma os das mesas de DJ: no medio, o son tal cal; á esquerda,
// o filtro do Juno-106 (o IR3109 de Barullo Xuno, xuno::Vcf: catro OTA e a súa resonancia) vai
// pechando ata 90 Hz; á dereita, un paso alto de 12 dB/oitava sobe ata 9 kHz. A resonancia medra
// un pouco no medio do percorrido, que é onde máis se oe o barrido.
#pragma once
#include "Comun.h"
#include "barullo-xuno/Source/xuno/Voice.h"

namespace enx {

class Filtro : public Ferramenta
{
public:
    // o corte (Hz) para o mando (−1..1): negativo, o paso baixo; positivo, o paso alto
    static double corte (double m)
    {
        const double a = std::pow (std::min (1.0, std::abs (m)), 0.9);
        return m < 0 ? 20000.0 * std::pow (90.0 / 20000.0, a) : 20.0 * std::pow (9000.0 / 20.0, a);
    }
    static double bulto (double m) { return 0.65 + 0.35 * std::sin (PI * std::min (1.0, std::abs (m))); }
    static double resonanciaJuno (double m, int sel) { return (sel == 0 ? 0.8 : sel == 2 ? 3.5 : 2.2) * bulto (m); }
    static double qPasoAlto (double m, int sel) { return (sel == 0 ? 0.75 : sel == 2 ? 3.2 : 1.6) * bulto (m); }
    static constexpr double FUNDIDO = 0.05;   // de 0 a 5 %, o filtro entra aos poucos

    void preparar (double f, int) override { fs = f; mS.preparar (fs, 30.0, m); limpar(); }
    void limpar() override { for (auto& v : vcf) v.reset(); s1[0] = s1[1] = s2[0] = s2[1] = 0; }
    void axustar (const Contexto& c) override { m = std::clamp (c.mando, -1.0, 1.0); sel = std::clamp (c.selector, 0, 2); mS.obx = m; }
    void procesar (float* l, float* r, int n) override
    {
        for (int i = 0; i < n; ++i)
        {
            const double v = mS.paso();
            if (cero (v) && mS.quedo()) continue;   // no medio: o son tal cal
            const double fc = std::min (corte (v), fs * 0.42);
            const double g = std::tan (PI * fc / fs);
            const double wet = std::min (1.0, std::abs (v) / FUNDIDO);
            float* xs[2] = { l + i, r + i };
            if (v < 0)
            {
                const double k = resonanciaJuno (v, sel), comp = 1.0 + 0.5 * k;
                for (int c = 0; c < 2; ++c)
                {
                    vcf[c].g = g; vcf[c].k = k;
                    const double x = *xs[c];
                    *xs[c] = (float) (x * (1.0 - wet) + vcf[c].process (x) * comp * wet);
                }
            }
            else
            {
                // SVF de Zavalishin (TPT): estable con calquera corte e resonancia
                const double R2 = 1.0 / qPasoAlto (v, sel), d = 1.0 / (1.0 + R2 * g + g * g);
                for (int c = 0; c < 2; ++c)
                {
                    const double x = *xs[c];
                    const double hp = (x - (R2 + g) * s1[c] - s2[c]) * d;
                    const double bp = g * hp + s1[c];
                    s1[c] = g * hp + bp;
                    const double lp = g * bp + s2[c];
                    s2[c] = g * bp + lp;
                    *xs[c] = (float) (x * (1.0 - wet) + hp * wet);
                }
            }
            fc_ = v < 0 ? -fc : fc;
        }
    }
    double fase() const override { return fc_; }   // o corte de agora (negativo: paso baixo)

private:
    double fs = 48000, m = 0, fc_ = 0;
    int sel = 1;
    Suave mS;
    xuno::Vcf vcf[2];
    double s1[2] {}, s2[2] {};
};

} // namespace enx
