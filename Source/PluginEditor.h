#pragma once
#include "PluginProcessor.h"

// Look: off-white panel, grey frame, red level caps, dark-navy channel buttons (TR-808 inspired).
class DrumLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DrumLookAndFeel()
    {
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1d3b73));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override
    {
        auto area = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(3.0f);
        const float r = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
        const auto c = area.getCentre();
        const bool isLevel = s.getName() == "level";

        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillEllipse(c.x - r - 2.5f, c.y - r - 2.5f, (r + 2.5f) * 2.0f, (r + 2.5f) * 2.0f);

        g.setColour(isLevel ? juce::Colour(0xffd62e2a) : juce::Colour(0xff8f9092));
        g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        g.setColour(juce::Colour(0x33000000));
        g.drawEllipse(c.x - r * 0.7f, c.y - r * 0.7f, r * 1.4f, r * 1.4f, 1.0f);

        const float a = startAngle + pos * (endAngle - startAngle);
        const float sa = std::sin(a), ca = std::cos(a);
        g.setColour(juce::Colours::white);
        g.drawLine(c.x + sa * r * 0.15f, c.y - ca * r * 0.15f,
                   c.x + sa * r * 0.85f, c.y - ca * r * 0.85f, 2.5f);
    }
};

// One step of the 16-step row. Colours follow the 808 groups: 1-4 red, 5-8 orange, 9-12 pale, 13-16 white.
class StepButton : public juce::Button
{
public:
    StepButton(DrumkitCloneProcessor& p, int idx) : juce::Button("step"), proc(p), index(idx) {}

    void setVoice(int v) { voice = v; repaint(); }
    void setPlaying(bool b) { if (b != playing) { playing = b; repaint(); } }

    void clicked() override
    {
        proc.toggleStep(voice, index);
        repaint();
    }

    void paintButton(juce::Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        const bool on = proc.isStepOn(voice, index);

        juce::Colour base = index < 4  ? juce::Colour(0xffd6342c)
                          : index < 8  ? juce::Colour(0xfff0b53b)
                          : index < 12 ? juce::Colour(0xfff6ea8c)
                                       : juce::Colour(0xfffafafa);

        g.setColour(on ? base : base.withMultipliedAlpha(0.35f));
        g.fillRoundedRectangle(r, 4.0f);

        if (playing)
        {
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(r, 4.0f, 3.0f);
        }

        g.setColour(juce::Colour(0xff3a3a3a));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String(index + 1), getLocalBounds(), juce::Justification::centredBottom, false);
    }

private:
    DrumkitCloneProcessor& proc;
    int index;
    int voice = 0;
    bool playing = false;
};

class DrumkitCloneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DrumkitCloneEditor(DrumkitCloneProcessor&);
    ~DrumkitCloneEditor() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;
    void selectVoice(int v);

    DrumkitCloneProcessor& proc;
    DrumLookAndFeel laf;
    int selected = 0;

    std::array<juce::Slider, kNumVoices> levelS, tuneS, decayS;
    juce::Slider masterS;
    std::array<juce::TextButton, kNumVoices> voiceB;
    std::array<std::unique_ptr<StepButton>, kSteps> stepB;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumkitCloneEditor)
};
