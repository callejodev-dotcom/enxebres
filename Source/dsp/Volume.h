// Volume.h — Máis forte sen pasar do teito: o mando sobe a ganancia (ata +14 dB) e un limitador
// que mira 1,5 ms adiante (latencia) baixa os picos xusto a tempo para que nada pase de −1 dBFS.
//   Limpo   soltura de 120 ms, sen máis
//   Cálido  unha saturación suave antes do limitador (os picos redondéanse) e soltura de 200 ms
//   Duro    soltura de 40 ms e un recorte suave despois: máis forte, con máis carácter
#pragma once
#include "Comun.h"

namespace enx {

class Volume : public Ferramenta
{
public:
    static constexpr double TEITO = 0.8913;   // −1 dBFS
    static double ganancia (double m) { return 14.0 * std::clamp (m, 0.0, 1.0); }   // dB

    void preparar (double f, int) override
    {
        fs = f;
        L = std::max (1, (int) std::lround (0.0015 * fs));
        for (auto& d : atraso) d.assign ((size_t) L + 1, 0.0f);
        req.assign ((size_t) L + 1, 1.0);
        caixa.assign ((size_t) L + 1, 1.0);
        gS.preparar (fs, 40.0, dbAGanancia (ganancia (m)));
        limpar();
    }
    void limpar() override
    {
        for (auto& d : atraso) std::fill (d.begin(), d.end(), 0.0f);
        std::fill (req.begin(), req.end(), 1.0);
        std::fill (caixa.begin(), caixa.end(), 1.0);
        suma = (double) caixa.size(); w = 0; env = 1.0; gr = 0;
    }
    void axustar (const Contexto& c) override
    {
        m = c.mando; sel = std::clamp (c.selector, 0, 2);
        gS.obx = dbAGanancia (ganancia (m));
        soltura = 1.0 - std::exp (-1.0 / ((sel == 1 ? 0.200 : sel == 2 ? 0.040 : 0.120) * fs));
    }
    void procesar (float* l, float* r, int n) override
    {
        const int N = L + 1;
        double minGr = 1.0;
        for (int i = 0; i < n; ++i)
        {
            const double g = gS.paso();
            double xl = l[i] * g, xr = r[i] * g;
            if (sel == 1) { xl = calido (xl); xr = calido (xr); }
            // o que fai falta para este instante, e o mínimo dos últimos L+1 (o pico que vén)
            const double pk = std::max (std::abs (xl), std::abs (xr));
            req[(size_t) w] = pk > TEITO ? TEITO / pk : 1.0;
            double mn = 1.0;
            for (int k = 0; k < N; ++k) mn = std::min (mn, req[(size_t) k]);
            // soltura cara arriba; para abaixo, de golpe (a caixa suaviza)
            env = mn < env ? mn : env + (mn - env) * soltura;
            // media móbil de L+1: chega ao mínimo xusto cando o pico sae do atraso
            suma += env - caixa[(size_t) w];
            caixa[(size_t) w] = env;
            const double gl = std::min (1.0, suma / N);
            // o atraso
            const int ri = (w + 1) % N;
            const double dl = atraso[0][(size_t) ri], dr = atraso[1][(size_t) ri];
            atraso[0][(size_t) w] = (float) xl; atraso[1][(size_t) w] = (float) xr;
            w = (w + 1) % N;
            double yl = dl * gl, yr = dr * gl;
            if (sel == 2) { yl = duro (yl); yr = duro (yr); }
            // o teito, por se acaso (a media pode quedar un chisco por riba nun pico moi illado)
            yl = std::clamp (yl, -TEITO, TEITO); yr = std::clamp (yr, -TEITO, TEITO);
            l[i] = (float) yl; r[i] = (float) yr;
            minGr = std::min (minGr, gl);
        }
        gr = gananciaADb (minGr);
    }
    int latencia() const override { return L; }
    double reducion() const override { return gr; }

private:
    static double calido (double x) { const double a = std::abs (x); return a < 0.6 ? x : std::copysign (0.6 + 0.4 * std::tanh ((a - 0.6) / 0.4), x); }
    static double duro (double x) { const double a = std::abs (x), t = TEITO * 0.9; return a < t ? x : std::copysign (t + (TEITO - t) * std::tanh ((a - t) / (TEITO - t)), x); }

    double fs = 48000, m = 0, env = 1, suma = 0, soltura = 0.0002, gr = 0;
    int sel = 0, L = 72, w = 0;
    std::array<std::vector<float>, 2> atraso;
    std::vector<double> req, caixa;
    Suave gS;
};

} // namespace enx
