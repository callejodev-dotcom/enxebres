// Ferramentas.h — As nove ferramentas de Barullo Enxebres: o nome, a cor, o mando grande, o selector
// pequeno e o que se le ao pasar o rato. Cada plugin é unha delas (ENXEBRE_FERRAMENTA no CMake).
#pragma once
#include <cstdint>

namespace enx {

enum Tipo { AMBIENTE = 0, SATURACION, BRILLO, GRAVES, VOLUME, COMPRESOR, SIDECHAIN, FILTRO, CHORUS, NUM_FERRAMENTAS };

struct Descricion
{
    const char* nome;          // «Barullo Filtro»
    const char* curto;         // «FILTRO» (na placa)
    uint32_t cor;              // a cor das luces (0xRRGGBB)
    bool bipolar;              // o mando vai de −100 a 100 (no medio, nada)
    double mandoInicial;       // en % (o que soa ao abrilo)
    const char* rotuloSelector;   // (na interface vai en maiúsculas)
    const char* opcions[3];
    int selectorInicial;
    const char* axudaMando;
    const char* axudaSelector;
    const char* descricion;    // unha liña
};

inline const Descricion FERRAMENTAS[NUM_FERRAMENTAS] = {
    { "Barullo Ambiente", "AMBIENTE", 0x6FA8FF, false, 30, "Tipo", { "Cuarto", "Sala", "Placa" }, 1,
      "Canta reverb: o tamaño, a mestura e o predelay soben xuntos.",
      "O espazo: un cuarto pequeno, unha sala ou unha placa brillante.",
      "Profundidade nun xesto: unha reverb dun só mando." },
    { "Barullo Saturador", "SATURADOR", 0xFF5C3A, false, 30, "Carácter", { "Cinta", "Válvula", "Fuzz" }, 0,
      "Canto satura: de morno a queimado. O volume compénsase só.",
      "Cinta: suave e redonda. Válvula: harmónicos pares, quente. Fuzz: duro e escuro.",
      "Do morno ao queimado: un saturador dun só mando." },
    { "Barullo Brillo", "BRILLO", 0xFFCD3C, false, 30, "Frecuencia", { "8 kHz", "12 kHz", "16 kHz" }, 1,
      "Canto brillo: sobe os agudos e engade un pouco de harmónicos arriba.",
      "Desde onde sobe: 8 kHz (presenza), 12 kHz (brillo) ou 16 kHz (aire).",
      "Aire e claridade: un realce de agudos dun só mando." },
    { "Barullo Graves", "GRAVES", 0x9674FF, false, 30, "Frecuencia", { "40 Hz", "60 Hz", "100 Hz" }, 1,
      "Canto peso: sobe os graves e engade harmónicos para que se oian tamén en altofalantes pequenos.",
      "Onde está o peso: 40 Hz (sub), 60 Hz (bombo) ou 100 Hz (corpo).",
      "Peso e corpo: un realce de graves dun só mando." },
    { "Barullo Volume", "VOLUME", 0xFF52A6, false, 30, "Carácter", { "Limpo", "Cálido", "Duro" }, 0,
      "Canto máis forte: sobe o volume e un limitador fai que nunca pase de −1 dBFS.",
      "Limpo: transparente. Cálido: satura un chisco antes. Duro: solta axiña e recorta os picos.",
      "Máis forte, sen pasar do teito: un limitador dun só mando." },
    { "Barullo Compresor", "COMPRESOR", 0x50D48E, false, 30, "Velocidade", { "Lento", "Medio", "Rápido" }, 1,
      "Canto comprime: o limiar baixa e a proporción sobe xuntos; o volume compénsase só.",
      "Lento: deixa pasar os golpes. Medio: para case todo. Rápido: aperta tamén os golpes.",
      "De suave a esmagado: un compresor dun só mando." },
    { "Barullo Sidechain", "SIDECHAIN", 0xFF9C24, false, 50, "Ritmo", { "1/2", "1/4", "1/8" }, 1,
      "Canto bombea: o volume baixa en cada tempo e volve, coma cun bombo no sidechain.",
      "Cada canto: brancas, negras ou corcheas, á par do tempo do DAW.",
      "O bombeo, ao tempo do DAW: un sidechain sen sidechain." },
    { "Barullo Filtro", "FILTRO", 0x34CDC8, true, 0, "Resonancia", { "Baixa", "Media", "Alta" }, 1,
      "No medio, nada. Á esquerda, o filtro do Juno-106 vai pechando (máis escuro); á dereita, un paso alto (máis fino).",
      "Canto resoa o filtro no corte.",
      "Escuro ← · → fino: o filtro do Juno-106 nun só mando." },
    { "Barullo Chorus", "CHORUS", 0xB4E04A, false, 50, "Modo", { "I", "II", "I+II" }, 0,
      "Canto chorus: do son seco ao chorus enteiro do Juno-106 (e algo máis aberto).",
      "Os modos do Juno: I lento e fondo, II máis rápido, I+II un vibrato rápido.",
      "O chorus do Juno-106, cos seus dous BBD." },
};

} // namespace enx
