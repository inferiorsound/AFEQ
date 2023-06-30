#include "EqBandDsp.h"
#include "../AudioFilter/src/ParametricCreator.h"



EqBandDsp::EqBandDsp(int maxOrder, const FreqResponseBase& freqresbase, int index)
    : bandIndex(index), filterInstance(2, 2*((maxOrder+1)/2)), biquads(2 * ((maxOrder+1)/2)), bwCreator(2 * ((maxOrder+1)/2)), freqResBase(freqresbase)
{
    freqRes.resize(freqResBase.getNumPoints());
    bandParams.maxOrder = maxOrder;
}

void EqBandDsp::setBlockSize(int blockSize)
{
    dataMain.resize(blockSize);
    dataAux.resize(blockSize);
}

void EqBandDsp::setSampleRate(double newSampleRate)
{
    jassert(newSampleRate > 40000);
    sampleRate = newSampleRate;
}

int EqBandDsp::getBandIndex() const
{
    return bandIndex;
}

const BandParams& EqBandDsp::getBandParamsConst() const
{
    return bandParams;
}

BandParams& EqBandDsp::getBandParams()
{
    return bandParams;
}

void EqBandDsp::syncParameters()
{
    const auto newType = bandParams.getType();
    const auto paramOrder = juce::jlimit(bandParams.getMinOrderForType(newType), bandParams.getMaxOrderForType(newType), bandParams.getOrder());

    if (bandParams.getEnabled() == bandParams.enabled && bandParams.getType() == bandParams.type && 
        bandParams.getRouting() == bandParams.routing && bandParams.getFreq() == bandParams.freq &&
        bandParams.getGain() == bandParams.gain && bandParams.getQ() == bandParams.Q && paramOrder == bandParams.order)
        return;

    bandParams.setChanged();
    bandParams.enabled = bandParams.getEnabled();
    bandParams.type = bandParams.getType();
    bandParams.routing = bandParams.getRouting();
    bandParams.freq = bandParams.getFreq();
    bandParams.gain = bandParams.getGain();
    bandParams.Q = bandParams.getQ();
    bandParams.order = paramOrder;

    auto createMzti = [this](AudioFilter::FilterType type) {
        biquads.resize(1);
        AudioFilter::ParametricCreator::createMZTiStage(biquads[0], bandParams.freq, bandParams.gain, bandParams.Q, type, sampleRate);
        filterInstance.setParams(biquads[0]);
    };

    switch (bandParams.type)
    {
    case BandParams::bandPeak:
        createMzti(AudioFilter::afPeak);
        break;
    case BandParams::bandLoShelf:
        createMzti(bandParams.order == 2 ? AudioFilter::afLoShelf : AudioFilter::afLoShelf6);
        break;
    case BandParams::bandHighShelf:
        createMzti(bandParams.order == 2 ? AudioFilter::afHiShelf : AudioFilter::afHiShelf6);
        break;
    case BandParams::bandHiPass:
        createMzti(bandParams.order == 2 ? AudioFilter::afHiPass : AudioFilter::afHiPass6);
        break;
    case BandParams::bandLoPass:
        createMzti(bandParams.order == 2 ? AudioFilter::afLoPass : AudioFilter::afLoPass6);
        break;
    case BandParams::bandVOHiPass:
        AudioFilter::QBasedButterworth::createHiLoPass(biquads, bandParams.freq, true, bandParams.order, sampleRate, AudioFilter::filterMZTi);
        filterInstance.setParams(biquads);
        break;
    case BandParams::bandVOLoPass:
        AudioFilter::QBasedButterworth::createHiLoPass(biquads, bandParams.freq, false, bandParams.order, sampleRate, AudioFilter::filterMZTi);
        filterInstance.setParams(biquads);
        break;
    case BandParams::bandVOLoShelf:
        AudioFilter::QBasedButterworth::createHiLoShelf(biquads, bandParams.freq, bandParams.gain, false, bandParams.order, sampleRate, AudioFilter::filterMZTi);
        filterInstance.setParams(biquads);
        break;
    case BandParams::bandVOHiShelf:
        AudioFilter::QBasedButterworth::createHiLoShelf(biquads, bandParams.freq, bandParams.gain, true, bandParams.order, sampleRate, AudioFilter::filterMZTi);
        filterInstance.setParams(biquads);
        break;
    case BandParams::bandVOBandShelf:
        bwCreator.createBandShelf(biquads, bandParams.freq, bandParams.Q, bandParams.gain, bandParams.order, sampleRate);
        filterInstance.setParams(biquads);
        break;
    default:
        jassertfalse;
        break;
    }
}

void EqBandDsp::processBlock(double* chL, double* chR, int numSamples)
{
    jassert(dataMain.size() == dataAux.size() && dataMain.size() >= static_cast<size_t>(numSamples));

    syncParameters();
    if (bandParams.getAndClearChanged())
        updateResponse();

    if (! bandParams.getEnabled())
        return;

    const auto curRouting = bandParams.routing;
    processRoutingIn(curRouting, chL, chR, numSamples);
    
    filterInstance.processBlock(procBuffers, const_cast<const double**> (procBuffers), numSamples);
    
    processRoutingOut(curRouting, chL, chR, numSamples);
}

const std::vector<float>& EqBandDsp::getResponse() const
{
    return freqRes;
}

const FreqResponseBase& EqBandDsp::getFreqResBase() const
{
    return freqResBase;
}

void EqBandDsp::updateResponse()
{
    responseUpdateFlag = true;
    AudioFilter::Response::initGains(freqRes, freqRes.size());
    freqResBase.getResponse(biquads, freqRes, bandParams.getFreq());
}

bool EqBandDsp::getAndClearResUpdate()
{
    const auto ret = responseUpdateFlag;
    responseUpdateFlag = false;
    return ret;
}

void EqBandDsp::processRoutingIn(BandParams::Routing routing, double* chL, double* chR, int numSamples)
{
    if (chR == nullptr || bandParams.routing == BandParams::routeStereo)
    {
        procBuffers[0] = chL;
        procBuffers[1] = chR;
        return;
    }

    switch (routing)
    {
    default:
        jassertfalse;
    case BandParams::routeLeft:
        procBuffers[0] = chL;
        procBuffers[1] = nullptr;
        break;
    case BandParams::routeRight:
        procBuffers[0] = chR;
        procBuffers[1] = nullptr;
        break;
    case BandParams::routeMid:
        for (int i = 0; i < numSamples; ++i)
            dataMain[i] = 0.5 * (chL[i] + chR[i]);

        for (int i = 0; i < numSamples; ++i)
            dataAux[i] = 0.5 * (chL[i] - chR[i]);

        procBuffers[0] = dataMain.data();
        procBuffers[1] = nullptr;
        break;
    case BandParams::routeSide:
        for (int i = 0; i < numSamples; ++i)
            dataMain[i] = 0.5 * (chL[i] + chR[i]);

        for (int i = 0; i < numSamples; ++i)
            dataAux[i] = 0.5 * (chL[i] - chR[i]);

        procBuffers[0] = dataAux.data();
        procBuffers[1] = nullptr;
        break;
    }
}

void EqBandDsp::processRoutingOut(BandParams::Routing routing, double* chL, double* chR, int numSamples)
{
    if (chR == nullptr || bandParams.routing == BandParams::routeStereo)
        return;

    switch (routing)
    {
    default:
        jassertfalse;
    case BandParams::routeLeft:
        break;
    case BandParams::routeRight:
        break;
    case BandParams::routeMid:
    case BandParams::routeSide:
        for (int i = 0; i < numSamples; ++i)
            chL[i] = dataMain[i] + dataAux[i];

        for (int i = 0; i < numSamples; ++i)
            chR[i] = dataMain[i] - dataAux[i];

        break;
    }
}