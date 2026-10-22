#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <escoita_nucleo/escoita_nucleo.h>
#include "Ferramentas.h"
#include "dsp/Comun.h"

#ifndef ENXEBRE_FERRAMENTA
 #define ENXEBRE_FERRAMENTA 0
#endif

// Un plugin de Barullo Enxebres: unha das nove ferramentas (Ferramentas.h), cun mando grande, un
// selector pequeno e, no sidechain, «desprazar». Cada plugin compílase co seu ENXEBRE_FERRAMENTA;
// a proba crea as nove.
class EnxProcessor : public juce::AudioProcessor
{
public:
    explicit EnxProcessor (int tipo = ENXEBRE_FERRAMENTA);
    ~EnxProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (int tipo);

    int tipo() const { return tipo_; }
    const enx::Descricion& descricion() const { return enx::FERRAMENTAS[tipo_]; }
    double mando() const;     // 0..1 (no filtro, −1..1)
    int selector() const;
    double tempoDAW() const { return bpm.load(); }
    double frecuencia() const { return sr; }

    juce::AudioProcessorValueTreeState apvts;

    // para a pantalla (escríbese desde o fío de audio)
    std::atomic<float> nivelEntrada { 0 }, nivelSaida { 0 }, reducion { 0 }, fase { 0 };
    float uiScale = 1.0f;

private:
    enx::Contexto contexto() const;

    const int tipo_;
    std::unique_ptr<enx::Ferramenta> ferr;
    escoita::Teito teito;
    std::atomic<double> bpm { 120.0 };
    double sr = 48000, bypassS = 0;
    int latencia = 0;
    std::vector<float> secoL, secoR;   // o son seco, atrasado o que tarda a ferramenta (para o bypass)
    std::vector<float> secoOutL, secoOutR, monoR;
    int secoW = 0;
    void reservar (int n);
    std::atomic<float> *pMando = nullptr, *pSelector = nullptr, *pExtra = nullptr, *pBypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnxProcessor)
};
