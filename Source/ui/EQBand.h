#pragma once

#include "JuceHeader.h"
#include "EQViewRange.h"
#include "../dsp/EqBandDsp.h"

class EQBand : juce::AudioProcessorParameter::Listener, public juce::ChangeBroadcaster
{
public:

    EQBand(EqBandDsp& eqbanddsp, const EQViewRange& eqviewrange)
        : eqBandDsp(eqbanddsp), eqViewRange(eqviewrange)
    {
    }

    const EqBandDsp& getDsp() const
    {
        return eqBandDsp;
    }

    EqBandDsp& getDsp()
    {
        return eqBandDsp;
    }

    juce::Point<float> getPosition() const
    {
        auto& bp = eqBandDsp.getBandParamsConst();
        const auto freq = bp.getFreq();
        const auto type = bp.getType();
        const auto gain = BandParams::hasGain(type) ? bp.getGain() : -3.f;

        return juce::Point<float>(eqViewRange.getXForFreq(freq), eqViewRange.getYForGain(gain));
    }

    void paint(juce::Graphics& g, bool isActive)
    {
        if (! eqBandDsp.getBandParamsConst().getEnabled())
            return;

        const auto p = getPosition();
        const auto r = 3.f * eqViewRange.scale;
        const auto rect = juce::Rectangle<float>(p.getX() - r, p.getY() - r, 2 * r, 2 * r);
        const auto col = isActive ? colour.brighter(0.5f) : colour;

        g.setColour(col.withMultipliedAlpha(0.5f));
        g.fillEllipse(rect);
        g.setColour(col);
        g.drawEllipse(rect, r / 3.f);

        juce::Rectangle<float> textRect(rect.translated(0, rect.getY() > 3.f*r ? -2.5f*r : 2.5f*r));
        g.setFont(2.f*r);
        g.drawText(juce::String(eqBandDsp.getBandIndex()), textRect, juce::Justification::centred, false);

    }

    juce::Colour colour = juce::Colour(0xFFCC2222);

private:

    void parameterValueChanged(int /*parameterIndex*/, float /*newValue*/) override
    {
        eqBandDsp.getBandParams().setChanged();
    }

    void parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/) override {}


    EqBandDsp& eqBandDsp;
    const EQViewRange& eqViewRange;
};
