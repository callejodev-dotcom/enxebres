// Sidechain.h — O bombeo dun sidechain sen sidechain: en cada tempo (1/2, 1/4 ou 1/8, á par do DAW)
// o volume baixa en 4 ms e volve aos poucos, coma se un bombo apertase un compresor. O mando di
// canto baixa (ata −40 dB); «desprazar» move a baixada respecto ao tempo (−50 a +50 ms). Sen o
// DAW a tocar, vai soa ao tempo do DAW (ou a 120). A 0, o son tal cal.
#pragma once
#include "Comun.h"

namespace enx {

class Sidechain : public Ferramenta
{
public:
    static constexpr double NEGRAS[3] = { 2.0, 1.0, 0.5 };
    static constexpr double CAIDA = 0.004;   // s
    static double fondura (double m) { return 40.0 * std::pow (std::clamp (m, 0.0, 1.0), 1.3); }   // dB
    // a forma: 0 (abaixo) → 1 (arriba), segundo o tempo desde o golpe (s) e o que dura cada ciclo
    static double forma (double t, double ciclo)
    {
        if (t < CAIDA) return 1.0 - t / CAIDA;
        return 1.0 - std::exp (-(t - CAIDA) / (0.2 * ciclo));
    }

    void preparar (double f, int) override { fs = f; mS.preparar (fs, 30.0, m); limpar(); }
    void limpar() override { libre = 0; ph = 0; gr = 0; }
    void axustar (const Contexto& c) override
    {
        m = c.mando; mS.obx = m;
        sel = std::clamp (c.selector, 0, 2);
        bpm = std::clamp (c.bpm, 20.0, 400.0);
        desprazar = std::clamp (c.extra, -50.0, 50.0);
        tocando = c.tocando;
        negrasDAW = c.negras;
    }
    void procesar (float* l, float* r, int n) override
    {
        const double porMostra = bpm / 60.0 / fs;   // negras por mostra
        const double cicloN = NEGRAS[sel], cicloS = cicloN * 60.0 / bpm;
        const double off = desprazar * 0.001 * bpm / 60.0;   // en negras
        double peor = 0;
        for (int i = 0; i < n; ++i)
        {
            const double b = tocando ? negrasDAW + i * porMostra : libre;
            if (! tocando) libre += porMostra;
            double fz = (b - off) / cicloN;
            fz -= std::floor (fz);
            ph = fz;
            const double v = mS.paso();
            if (cero (v) && mS.quedo()) continue;
            const double db = -fondura (v) * (1.0 - forma (fz * cicloS, cicloS));
            const double g = dbAGanancia (db);
            l[i] = (float) (l[i] * g); r[i] = (float) (r[i] * g);
            peor = std::min (peor, db);
        }
        if (tocando) negrasDAW += n * porMostra;
        else if (libre > 1e6) libre = std::fmod (libre, 4.0);
        gr = peor;
    }
    double reducion() const override { return gr; }
    double fase() const override { return ph; }

private:
    double fs = 48000, m = 0, bpm = 120, desprazar = 0, negrasDAW = 0, libre = 0, ph = 0, gr = 0;
    int sel = 1;
    bool tocando = false;
    Suave mS;
};

} // namespace enx
