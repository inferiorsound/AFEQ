#include "FFTAnalyser.h"
#include <cmath>
#include <algorithm>


FFTAnalyser::FFTAnalyser(int fftOrder, int overlapratio, int numbands, float releaseTime, int sampleRate)
    :fftSize(static_cast<size_t>(1) << fftOrder), overlapRatio(overlapratio), buffer(fftSize), window(fftSize), procBuffer(fftSize),
    freqs(numbands), mags(numbands), peakMags(numbands), bandBorders(numbands), downSamplingFactor(std::max(1, sampleRate / 44100))
{
    sampleRate /= downSamplingFactor;

    {
        const auto blockRate = static_cast<float> (sampleRate * overlapRatio) / fftSize;
        magRel = 1.f - std::exp(-1.f / (releaseTime * blockRate));
    }

    for (size_t i = 0; i < window.size(); ++i)
        window[i] = 0.5f - 0.5f * std::cos(2 * 3.14159265358979323846f * i / window.size());

    for (int i = 0; i < numbands; ++i)
        freqs[i] = 20.f * std::pow(10.f, 3.f * i / (numbands - 1.f));

    {
        int lastIdx = 1;
        float lastFreq = freqs[0] * freqs[0] / freqs[1];

        for (int i = 0; i < numbands - 1; ++i)
        {
            auto nextFreq = std::sqrt(freqs[i] * freqs[i + 1]);
            const auto nextIdx = static_cast<int> (fftSize * nextFreq / sampleRate + 0.5f);

            if (nextIdx >= lastIdx)
            {
                bandBorders[i] = nextIdx;
            }
            else
            {
                bandBorders[i] = lastIdx;
                nextFreq = sampleRate * (bandBorders[i]) / static_cast<float> (fftSize);
                freqs[i] = std::sqrt(nextFreq * lastFreq);
            }

            lastIdx = bandBorders[i] + 1;
            lastFreq = freqs[i];
        }

        const auto nextFreq = freqs[numbands - 1] * freqs[numbands - 1] / std::sqrt(freqs[numbands - 1] * freqs[numbands - 2]);
        const auto nextIdx = std::min(static_cast<int> (fftSize / 2 - 1), static_cast<int> (fftSize * nextFreq / sampleRate + 0.5f));
        bandBorders[numbands-1] = nextIdx;
    }

    clear();
}

int FFTAnalyser::getDownsamplingFactor() const
{
    return downSamplingFactor;
}

void FFTAnalyser::clear()
{
    bufferIndex = 0;
    downSamplingIndex = 0;
    newDataAvailable = false;

    for (auto& m : mags)
        m = 0.f;

    clearPeaks();
}

void FFTAnalyser::clearPeaks()
{
    for (auto& m : peakMags)
        m = 0.f;
}

void FFTAnalyser::processBlock(const double* inL, const double* inR, int numSamples)
{
    const auto blockSize = 128;
    float tempBuffer[blockSize];

    while (numSamples > 0)
    {
        const auto curNumSamples = std::min(numSamples, blockSize);
        numSamples -= curNumSamples;

        if (inR == nullptr)
        {
            for (int i = 0; i < curNumSamples; ++i)
                tempBuffer[i] = static_cast<float> (inL[i]);
        }
        else
        {
            for (int i = 0; i < curNumSamples; ++i)
                tempBuffer[i] = static_cast<float> (0.5 * (inL[i] + inR[i]));

            inR += curNumSamples;
        }

        processBlock(tempBuffer, nullptr, curNumSamples);
        inL += curNumSamples;
    }
}

void FFTAnalyser::processBlock(const float* inL, const float* inR, int numSamples)
{
    while (numSamples > 0)
    {
        const auto curNumSamples = std::min(numSamples, static_cast<int> (procBuffer.size()));

        if (inR != nullptr)
        {
            for (int i = 0; i < curNumSamples; ++i)
                procBuffer[i] = 0.5f * (inL[i] + inR[i]);
        }
        else
        {
            memcpy(procBuffer.data(), inL, numSamples * sizeof(float));
        }

        if (downSamplingFactor == 1)
        {
            for (int i = 0; i < curNumSamples; ++i)
            {
                buffer[bufferIndex++] = procBuffer[i];

                if (bufferIndex >= buffer.size())
                {
                    processFft();
                    bufferIndex = buffer.size() - buffer.size() / overlapRatio;
                }
            }
        }
        else
        {
            if (performDownsampling != nullptr)
                performDownsampling(procBuffer.data(), curNumSamples);

            while (downSamplingIndex < curNumSamples)
            {
                buffer[bufferIndex++] = procBuffer[downSamplingIndex];
                downSamplingIndex += downSamplingFactor;

                if (bufferIndex >= buffer.size())
                {
                    processFft();
                    bufferIndex = buffer.size() - buffer.size() / overlapRatio;
                }
            }

            downSamplingIndex -= curNumSamples;
        }

        inL += curNumSamples;
        if (inR != nullptr)
            inR += curNumSamples;

        numSamples -= curNumSamples;
    }
}

bool FFTAnalyser::hasNewData()
{
    const auto retVal = newDataAvailable;
    newDataAvailable = false;
    return retVal;
}

const std::vector<float>& FFTAnalyser::getFreqs()
{
    return freqs;
}

const std::vector<float>& FFTAnalyser::getMags()
{
    return mags;
}

const std::vector<float>& FFTAnalyser::getPeakMags()
{
    return peakMags;
}

void FFTAnalyser::processFft()
{
    for (size_t i = 0; i < buffer.size(); ++i)
        procBuffer[i] = buffer[i] * window[i];

    const auto sampleOverlap = buffer.size() / overlapRatio;
    const auto numSamplesToCopy = buffer.size() - sampleOverlap;
    for (size_t i = 0; i < numSamplesToCopy; ++i)
        buffer[i] = buffer[i + sampleOverlap];

    if (performFFT != nullptr)
    {
        performFFT(procBuffer.data(), static_cast<int> (procBuffer.size()));

        auto lastIdx = 1;
        for (size_t i = 0; i < bandBorders.size(); ++i)
        {
            float val = 0;

            for (int n = lastIdx; n <= bandBorders[i]; ++n)
            {
                const auto re = procBuffer[2 * n + 0];
                const auto im = procBuffer[2 * n + 1];
                val += std::sqrt(re * re + im * im);
            }

            const auto gain = std::sqrt(freqs[i] / 1000.f);
            const auto curMag = gain * val / (bandBorders[i] + 1 - lastIdx);
            mags[i] = curMag > mags[i] ? curMag : mags[i] + (curMag - mags[i])*magRel;
            peakMags[i] = std::max(mags[i], peakMags[i]);
            lastIdx = bandBorders[i] + 1;
        }

        newDataAvailable = true;
    }
}

