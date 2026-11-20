#include "PluginEditor.h"

namespace
{
constexpr int kW = 900, kH = 430;
constexpr int kX0 = 30, kColW = 100, kKnob = 56;
constexpr int kLevelY = 92, kTuneY = 170, kDecayY = 248, kVoiceY = 318;
constexpr int kStepY = 362, kStepW = 52, kStepH = 52;
}

DrumkitCloneEditor::DrumkitCloneEditor(DrumkitCloneProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);
    setSize(kW, kH);

    auto& apvts = proc.apvts;

    auto addKnob = [this, &apvts](juce::Slider& s, const juce::String& id, bool isLevel)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setName(isLevel ? "level" : "knob");
        addAndMakeVisible(s);
        attachments.push_back(std::make_unique<SliderAttachment>(apvts, id, s));
    };

    for (int v = 0; v < kNumVoices; ++v)
    {
        addKnob(levelS[(size_t) v], "level" + juce::String(v), true);
        addKnob(tuneS[(size_t) v], "tune" + juce::String(v), false);
        addKnob(decayS[(size_t) v], "decay" + juce::String(v), false);

        auto& b = voiceB[(size_t) v];
        b.setButtonText(DrumkitCloneProcessor::voiceNames[(size_t) v]);
        b.onClick = [this, v] { selectVoice(v); };
        addAndMakeVisible(b);
    }

    addKnob(masterS, "master", false);

    for (int s = 0; s < kSteps; ++s)
    {
        stepB[(size_t) s] = std::make_unique<StepButton>(proc, s);
        addAndMakeVisible(*stepB[(size_t) s]);
    }

    selectVoice(0);
    startTimerHz(30);
}

void DrumkitCloneEditor::selectVoice(int v)
{
    selected = v;
    for (int i = 0; i < kNumVoices; ++i)
        voiceB[(size_t) i].setColour(juce::TextButton::buttonColourId,
                                     i == v ? juce::Colour(0xffd62e2a) : juce::Colour(0xff1d3b73));
    for (auto& s : stepB)
        s->setVoice(v);
    repaint();
}

void DrumkitCloneEditor::timerCallback()
{
    const int cs = proc.currentStep.load();
    for (int s = 0; s < kSteps; ++s)
        stepB[(size_t) s]->setPlaying(s == cs);
}

void DrumkitCloneEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff6f7174));

    auto panel = getLocalBounds().toFloat().reduced(12.0f);
    g.setColour(juce::Colour(0xffe9e8e3));
    g.fillRoundedRectangle(panel, 6.0f);
    g.setColour(juce::Colour(0xffb9b9b4));
    g.drawRoundedRectangle(panel, 6.0f, 1.5f);

    // Title
    g.setColour(juce::Colour(0xff1b1b1b));
    g.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    g.drawText("DRUMKIT CLONE", 30, 22, 400, 30, juce::Justification::left, false);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("COMPUTER CONTROLLED  \u2022  16 STEP", 30, 50, 400, 16, juce::Justification::left, false);
    g.drawText("MASTER", 806, 56, 80, 12, juce::Justification::centred, false);

    // Section divider
    g.setColour(juce::Colour(0xff2b2b2b));
    g.fillRect(30, 68, kW - 60, 2);

    // Per-channel labels and separators
    g.setFont(juce::FontOptions(10.0f));
    for (int v = 0; v < kNumVoices; ++v)
    {
        const int x = kX0 + v * kColW;
        g.setColour(juce::Colour(0xff2b2b2b));
        g.drawText("LEVEL", x, kLevelY - 16, kColW, 14, juce::Justification::centred, false);
        g.drawText("TUNE", x, kTuneY - 16, kColW, 14, juce::Justification::centred, false);
        g.drawText("DECAY", x, kDecayY - 16, kColW, 14, juce::Justification::centred, false);

        if (v > 0)
        {
            g.setColour(juce::Colour(0x33000000));
            g.fillRect(x, 80, 1, kVoiceY + 28 - 80);
        }
    }

    // Step row divider and label
    g.setColour(juce::Colour(0xff2b2b2b));
    g.fillRect(30, kStepY - 14, kW - 60, 1);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("STEP  \u2022  " + juce::String(DrumkitCloneProcessor::voiceNames[(size_t) selected]),
               30, kStepY - 30, 300, 14, juce::Justification::left, false);
    g.drawText("BASIC RHYTHM", 30, kStepY + kStepH + 6, 200, 14, juce::Justification::left, false);
    g.drawText("INTRO / FILL IN", kW - 230, kStepY + kStepH + 6, 200, 14, juce::Justification::right, false);
}

void DrumkitCloneEditor::resized()
{
    for (int v = 0; v < kNumVoices; ++v)
    {
        const int x = kX0 + v * kColW + (kColW - kKnob) / 2;
        levelS[(size_t) v].setBounds(x, kLevelY, kKnob, kKnob);
        tuneS[(size_t) v].setBounds(x, kTuneY, kKnob, kKnob);
        decayS[(size_t) v].setBounds(x, kDecayY, kKnob, kKnob);
        voiceB[(size_t) v].setBounds(kX0 + v * kColW + 4, kVoiceY, kColW - 8, 26);
    }

    masterS.setBounds(826, 6, 50, 50);

    for (int s = 0; s < kSteps; ++s)
        stepB[(size_t) s]->setBounds(kX0 + s * kStepW + 1, kStepY, kStepW - 3, kStepH);
}
