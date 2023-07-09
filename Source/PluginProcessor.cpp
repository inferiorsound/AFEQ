/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


juce::AudioProcessorValueTreeState::ParameterLayout AFEQAudioProcessor::getLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto freqRange = juce::NormalisableRange<float>(20.f, 20e3f,
        [](float s, float e, float v) { return juce::jlimit(s, e, s * std::exp(std::log(e / s) * v)); },
        [](float s, float e, float v) { return juce::jlimit(0.f, 1.f, std::log(v / s) / std::log(e / s)); },
        [](float s, float e, float v) { v = juce::jlimit(s, e, v); return v < 1000 ? 0.1f * std::round(10.f * v) : std::round(v); });
    auto gainRange = juce::NormalisableRange<float>(-24.f, 24.f, 0.1f);
    auto qRange = juce::NormalisableRange<float>(0.1f, 10.f,
        [](float s, float e, float v) { return juce::jlimit(s, e, s * std::exp(std::log(e / s) * v)); },
        [](float s, float e, float v) { return juce::jlimit(0.f, 1.f, std::log(v / s) / std::log(e / s)); },
        [](float s, float e, float v) { v = juce::jlimit(s, e, v); return 0.01f * std::round(100.f * v); });

    auto freqToStr = [](float v, int /*maxLen*/) {
        return v < 1e3f ? juce::String(static_cast<int> (v)) : juce::String(v*0.001f, 2) + "k"; 
    };
    auto strToFreq = [](const juce::String& s) {
        const auto s2 = s.replace("k", "."); 
        const auto factor = s2.contains(".") ? 1000.f : 1.f;
        return s2.getFloatValue() * factor;
    };
    const auto types = BandParams::getTypeNames();
    const auto routings = BandParams::getRoutingNames();

    for (int i = 0; i < numBands; ++i)
    {
        auto eqBand = eqBands.add(std::make_unique<EqBandDsp>(maxOrder, freqResBase, i+1));
        auto& bp = eqBand->getBandParams();
        bp.setBandId("Band " + juce::String(i + 1));
        auto grp = std::make_unique<juce::AudioProcessorParameterGroup>("band"+juce::String(i), bp.getBandId(), "|");
        grp->addChild(std::make_unique<juce::AudioParameterBool> (juce::ParameterID(bp.enableId.toString(), 1), bp.enableId.toString(), false));
        grp->addChild(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(bp.typeId.toString(), 1), bp.typeId.toString(), types, 0));
        grp->addChild(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(bp.routingId.toString(), 1), bp.routingId.toString(), routings, 0));
        grp->addChild(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(bp.freqId.toString(), 1), bp.freqId.toString(),
            freqRange, 1000.f, "Hz", juce::AudioProcessorParameter::genericParameter, freqToStr, strToFreq));
        grp->addChild(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID(bp.gainId.toString(), 1), bp.gainId.toString(),
            gainRange, 0.f, "dB"));
        grp->addChild(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(bp.qId.toString(), 1), bp.qId.toString(),
            qRange, 0.707f, "", juce::AudioProcessorParameter::genericParameter, 
            [](float v, int) { return juce::String(v, 3); }));
        grp->addChild(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(bp.orderId.toString(), 1), bp.orderId.toString(),
            1, AFEQAudioProcessor::maxOrder, 2, "db/Oct",
            [](int v, int) { return juce::String(6 * v); },
            [](const juce::String& s) { return juce::jlimit(1, AFEQAudioProcessor::maxOrder, s.getIntValue() / 6); }));

        layout.add(std::move(grp));
    }

    return layout;
}

//==============================================================================
AFEQAudioProcessor::AFEQAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    state = std::make_unique<juce::AudioProcessorValueTreeState>(*this, &undoManager, "STATE", getLayout());
    
    for (auto band : eqBands)
        band->getBandParams().syncParameters(*state);
}

AFEQAudioProcessor::~AFEQAudioProcessor()
{
    state.reset();
}

//==============================================================================
const juce::String AFEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AFEQAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AFEQAudioProcessor::producesMidi() const
{
    return false;
}

bool AFEQAudioProcessor::isMidiEffect() const
{
    return false;
}

double AFEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AFEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AFEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AFEQAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String AFEQAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void AFEQAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void AFEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    procBuffer.setSize(2, samplesPerBlock);
    freqResBase.setSampleRate(static_cast<float> (sampleRate));

    for (auto b : eqBands)
    {
        b->setBlockSize(samplesPerBlock);
        b->setSampleRate(sampleRate);
    }

    {
        const int fftOrder = 13;
        const int numAnalyserBands = 61;
        const int overlap = 4;
        fftAnalyser = std::make_unique<FFTAnalyser>(fftOrder, overlap, numAnalyserBands, 0.3f, static_cast<int> (sampleRate));
        fft = std::make_unique<juce::dsp::FFT>(fftOrder);
        fftBuffer.resize(1 << (fftOrder + 1));

        fftAnalyser->performFFT = [this](float* data, int size) {
            juce::FloatVectorOperations::copy(fftBuffer.data(), data, size);
            fft->performRealOnlyForwardTransform(fftBuffer.data(), true);
            const auto gain = std::sqrt(static_cast<float> (size) / static_cast<float> (1 << 14)) / static_cast<float> (std::sqrt(size));
            juce::FloatVectorOperations::copyWithMultiply(data, fftBuffer.data(), gain, size);
        };
    }
}

void AFEQAudioProcessor::releaseResources()
{
}

bool AFEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AFEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto in = buffer.getReadPointer(ch);
        auto out = procBuffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            out[i] = static_cast<double> (in[i]);
    }

    processBlock(procBuffer, midiMessages);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto in = procBuffer.getReadPointer(ch);
        auto out = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            out[i] = static_cast<float> (in[i]);
    }
}

void AFEQAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    auto chL = buffer.getWritePointer(0);
    auto chR = numChannels == 1 ? nullptr : buffer.getWritePointer(1);

    if (prevAnalyserProc != analyserProc)
        fftAnalyser->clear();

    prevAnalyserProc = analyserProc;

    if (analyserProc == kAnalyserPre)
        fftAnalyser->processBlock(chL, chR, numSamples);

    for (auto b : eqBands)
        b->processBlock(chL, chR, numSamples);

    if (dspResponseChanged())
        updateGlobalResponse();

    if (analyserProc == kAnalyserPost)
        fftAnalyser->processBlock(chL, chR, numSamples);
}

//==============================================================================
bool AFEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AFEQAudioProcessor::createEditor()
{
    const auto scale = guiScale;
    auto ed = new AFEQAudioProcessorEditor(*this);
    guiScale = scale;
    juce::MessageManager::callAsync([this]() { 
        if (auto ed = dynamic_cast<AFEQAudioProcessorEditor*> (getActiveEditor()))
            ed->setScale(guiScale, true); 
    });
    return ed;
}

//==============================================================================
void AFEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto s2 = state->copyState();
    std::unique_ptr<juce::XmlElement> xml(std::make_unique<juce::XmlElement> ("AFEQSTATE"));
    xml->setAttribute("scale", guiScale);
    xml->setAttribute("analyser", analyserProc);
    xml->addChildElement(s2.createXml().release());
    copyXmlToBinary(*xml, destData);
}

void AFEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr && xmlState->hasTagName("AFEQSTATE"))
    {
        if (auto vtState = xmlState->getChildByName(state->state.getType()))
            state->replaceState(juce::ValueTree::fromXml(*vtState));

        guiScale = static_cast<float> (xmlState->getDoubleAttribute("scale"));
        analyserProc = static_cast<AnalyserProcessing> (xmlState->getIntAttribute("analyser", static_cast<int> (analyserProc)));

        if (auto ed = dynamic_cast<AFEQAudioProcessorEditor*>(getActiveEditor()))
            juce::MessageManager::callAsync([ed, this]() { ed->syncWithProcessor(); });
    }
}

bool AFEQAudioProcessor::dspResponseChanged() const
{
    auto changed = false;
    for (auto b : eqBands)
        changed |= b->getAndClearResUpdate();

    return changed;
}

void AFEQAudioProcessor::updateGlobalResponse()
{
    auto& resSt = freqResBase.resStereo;
    auto& resL = freqResBase.resLeft;
    auto& resR = freqResBase.resRight;
    auto& resM = freqResBase.resMid;
    auto& resS = freqResBase.resSide;
    AudioFilter::Response::initGains(resSt.res, resSt.res.size());
    AudioFilter::Response::initGains(resL.res, resL.res.size());
    AudioFilter::Response::initGains(resR.res, resR.res.size());
    AudioFilter::Response::initGains(resM.res, resM.res.size());
    AudioFilter::Response::initGains(resS.res, resS.res.size());
    resSt.used = false;
    resL.used = false;
    resR.used = false;
    resM.used = false;
    resS.used = false;


    for (auto b : eqBands)
    {
        jassert(resSt.res.size() == b->getResponse().size());

        if (b->getBandParamsConst().getEnabled())
        {
            const auto routing = b->getBandParamsConst().getRouting();

            auto& res = routing == BandParams::routeLeft ? resL
                : routing == BandParams::routeRight ? resR
                : routing == BandParams::routeMid ? resM
                : routing == BandParams::routeSide ? resS
                : resSt;

            res.used = true;
            juce::FloatVectorOperations::multiply(res.res.data(), b->getResponse().data(), static_cast<int> (res.res.size()));
        }

    }

    if (!resL.used && !resR.used && !resM.used && !resS.used)
        resSt.used = true;

    if (auto ed = dynamic_cast<AFEQAudioProcessorEditor*>(getActiveEditor()))
        juce::MessageManager::callAsync([ed, this]() { ed->responseChanged(); });
}

juce::AudioProcessorValueTreeState& AFEQAudioProcessor::getAPValueTreeState()
{
    return *state;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AFEQAudioProcessor();
}
