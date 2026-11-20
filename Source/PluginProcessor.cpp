#include "PluginProcessor.h"
#include "PluginEditor.h"

const std::array<const char*, kNumVoices> DrumkitCloneProcessor::voiceNames {
    "BASS DRUM", "SNARE", "CL HAT", "OP HAT", "CLAP", "RIM", "LOW TOM", "COWBELL"
};

// 0 kick, 1 snare, 2 hat, 3 clap, 4 metallic (rim/cowbell), 5 tom
static const int voiceType[kNumVoices]      = { 0, 1, 2, 2, 3, 4, 5, 4 };
static const float defaultLevel[kNumVoices] = { 0.9f, 0.7f, 0.5f, 0.45f, 0.6f, 0.5f, 0.7f, 0.5f };
static const float defaultDecay[kNumVoices] = { 0.45f, 0.2f, 0.05f, 0.25f, 0.12f, 0.05f, 0.35f, 0.15f };

float DrumVoice::render(double sr, int type, float decaySec, float tuneSemis, float noise)
{
    if (! active)
        return 0.0f;

    const double t = time / sr;
    time += 1.0;

    const double env = std::exp(-t / (double) juce::jmax(0.005f, decaySec));
    if (env < 0.0005)
    {
        active = false;
        return 0.0f;
    }

    const double ratio = std::pow(2.0, tuneSemis / 12.0);
    const double twoPi = juce::MathConstants<double>::twoPi;
    float out = 0.0f;

    switch (type)
    {
        case 0: // kick: sine with pitch drop
        {
            const double f = (50.0 + 120.0 * std::exp(-t * 28.0)) * ratio;
            phase += f / sr;
            out = (float) (std::sin(twoPi * phase) * env);
            break;
        }
        case 1: // snare: tone + noise
        {
            phase += 185.0 * ratio / sr;
            out = (float) ((0.5 * std::sin(twoPi * phase) + 0.6 * noise) * env);
            break;
        }
        case 2: // hats: high-passed noise
        {
            const float hp = noise - lastNoise;
            lastNoise = noise;
            out = (float) (hp * 0.45 * env);
            break;
        }
        case 3: // clap: noise burst
        {
            out = (float) (noise * env * 0.8);
            break;
        }
        case 4: // metallic: rim / cowbell
        {
            phase += 560.0 * ratio / sr;
            out = (float) ((std::sin(twoPi * phase) + 0.6 * std::sin(twoPi * phase * 1.49)) * 0.4 * env);
            break;
        }
        case 5: // tom: sine with pitch drop
        {
            const double f = (80.0 + 70.0 * std::exp(-t * 12.0)) * ratio;
            phase += f / sr;
            out = (float) (std::sin(twoPi * phase) * env * 0.9);
            break;
        }
        default:
            break;
    }
    return out;
}

DrumkitCloneProcessor::DrumkitCloneProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (auto& p : pattern)
        p.store(false);

    // Default groove so it makes sound immediately
    for (int s = 0; s < kSteps; ++s)
    {
        if (s % 8 == 0 || s == 10) toggleStep(0, s);   // kick
        if (s == 4 || s == 12)     toggleStep(1, s);   // snare
        if (s % 2 == 0)            toggleStep(2, s);   // closed hat
    }
    toggleStep(3, 14); // open hat

    for (int v = 0; v < kNumVoices; ++v)
    {
        levelP[(size_t) v] = apvts.getRawParameterValue("level" + juce::String(v));
        tuneP[(size_t) v]  = apvts.getRawParameterValue("tune" + juce::String(v));
        decayP[(size_t) v] = apvts.getRawParameterValue("decay" + juce::String(v));
    }
    masterP = apvts.getRawParameterValue("master");
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumkitCloneProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int v = 0; v < kNumVoices; ++v)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "level" + juce::String(v), 1 }, "Level " + juce::String(v),
            juce::NormalisableRange<float>(0.0f, 1.0f), defaultLevel[v]));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "tune" + juce::String(v), 1 }, "Tune " + juce::String(v),
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "decay" + juce::String(v), 1 }, "Decay " + juce::String(v),
            juce::NormalisableRange<float>(0.02f, 2.0f, 0.001f, 0.5f), defaultDecay[v]));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "master", 1 }, "Master",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    return layout;
}

void DrumkitCloneProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    samplesUntilNext = 0.0;
    stepIndex = 0;
    wasPlaying = false;
}

bool DrumkitCloneProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void DrumkitCloneProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // MIDI notes 36..43 (C1..G#1) trigger the 8 voices directly
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int v = msg.getNoteNumber() - 36;
            if (v >= 0 && v < kNumVoices)
                voices[(size_t) v].trigger();
        }
    }

    // Transport: follow host play state and tempo
    bool playing = false;
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto b = pos->getBpm())
                bpm = *b;
        }
    }

    if (playing && ! wasPlaying)
    {
        stepIndex = 0;
        samplesUntilNext = 0.0;
    }
    wasPlaying = playing;

    const double samplesPerStep = sampleRate * 60.0 / juce::jmax(20.0, bpm) / 4.0; // 16th notes

    float lvl[kNumVoices], tun[kNumVoices], dec[kNumVoices];
    for (int v = 0; v < kNumVoices; ++v)
    {
        lvl[v] = levelP[(size_t) v]->load();
        tun[v] = tuneP[(size_t) v]->load();
        dec[v] = decayP[(size_t) v]->load();
    }
    const float master = masterP->load() * 0.5f;

    const int numSamples = buffer.getNumSamples();
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        if (playing)
        {
            if (samplesUntilNext <= 0.0)
            {
                currentStep.store(stepIndex);
                for (int v = 0; v < kNumVoices; ++v)
                    if (pattern[(size_t) (v * kSteps + stepIndex)].load())
                        voices[(size_t) v].trigger();

                stepIndex = (stepIndex + 1) % kSteps;
                samplesUntilNext += samplesPerStep;
            }
            samplesUntilNext -= 1.0;
        }
        else
        {
            currentStep.store(-1);
        }

        const float noise = rng.nextFloat() * 2.0f - 1.0f;
        float mix = 0.0f;
        for (int v = 0; v < kNumVoices; ++v)
            mix += voices[(size_t) v].render(sampleRate, voiceType[v], dec[v], tun[v], noise) * lvl[v];

        mix *= master;
        L[i] += mix;
        if (R != nullptr)
            R[i] += mix;
    }
}

juce::AudioProcessorEditor* DrumkitCloneProcessor::createEditor()
{
    return new DrumkitCloneEditor(*this);
}

void DrumkitCloneProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto xml = apvts.copyState().createXml();
    juce::String pat;
    for (auto& p : pattern)
        pat << (p.load() ? "1" : "0");
    xml->setAttribute("pattern", pat);
    copyXmlToBinary(*xml, dest);
}

void DrumkitCloneProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            const auto pat = xml->getStringAttribute("pattern");
            for (int i = 0; i < kNumVoices * kSteps && i < pat.length(); ++i)
                pattern[(size_t) i].store(pat[i] == '1');
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumkitCloneProcessor();
}
