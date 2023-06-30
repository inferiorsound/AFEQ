#pragma once

#include "JuceHeader.h"

struct EQViewRange
{
    juce::Range<float> freqRange = { 20.f, 20e3f };
    juce::Range<float> gainRange = { -24.f, 24.f };
    juce::Range<float> analyserRange = { -80.f, 0.f };
    int width = 0;
    int height = 0;
    int offsetX = 0;
    int offsetY = 0;
    float scale = 1.f;

    float getXForFreq(float freq) const
    {
        return offsetX + width * std::log(freq / freqRange.getStart()) / std::log(freqRange.getEnd() / freqRange.getStart());
    }
    
    float getFreqForX(float x) const
    {
        return freqRange.getStart() * std::exp((x - offsetX) / width * std::log(freqRange.getEnd() / freqRange.getStart()));
    }
    
    float getYForGain(float gain) const
    {
        return offsetY + height * (1.f - (gain - gainRange.getStart()) / gainRange.getLength());
    }
    
    float getGainForY(float y) const
    {
        return gainRange.getStart() + gainRange.getLength() * (1.f - (y - offsetY) / height);
    }

    float getYForAnalyserMag(float mag) const
    {
        mag = juce::jlimit(1e-8f, 1e3f, mag);
        mag = 20.f * std::log10(mag);
        return offsetY + height * (1.f - (mag - analyserRange.getStart()) / analyserRange.getLength());
    }
};
