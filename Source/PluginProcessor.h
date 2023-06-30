/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "dsp/EqBandDsp.h"
#include "dsp/FFTAnalyser.h"

//==============================================================================
/**
*/
class AFEQAudioProcessor  : public juce::AudioProcessor
{
private:
    juce::AudioProcessorValueTreeState::ParameterLayout getLayout();

public:

    enum AnalyserProcessing
    {
        kAnalyserDisabled,
        kAnalyserPre,
        kAnalyserPost
    };

    //==============================================================================
    AFEQAudioProcessor();
    ~AFEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    bool dspResponseChanged() const;
    void updateGlobalResponse();

    juce::AudioProcessorValueTreeState& getAPValueTreeState();

    EqBandDspGroup eqBands;
    std::unique_ptr<FFTAnalyser> fftAnalyser;
    static constexpr int numBands = 12;
    static constexpr int maxOrder = 8;
    float guiScale = 1.6f;
    AnalyserProcessing analyserProc = kAnalyserDisabled;

private:

    FreqResponseBase freqResBase = FreqResponseBase(300, 20.f, 20e3f);
    juce::UndoManager undoManager;
    std::unique_ptr<juce::AudioProcessorValueTreeState> state;
    juce::AudioBuffer<double> procBuffer;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fftBuffer;
    AnalyserProcessing prevAnalyserProc = kAnalyserDisabled;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AFEQAudioProcessor)
};
