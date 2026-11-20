#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

constexpr int kNumVoices = 8;
constexpr int kSteps = 16;

// Simple synthesized drum voice (no samples needed).
struct DrumVoice
{
    bool active = false;
    double time = 0.0;
    double phase = 0.0;
    float lastNoise = 0.0f;

    void trigger() { active = true; time = 0.0; phase = 0.0; }
    float render(double sr, int type, float decaySec, float tuneSemis, float noise);
};

class DrumkitCloneProcessor : public juce::AudioProcessor
{
public:
    DrumkitCloneProcessor();
    ~DrumkitCloneProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DrumKit Clone"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Pattern access for the UI
    bool isStepOn(int voice, int step) const { return pattern[(size_t) (voice * kSteps + step)].load(); }
    void toggleStep(int voice, int step)
    {
        auto& s = pattern[(size_t) (voice * kSteps + step)];
        s.store(! s.load());
    }

    static const std::array<const char*, kNumVoices> voiceNames;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentStep { -1 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<std::atomic<bool>, kNumVoices * kSteps> pattern {};
    std::array<DrumVoice, kNumVoices> voices;
    std::array<std::atomic<float>*, kNumVoices> levelP {}, tuneP {}, decayP {};
    std::atomic<float>* masterP = nullptr;

    juce::Random rng;
    double sampleRate = 44100.0;
    double samplesUntilNext = 0.0;
    int stepIndex = 0;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumkitCloneProcessor)
};
