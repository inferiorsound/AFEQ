#pragma once

#include "../AudioFilter/src/FilterInstance.h"
#include "../AudioFilter/src/ButterworthCreator.h"
#include "../AudioFilter/src/Response.h"

#include "JuceHeader.h"

struct BandParams
{
    enum Group
    {
        bandMZTi,
        bandButterworth,

        bandNumGroups
    };

    enum Type
    {
        // Single biquad
        bandPeak,
        bandLoShelf,
        bandHighShelf,
        bandHiPass,
        bandLoPass,
       
        // Variable order
        bandVOHiPass,
        bandVOLoPass,
        bandVOLoShelf,
        bandVOHiShelf,
        bandVOBandShelf,

        bandNumTypes
    };

    enum Routing
    {
        routeStereo,
        routeLeft,
        routeRight,
        routeMid,
        routeSide,

        routeNumRoutings
    };

    static juce::String getNameForType(Type t)
    {
        switch (t)
        {
            case bandPeak:
                return "Peak";
            case bandLoShelf:
                return "Low Shelf";
            case bandHighShelf:
                return "High Shelf";
            case bandHiPass:
                return "Low Cut";
            case bandLoPass:
                return "High Cut";
            case bandVOHiPass:
                return "Low Cut";
            case bandVOLoPass:
                return "High Cut";
            case bandVOLoShelf:
                return "Low Shelf";
            case bandVOHiShelf:
                return "High Shelf";
            case bandVOBandShelf:
                return "Band Shelf";
            default:
                jassertfalse;
                break;
        }
        return "";
    }

    static juce::String getNameForRouting(Routing r)
    {
        switch (r)
        {
        default:
            jassertfalse;
        case routeStereo:
            return "Normal";
        case routeLeft:
            return "Left";
        case routeRight:
            return "Right";
        case routeMid:
            return "Mid";
        case routeSide:
            return "Side";
        }
    }

    static juce::StringArray getTypeNames()
    {
        juce::StringArray ret;
        for (int i = 0; i < bandNumTypes; ++i)
        {
            const auto t = static_cast<Type> (i);
            const auto g = getGroupForType(t);
            const auto grp = g == bandButterworth ? "BW " : "";
            ret.add(grp + getNameForType(t));
        }
        return ret;
    }

    static juce::StringArray getRoutingNames()
    {
        juce::StringArray ret;
        for (int i = 0; i < routeNumRoutings; ++i)
            ret.add(getNameForRouting(static_cast<Routing> (i)));
        return ret;
    }

    static Type getTypeForName(const juce::String& name)
    {
        const auto n2 = name.toLowerCase().replace(" ", "");
        for (int i = 0; i < bandNumTypes; ++i)
            if (getNameForType(static_cast<Type> (i)).toLowerCase().replace(" ", "") == n2)
                return static_cast<Type> (i);

        return bandPeak;
    }

    static Group getGroupForType(Type t)
    {
        switch (t)
        {
        case bandPeak:
        case bandLoShelf:
        case bandHighShelf:
        case bandHiPass:
        case bandLoPass:
            return bandMZTi;
        case bandVOHiPass:
        case bandVOLoPass:
        case bandVOLoShelf:
        case bandVOHiShelf:
        case bandVOBandShelf:
            return bandButterworth;
        default:
            jassertfalse;
            break;
        }
        return bandMZTi;
    }

    static juce::String getGroupNameForType(Type t)
    {
        const auto group = getGroupForType(t);
        switch (group)
        {
        case bandMZTi:
            return "Basic";
        case bandButterworth:
            return "Variable Order";
        default:
            jassertfalse;
            break;
        }
        return "";
    }

    static bool hasGain(Type t)
    {
        switch (t)
        {
        default:
            jassertfalse;
        case bandPeak:
        case bandLoShelf:
        case bandHighShelf:
        case bandVOLoShelf:
        case bandVOHiShelf:
        case bandVOBandShelf:
            return true;
        case bandHiPass:
        case bandLoPass:
        case bandVOHiPass:
        case bandVOLoPass:
            return false;
        }
    }

    static bool hasQFactor(Type t)
    {
        switch (t)
        {
        default:
            jassertfalse;
        case bandPeak:
        case bandLoShelf:
        case bandHighShelf:
        case bandHiPass:
        case bandLoPass:
        case bandVOBandShelf:
            return true;
        case bandVOLoShelf:
        case bandVOHiShelf:
        case bandVOHiPass:
        case bandVOLoPass:
            return false;
        }
    }

    static bool hasOrder(Type t)
    {
        switch (t)
        {
        default:
            jassertfalse;
        case bandVOLoShelf:
        case bandVOHiShelf:
        case bandVOHiPass:
        case bandVOLoPass:
        case bandVOBandShelf:
            return true;
        case bandPeak:
        case bandLoShelf:
        case bandHighShelf:
        case bandHiPass:
        case bandLoPass:
            return false;
        }
    }
    int getMinOrderForType(Type t)
    {
        return t == bandPeak ? 2 : 1;
    }

    int getMaxOrderForType(Type t)
    {
        const auto group = getGroupForType(t);
        return group == bandMZTi ? 2 : maxOrder;
    }

    bool enabled = false;
    Type type = bandPeak;
    Routing routing = routeStereo;
    float freq = 1000.f;
    float gain = 0.f;
    float Q = std::sqrt(0.5f);
    int order = 2;

    bool getEnabled() const
    {
        jassert(enabledParam != nullptr);
        return enabledParam->get();
    }

    Type getType() const
    {
        jassert(typeParam != nullptr);
        return static_cast<Type> (typeParam->getIndex());
    }

    Routing getRouting() const
    {
        jassert(routingParam != nullptr);
        return static_cast<Routing> (routingParam->getIndex());
    }

    float getFreq() const
    {
        jassert(freqParam != nullptr);
        return freqParam->get();
    }

    float getGain() const
    {
        jassert(gainParam != nullptr);
        return gainParam->get();
    }

    float getQ() const
    {
        jassert(qParam != nullptr);
        return qParam->get();
    }

    int getOrder() const
    {
        jassert(orderParam != nullptr);
        return orderParam->get();
    }

    int maxOrder = 8;

    void setBandId(const juce::String& newBandId)
    {
        jassert(bandId.isEmpty());
        bandId = newBandId;
        enableId = bandId + " Enabled";
        typeId = bandId + " Type";
        routingId = bandId + " Routing";
        freqId = bandId + " Freq";
        gainId = bandId + " Gain";
        qId = bandId + " Q";
        orderId = bandId + " Order";
    }

    juce::String getBandId() const
    {
        return bandId;
    }
    juce::Identifier enableId;
    juce::Identifier typeId;
    juce::Identifier routingId;
    juce::Identifier freqId;
    juce::Identifier gainId;
    juce::Identifier qId;
    juce::Identifier orderId;

    juce::AudioParameterBool* enabledParam = nullptr;
    juce::AudioParameterChoice* typeParam = nullptr;
    juce::AudioParameterChoice* routingParam = nullptr;
    juce::AudioParameterFloat* freqParam = nullptr;
    juce::AudioParameterFloat* gainParam = nullptr;
    juce::AudioParameterFloat* qParam = nullptr;
    juce::AudioParameterInt* orderParam = nullptr;

    void syncParameters(juce::AudioProcessorValueTreeState& apvst)
    {
        enabledParam = dynamic_cast<juce::AudioParameterBool*> (apvst.getParameter(enableId));
        typeParam = dynamic_cast<juce::AudioParameterChoice*> (apvst.getParameter(typeId));
        routingParam = dynamic_cast<juce::AudioParameterChoice*> (apvst.getParameter(routingId));
        freqParam = dynamic_cast<juce::AudioParameterFloat*> (apvst.getParameter(freqId));
        gainParam = dynamic_cast<juce::AudioParameterFloat*> (apvst.getParameter(gainId));
        qParam = dynamic_cast<juce::AudioParameterFloat*> (apvst.getParameter(qId));
        orderParam = dynamic_cast<juce::AudioParameterInt*> (apvst.getParameter(orderId));
    }

    void setChanged()
    {
        changedFlag = true;
    }

    bool getAndClearChanged()
    {
        const auto ret = changedFlag;
        changedFlag = false;
        return ret;
    }

private:
    juce::String bandId;
    bool changedFlag = true;
};

struct FreqResponseBase : AudioFilter::Response::ResponseBase
{
    struct ResponseData
    {
        ResponseData(int size) : res(size) {}
        bool used = false;
        std::vector<float> res;
    };

    FreqResponseBase(int numfreqs, float startfreq, float endfreq)
        :ResponseBase(numfreqs, startfreq, endfreq), resStereo(numfreqs),
        resLeft(numfreqs), resRight(numfreqs), resMid(numfreqs), resSide(numfreqs)
    {
    }

    ResponseData resStereo;
    ResponseData resLeft;
    ResponseData resRight;
    ResponseData resMid;
    ResponseData resSide;
};

class EqBandDsp
{
public:

    EqBandDsp(int maxOrder, const FreqResponseBase& freqresbase, int index);
    void setBlockSize(int blockSize);
    void setSampleRate(double newSampleRate);
    int getBandIndex() const; 

    const BandParams& getBandParamsConst() const;
    BandParams& getBandParams();
    void syncParameters();

    void processBlock(double* chL, double* chR, int numSamples);
    const std::vector<float>& getResponse() const;
    const FreqResponseBase& getFreqResBase() const;
    void updateResponse();
    bool getAndClearResUpdate();

private:

    void processRoutingIn(BandParams::Routing routing, double* chL, double* chR, int numSamples);
    void processRoutingOut(BandParams::Routing routing, double* chL, double* chR, int numSamples);
    BandParams bandParams;
    double sampleRate;
    int bandIndex;

    std::vector<double> dataMain;
    std::vector<double> dataAux;
    double* procBuffers[2] = { nullptr, nullptr };
    AudioFilter::FilterInstance<double> filterInstance;
    AudioFilter::BiquadParamCascade biquads;
    AudioFilter::ButterworthCreator bwCreator;
    const FreqResponseBase& freqResBase;
    std::vector<float> freqRes;
    bool responseUpdateFlag = false;
};

using EqBandDspGroup = juce::OwnedArray<EqBandDsp>;
