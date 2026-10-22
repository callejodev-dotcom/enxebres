// Todas.h — As nove ferramentas e a fábrica (unha por número, coma en Ferramentas.h).
#pragma once
#include "../Ferramentas.h"
#include "Ambiente.h"
#include "Saturacion.h"
#include "Brillo.h"
#include "Graves.h"
#include "Volume.h"
#include "Compresor.h"
#include "Sidechain.h"
#include "Filtro.h"
#include "Chorus.h"

namespace enx {

inline std::unique_ptr<Ferramenta> crear (int tipo)
{
    switch (tipo)
    {
        case AMBIENTE:   return std::make_unique<Ambiente>();
        case SATURACION: return std::make_unique<Saturacion>();
        case BRILLO:     return std::make_unique<Brillo>();
        case GRAVES:     return std::make_unique<Graves>();
        case VOLUME:     return std::make_unique<Volume>();
        case COMPRESOR:  return std::make_unique<Compresor>();
        case SIDECHAIN:  return std::make_unique<Sidechain>();
        case FILTRO:     return std::make_unique<Filtro>();
        default:         return std::make_unique<Chorus>();
    }
}

} // namespace enx
