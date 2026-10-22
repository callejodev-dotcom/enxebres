**Barullo Enxebres** son nove ferramentas dun só mando. Cada unha ten un mando grande que move por dentro todo o que fai falta, un selector pequeno e unha pantalla que ensina o que está a facer. Un só instalador leva as nove.

| | Fai | O mando | O selector |
|---|---|---|---|
| **Barullo Ambiente** | reverb | tamaño, mestura e predelay xuntos | cuarto · sala · placa |
| **Barullo Saturador** | saturación | de morno a queimado, co volume compensado | cinta · válvula · fuzz |
| **Barullo Brillo** | brillo | agudos (ata +12 dB) e harmónicos arriba | 8 · 12 · 16 kHz |
| **Barullo Graves** | graves | graves (ata +10 dB) e os seus harmónicos | 40 · 60 · 100 Hz |
| **Barullo Volume** | volume | ata +14 dB, sen pasar de −1 dBFS | limpo · cálido · duro |
| **Barullo Compresor** | compresión | limiar e proporción xuntos, co volume compensado | lento · medio · rápido |
| **Barullo Sidechain** | bombeo | canto baixa en cada tempo (ata −40 dB) | 1/2 · 1/4 · 1/8 e *desprazar* |
| **Barullo Filtro** | filtro | no medio nada; á esquerda o filtro do Juno-106, á dereita un paso alto | resonancia baixa · media · alta |
| **Barullo Chorus** | chorus | do seco ao chorus enteiro do Juno-106 | I · II · I+II |

Unha ferramenta de [Escoita](https://www.escoita.link).

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+): AU, VST3 e CLAP | `BarulloEnxebres-…-mac.pkg` | `BarulloEnxebres-…-mac.zip` |
| **Windows** (64 bits): VST3 e CLAP | `BarulloEnxebres-…-windows-setup.exe` | `BarulloEnxebres-…-windows-x64.zip` |
| **Linux** (x86_64): VST3, LV2 e CLAP | — | `BarulloEnxebres-…-linux-x86_64.tar.gz` → `./instalar.sh` |

Se o sistema avisa ao abrilo:
- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.

## Como se usa

- O **mando grande** é todo: xira e escoita. Dobre clic, ao valor de partida.
- O **selector** cambia o carácter (ou a frecuencia, ou o ritmo).
- A **luz** de arriba á dereita é o bypass: apagada, o son pasa tal cal.
- A **pantalla** ensina o que fai: a caída da reverb, a curva da saturación ou do compresor, a ecualización, o volume contra o teito, o bombeo co tempo, a resposta do filtro e os dous BBD do chorus.
- Os medidores dos lados: a entrada e a saída.

## Como se fixo

- **Os motores doutros plugins non se copian**: o CMake descarga Barullo Xuno e Barullo Verb nunha versión fixada (v0.1.0) e usa o filtro IR3109 e os BBD MN3009 do Xuno e a reverb do Verb. Se melloran, Barullo Enxebres mellora ao subir esa versión.
- **Un código para os nove**: cada plugin compílase co seu número (`ENXEBRE_FERRAMENTA`) e escolle a súa ferramenta (`Source/dsp/`). A interface é a mesma, coa cor e a pantalla de cada un.
- **Saturador e Chorus** traballan a catro veces a frecuencia de mostraxe; **Volume** mira 1,5 ms adiante. Os tres din a súa latencia ao DAW.
- **A 0, o son tal cal** en Ambiente, Brillo, Graves, Compresor, Sidechain e Filtro (bit a bit).
- **O saturador** compensa o volume medindo a súa propia curva: un seno a −18 dBFS sae co mesmo volume con calquera carácter e mando.
- **O sidechain** segue a posición do DAW mentres toca; parado, vai só ao tempo do DAW (ou a 120).

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/Ferramentas.h` | As nove: nome, cor, mando, selector e axudas |
| `Source/dsp/*.h` | O son de cada unha (e `Comun.h`, o que comparten) |
| `Source/PluginProcessor.*` | O plugin (o mesmo para as nove) |
| `Source/PluginEditor.*` | A interface e as pantallas |
| `tools/RenderTest.cpp` | A proba |

Usa o núcleo común de Escoita ([escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo), v0.4.0): o texto en galego, o aviso das versións novas, o limitador de seguridade (`escoita::Teito`), a CI e o instalador da serie.

## Probas

Para cada unha das nove: ao abrir nos valores de partida, finita e baixo o teito co mando ao máximo, o mesmo son con bloques de 512 e de 37 mostras, o bypass (o son tal cal, coa latencia), o estado e o mono. E, medido:

- **Ambiente**: a cola segue despois do son, máis longa na sala ca no cuarto.
- **Saturador**: o volume queda parello (±3,5 dB) nos tres caracteres e aparecen harmónicos.
- **Brillo / Graves**: suben onde din e non tocan o resto.
- **Volume**: un son baixo sobe 14 dB; un forte non pasa de −1 dBFS.
- **Compresor**: aperta os fortes e compensa.
- **Sidechain**: baixa 40 dB en cada tempo e volve; *desprazar* move a baixada.
- **Filtro**: o paso baixo pecha os agudos; o paso alto, os graves.
- **Chorus**: estéreo nos modos I e II, mono no I+II.
- **Validación**: pluginval ao nivel máximo (10), auval no Mac e clap-validator, nos nove.

Na CI van en Mac, Windows e Linux, e tamén con AddressSanitizer e UBSan (`tools/probas-ci.sh`).

## Compilar

Nun Mac: `./build_mac.sh` (compila e instala os nove no teu usuario). Á man:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBARULLO_TESTS=ON
cmake --build build --config Release
build/BERenderTest_artefacts/Release/BERenderTest
```

Outras ordes da proba: `--snapshot saida.png <0–8>` (a interface dunha), `--galeria saida.png` (as nove) e `--render saida.wav <0–8> [mando %] [selector]`.

## Publicar unha versión

```
git tag v0.1.0 && git push origin v0.1.0
```

A versión sae da liña `project(BarulloEnxebres VERSION …)` de `CMakeLists.txt`; o texto da versión, de `packaging/NOTAS-VERSION.md`.

## Créditos e licenzas

- **Barullo Enxebres é software libre**, baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`): podes usalo, estudalo, modificalo e compartilo; se distribúes unha versión modificada, ten que ir co seu código e coa mesma licenza. Usa JUCE 8 coa súa licenza AGPLv3.
- O filtro e o chorus son os de Barullo Xuno, e a reverb a de Barullo Verb (ver o LEEME de cada un).
- Tipografías Montserrat e Inter (licenza OFL, `Resources/Montserrat-OFL.txt` e `Resources/Inter-OFL.txt`).
- Juno-106 é unha marca de Roland Corporation. Barullo Enxebres non ten relación con ela.
