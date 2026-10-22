// Comun.h — As pezas que comparten as ferramentas de Barullo Enxebres, sen JUCE: filtros
// biquad (RBJ), un suavizado, un seguidor de envolvente e a interface de cada ferramenta.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

namespace enx {

constexpr double PI = 3.14159265358979323846;

// comparacións exactas (adrede: «a 0, o son tal cal» quere dicir 0 de verdade)
inline bool exacto (double a, double b) { return std::equal_to<double>() (a, b); }
inline bool cero (double x) { return exacto (x, 0.0); }

inline double dbAGanancia (double db) { return std::pow (10.0, db / 20.0); }
inline double gananciaADb (double g) { return 20.0 * std::log10 (std::max (g, 1e-9)); }

// biquad (Audio EQ Cookbook de R. Bristow-Johnson)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    static double w0 (double fs, double f) { return 2 * PI * std::clamp (f, 5.0, fs * 0.45) / fs; }

    void pasoBaixo (double fs, double f, double q = 0.7071)
    { const double w = w0 (fs, f), c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
    void pasoAlto (double fs, double f, double q = 0.7071)
    { const double w = w0 (fs, f), c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
    void pasoBanda (double fs, double f, double q)
    { const double w = w0 (fs, f), c = std::cos (w), al = std::sin (w) / (2 * q); set (al, 0, -al, 1 + al, -2 * c, 1 - al); }
    void andelAgudos (double fs, double f, double db, double s = 0.8)   // high shelf
    {
        const double A = std::pow (10.0, db / 40), w = w0 (fs, f), c = std::cos (w), sn = std::sin (w);
        const double al = sn / 2 * std::sqrt ((A + 1 / A) * (1 / s - 1) + 2), r = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + r), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - r),
             (A + 1) - (A - 1) * c + r, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - r);
    }
    void andelGraves (double fs, double f, double db, double s = 0.8)   // low shelf
    {
        const double A = std::pow (10.0, db / 40), w = w0 (fs, f), c = std::cos (w), sn = std::sin (w);
        const double al = sn / 2 * std::sqrt ((A + 1 / A) * (1 / s - 1) + 2), r = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + r), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - r),
             (A + 1) + (A - 1) * c + r, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - r);
    }
    // a ganancia en dB a unha frecuencia (para debuxar as curvas)
    double db (double fs, double f) const
    {
        const double w = 2 * PI * f / fs;
        const double cr = std::cos (w), ci = -std::sin (w), c2r = std::cos (2 * w), c2i = -std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i;
        const double dr = 1 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
        return 10.0 * std::log10 ((nr * nr + ni * ni) / std::max (dr * dr + di * di, 1e-30) + 1e-30);
    }
};

// un parámetro que vai ao seu sitio sen saltos (un polo, en ms)
struct Suave
{
    double v = 0, obx = 0, k = 1;
    void preparar (double fs, double ms, double inicial) { k = 1.0 - std::exp (-1.0 / (ms * 0.001 * fs)); v = obx = inicial; }
    double paso() { v += (obx - v) * k; if (std::abs (obx - v) < 1e-9) v = obx; return v; }
    bool quedo() const { return exacto (v, obx); }
};

// seguidor de envolvente (pico, con ataque e soltura en ms)
struct Envolvente
{
    double e = 0, at = 0.1, so = 0.001;
    void preparar (double fs, double ataqueMs, double solturaMs)
    {
        at = 1.0 - std::exp (-1.0 / (std::max (0.01, ataqueMs) * 0.001 * fs));
        so = 1.0 - std::exp (-1.0 / (std::max (0.01, solturaMs) * 0.001 * fs));
    }
    double paso (double x) { const double a = std::abs (x); e += (a - e) * (a > e ? at : so); return e; }
};

// o que lle chega a cada ferramenta en cada bloque
struct Contexto
{
    double mando = 0;        // o mando grande: 0..1 (no filtro, −1..1)
    int selector = 0;        // o selector pequeno (0, 1, 2)
    double extra = 0;        // o mando pequeno (só no sidechain: desprazar, en ms)
    double bpm = 120;        // o tempo do DAW
    double negras = 0;       // onde vai o DAW (en negras) ao comezo do bloque
    bool tocando = false;    // o DAW está a tocar
};

// unha ferramenta: o son, o que tarda e o que se debuxa
class Ferramenta
{
public:
    virtual ~Ferramenta() = default;
    virtual void preparar (double fs, int bloque) = 0;
    virtual void limpar() = 0;
    virtual void axustar (const Contexto&) = 0;            // unha vez por bloque
    virtual void procesar (float* l, float* r, int n) = 0; // no seu sitio
    virtual int latencia() const { return 0; }
    virtual double cola() const { return 0; }
    // para a pantalla: canto baixa agora (dB, ≤ 0) e un valor propio de cada unha
    virtual double reducion() const { return 0; }
    virtual double fase() const { return 0; }
};

} // namespace enx
