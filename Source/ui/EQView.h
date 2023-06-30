#pragma once

#include "JuceHeader.h"
#include "EQBand.h"
#include "EQViewRange.h"
#include "../PluginProcessor.h"

class AFEQAudioProcessorEditor;

class EQView : public juce::Component, public juce::Timer
{
public:

    EQView(AFEQAudioProcessorEditor& afeqeditor, EqBandDspGroup& dspBands);
    void resized() override;
    void paint(juce::Graphics& g) override;

    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void timerCallback() override;

    void setScale(float newScale);

    void responseChanged();

private:

    EQBand* getNextDisabledBand();
    EQBand* getBandNearPoint(juce::Point<float> p);
    bool isBandQActive(const EQBand* band);
    bool isBandGainActive(const EQBand* band);

    void drawAnalyser(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawResponse(juce::Graphics& g, const std::vector<float>& mags);
    std::unique_ptr<juce::PopupMenu> getAnalyserMenu();
    std::unique_ptr<juce::PopupMenu> getBandMenu(EQBand* band);
    EQViewRange viewRange;

    AFEQAudioProcessorEditor& afeqEditor;
    juce::OwnedArray<EQBand> bands;
    EQBand* dragBand = nullptr;

    std::unique_ptr<juce::PopupMenu> menu;

    std::vector<float> resFreqs;
    std::vector<float> resMagsStereo;
    std::vector<float> resMagsLeft;
    std::vector<float> resMagsRight;
    std::vector<float> resMagsMid;
    std::vector<float> resMagsSide;
};