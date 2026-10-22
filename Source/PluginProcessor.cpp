#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/Todas.h"

using escoita::U;

namespace
{
    constexpr int ANACO = 512;   // a ferramenta traballa en anacos de ata 512 mostras (igual con calquera bloque)
}

EnxProcessor::EnxProcessor (int t)
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BarulloEnxebre", createLayout (t)),
      tipo_ (juce::jlimit (0, enx::NUM_FERRAMENTAS - 1, t)),
      ferr (enx::crear (tipo_))
{
    pMando = apvts.getRawParameterValue ("mando");
    pSelector = apvts.getRawParameterValue ("selector");
    pExtra = apvts.getRawParameterValue ("desprazar");   // só no sidechain (no resto, nullptr)
    pBypass = apvts.getRawParameterValue ("bypass");
}

EnxProcessor::~EnxProcessor() = default;

const juce::String EnxProcessor::getName() const { return U (descricion().nome); }

juce::AudioProcessorValueTreeState::ParameterLayout EnxProcessor::createLayout (int t)
{
    using namespace juce;
    const auto& d = enx::FERRAMENTAS[jlimit (0, enx::NUM_FERRAMENTAS - 1, t)];
    AudioProcessorValueTreeState::ParameterLayout l;
    const float mn = d.bipolar ? -100.0f : 0.0f;
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mando", 1 }, U (d.nome).fromFirstOccurrenceOf ("Barullo ", false, false),
               NormalisableRange<float> (mn, 100.0f), (float) d.mandoInicial,
               AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
    // o texto do selector pode volver do DAW noutra codificación («Válvula», «Cálido»): compárase só
    // o que é ASCII
    StringArray ops;
    for (auto* o : d.opcions) ops.add (U (o));
    auto ascii = [] (const String& s) { String r; for (auto c : s) if (c < 128) r << String::charToString (c); return r; };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "selector", 1 }, U (d.rotuloSelector), ops, d.selectorInicial,
               AudioParameterChoiceAttributes().withValueFromStringFunction ([ops, ascii] (const String& s)
               {
                   for (int k = 0; k < ops.size(); ++k) if (s == ops[k]) return (float) k;
                   for (int k = 0; k < ops.size(); ++k) if (ascii (s) == ascii (ops[k])) return (float) k;
                   return 0.0f;
               })));
    if (t == enx::SIDECHAIN)
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "desprazar", 1 }, "Desprazar", NormalisableRange<float> (-50.0f, 50.0f), 0.0f,
                   AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bypass", 1 }, "Bypass", StringArray { "Off", "On" }, 0));
    return l;
}

double EnxProcessor::mando() const { return pMando->load() / 100.0; }
int EnxProcessor::selector() const { return juce::jlimit (0, 2, (int) std::lround (pSelector->load())); }

enx::Contexto EnxProcessor::contexto() const
{
    enx::Contexto c;
    c.mando = mando();
    c.selector = selector();
    c.extra = pExtra != nullptr ? (double) pExtra->load() : 0.0;
    c.bpm = bpm.load();
    return c;
}

void EnxProcessor::reservar (int n)
{
    secoOutL.resize ((size_t) n); secoOutR.resize ((size_t) n); monoR.resize ((size_t) n);
}

double EnxProcessor::getTailLengthSeconds() const { return ferr->cola() + latencia / sr; }

bool EnxProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void EnxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    reservar (samplesPerBlock);
    sr = sampleRate;
    const auto c = contexto();
    ferr->axustar (c);   // primeiro os axustes, para que prepare xa nos seus valores
    ferr->preparar (sr, ANACO);
    ferr->axustar (c);
    latencia = ferr->latencia();
    setLatencySamples (latencia);
    secoL.assign ((size_t) latencia + 1, 0.0f); secoR.assign ((size_t) latencia + 1, 0.0f); secoW = 0;
    teito.preparar (sr, 0.080, 0.891);
    bypassS = pBypass->load() > 0.5f ? 1.0 : 0.0;
}

void EnxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    float* L = buffer.getWritePointer (0);
    float* R = outs > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr && ins < 2) std::copy (L, L + n, R);   // mono → estéreo
    if (n > (int) secoOutL.size()) reservar (n);   // (un bloque maior do anunciado: raro)
    const bool mono = R == nullptr;
    if (mono) { std::copy (L, L + n, monoR.begin()); R = monoR.data(); }

    // o DAW: o tempo e onde vai
    auto c = contexto();
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) if (*b > 1.0) { bpm.store (*b); c.bpm = *b; }
            if (auto q = pos->getPpqPosition()) c.negras = *q;
            c.tocando = pos->getIsPlaying() && pos->getPpqPosition().hasValue();
        }
    ferr->axustar (c);

    // o seco (para o bypass), atrasado a latencia da ferramenta
    float pe = 0;
    const int N = (int) secoL.size();
    for (int i = 0; i < n; ++i)
    {
        pe = std::max ({ pe, std::abs (L[i]), std::abs (R[i]) });
        secoL[(size_t) secoW] = L[i]; secoR[(size_t) secoW] = R[i];
        const int ri = (secoW + 1) % N;
        secoOutL[(size_t) i] = secoL[(size_t) ri]; secoOutR[(size_t) i] = secoR[(size_t) ri];
        secoW = (secoW + 1) % N;
    }

    for (int pos = 0; pos < n; pos += ANACO)
        ferr->procesar (L + pos, R + pos, std::min (ANACO, n - pos));

    const double bypassT = pBypass->load() > 0.5f ? 1.0 : 0.0;
    const double sm = 1.0 - std::exp (-1.0 / (0.02 * sr));
    float ps = 0;
    for (int i = 0; i < n; ++i)
    {
        bypassS += (bypassT - bypassS) * sm;
        if (std::abs (bypassS - bypassT) < 1e-6) bypassS = bypassT;
        double l = L[i], r = R[i];
        teito.procesar (l, r);
        l = l * (1.0 - bypassS) + secoOutL[(size_t) i] * bypassS;
        r = r * (1.0 - bypassS) + secoOutR[(size_t) i] * bypassS;
        L[i] = (float) l; R[i] = (float) r;
        ps = std::max ({ ps, (float) std::abs (l), (float) std::abs (r) });
    }
    if (mono) for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
    for (int ch = 2; ch < outs; ++ch) buffer.clear (ch, 0, n);
    nivelEntrada.store (pe); nivelSaida.store (ps);
    reducion.store ((float) ferr->reducion()); fase.store ((float) ferr->fase());
}

void EnxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void EnxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    apvts.replaceState (tree);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));   // que o DAW volva ler os valores
}

juce::AudioProcessorEditor* EnxProcessor::createEditor() { return new EnxEditor (*this); }

#ifndef BARULLO_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EnxProcessor (ENXEBRE_FERRAMENTA); }
#endif
