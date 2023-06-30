#pragma once

#include <functional>
#include <vector>

// Simple FFT analyser that creates log-spaced bands from the FFT bins.
class FFTAnalyser
{
public:

    FFTAnalyser(int fftOrder, int overlapratio, int numbands, float releaseTime, int sampleRate);
    int getDownsamplingFactor() const;
    void clear();
    void clearPeaks();
    void processBlock(const double* inL, const double* inR, int numSamples);
    void processBlock(const float* inL, const float* inR, int numSamples);

    bool hasNewData();
    const std::vector<float>& getFreqs();
    const std::vector<float>& getMags();
    const std::vector<float>& getPeakMags();

    std::function<void(float*, int)> performFFT = nullptr;
    std::function<void(float*, int)> performDownsampling = nullptr;

private:

    void processFft();
    size_t fftSize;
    size_t overlapRatio;
    float magRel;
    std::vector<float> buffer;
    std::vector<float> window;
    std::vector<float> procBuffer;
    std::vector<float> freqs;
    std::vector<float> mags;
    std::vector<float> peakMags;
    std::vector<int> bandBorders;
    int downSamplingFactor;
    int downSamplingIndex = 0;
    size_t bufferIndex = 0;
    bool newDataAvailable = false;
};