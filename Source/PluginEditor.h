#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace ui
{
    // A familia Barullo: o azul mariño da noite de Vigo, a crema da letra de Escoita e a luz laranxa,
    // nun aparello de estudio (paneis en relevo, pantallas de cristal, visores dourados).
    namespace col
    {
        const juce::Colour noite    { 0xff0c111f };
        const juce::Colour crema    { 0xfffff7c1 };
        const juce::Colour crema2   { 0xffbdb792 };
        const juce::Colour crema3   { 0xff857f67 };
        const juce::Colour laranxa  { 0xffe08b16 };
        const juce::Colour ouro     { 0xffedba23 };
        const juce::Colour verde    { 0xff9dc6a0 };
        const juce::Colour azul     { 0xff8fb0e8 };
        const juce::Colour panel1   { 0xff1c2540 }, panel2 { 0xff111830 };
        const juce::Colour tecla1   { 0xff27314f }, tecla2 { 0xff1a2239 };
        const juce::Colour visor    { 0xff070b16 };
        const juce::Colour cristal1 { 0xff142046 }, cristal2 { 0xff070c1c };
        const juce::Colour lina     { 0xff2a3350 };
    }

    juce::Font body (float size);    // Inter
    juce::Font semi (float size);    // Montserrat SemiBold
    juce::Font cond (float size);    // Montserrat Bold
    juce::Font caps (float size, float tracking = 0.18f);
    juce::String upper (const juce::String&);
    void drawCaps (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, float size, float tracking = 0.18f);
    void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour);
    void paintPanel (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintWell (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintGlass (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintKey (juce::Graphics&, juce::Rectangle<float>, bool over, bool down);
    void paintLed (juce::Graphics&, juce::Point<float>, bool on, juce::Colour = col::laranxa);
    void groupTitle (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour = col::laranxa);

    struct Look : juce::LookAndFeel_V4
    {
        Look();
        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        juce::Font getPopupMenuFont() override { return body (15.0f); }
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    };

    // o mando grande: anel de LED na cor da ferramenta, saia moleteada e tapa metálica
    struct MandoGrande : juce::Slider
    {
        MandoGrande();
        void paint (juce::Graphics&) override;
        juce::Colour cor;
        bool bipolar = false;
    };

    // o mando pequeno (desprazar, no sidechain)
    struct MandoPequeno : juce::Slider
    {
        MandoPequeno();
        void paint (juce::Graphics&) override;
        juce::Colour cor;
    };

    // a luz de acendido: acendida, a ferramenta traballa; un clic, bypass
    struct Luz : juce::Component, juce::SettableTooltipClient
    {
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        juce::Colour cor;
        bool acesa = true;
        std::function<void()> onClick;
    };

    // a pantalla: o que fai a ferramenta, debuxado (cada unha a súa)
    class Pantalla : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Pantalla (EnxProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void tick();
    private:
        void ambiente (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void saturacion (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void eq (juce::Graphics&, juce::Rectangle<float>, juce::Colour, bool agudos);
        void volume (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void compresor (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void sidechain (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void filtro (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void chorus (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
        void rotulos (juce::Graphics&, const juce::String& esq, const juce::String& der, juce::Colour);
        EnxProcessor& proc;
        std::array<float, 48> historia {};   // os últimos picos da saída (volume)
        int hPos = 0;
        float entrada = 0, saida = 0, red = 0;
        friend class MainView;
    };

    class MainView : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int W = 340, H = 540;
        explicit MainView (EnxProcessor&);
        ~MainView() override;
        void paint (juce::Graphics&) override;
        void resized() override;
        void timerCallback() override;

    private:
        juce::String lectura() const;   // o valor do mando, no visor

        EnxProcessor& proc;
        Look look;
        juce::TooltipWindow tooltips { this, 500 };
        juce::Colour cor;
        MandoGrande mando;
        MandoPequeno desprazar;
        Luz luz;
        juce::TextButton opcions[3], newVersion;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mandoAtt, desprazarAtt;
        std::unique_ptr<juce::ParameterAttachment> selectorAtt, bypassAtt;
        Pantalla pantalla;
        float entrada = 0, saida = 0;
        juce::SharedResourcePointer<escoita::Actualizacions> versions;
    };
}

class EnxEditor : public juce::AudioProcessorEditor
{
public:
    explicit EnxEditor (EnxProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::col::noite); }
    ui::MainView& view() { return main; }
private:
    EnxProcessor& proc;
    ui::MainView main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnxEditor)
};
