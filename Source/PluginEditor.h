/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/EQView.h"
#include "ui/AFEQLookAndFeel.h"

class AFEQAudioProcessorEditor;

class RoutingSelector : public juce::ComboBox
{
public:

    void resized() override
    {
    }

    void paint(juce::Graphics& g) override
    {
        const auto w = getWidth();
        const auto h = getHeight();
        const auto wBox = w / 5;
        const auto distX = (w - 5 * wBox) / 2;

        auto lnf = dynamic_cast<AFEQLookAndFeel*> (&getLookAndFeel());
        if (lnf == nullptr)
        {
            juce::ComboBox::paint(g);
            return;
        }

        const auto font = lnf->getFont();
        const auto cornerSize = 6.f;
        g.setColour(findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(juce::Rectangle<int>(distX, 0, 5 * wBox, h).toFloat(), cornerSize);
        const auto numTexts = 5;
        const char* texts[numTexts] = { "St", "L", "R", "M", "S" };

        for (int i = 0; i < numTexts; ++i)
        {
            const auto active = i == getSelectedId() - 1;
            const auto thickness = active ? 0.5f * cornerSize : 1.f;
            juce::Rectangle<int> r(distX + i * wBox, 0, wBox, h);
            const auto rf = r.toFloat();
            g.setFont(active ? font.boldened() : font);

            juce::Path path;
            path.addRoundedRectangle(rf.getX(), rf.getY(), rf.getWidth(), rf.getHeight(),
                cornerSize, cornerSize, i == 0, i == numTexts - 1, i == 0, i == numTexts - 1);

            g.setColour(findColour(juce::ComboBox::textColourId));
            g.drawText(texts[i], r, juce::Justification::centred, false);

            g.setColour(findColour(juce::ComboBox::outlineColourId));
            g.strokePath(path, juce::PathStrokeType(thickness));
        }

    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto w = getWidth();
        const auto wBox = w / 5;
        const auto distX = (w - 5 * wBox) / 2;
        const auto index = juce::jlimit(0, 4, (e.getPosition().getX() - distX) / wBox) + 1;
        setSelectedId(index);
    }

    void mouseDrag(const juce::MouseEvent&) override {}
    void mouseUp(const juce::MouseEvent&) override {}
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}

private:
};

struct BandControls : public juce::AudioProcessorParameter::Listener
{
    BandControls(AFEQAudioProcessorEditor* parent, BandParams* bandparams);
    ~BandControls();

    void setSelected(bool selected);
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/) override {}
    void updateEnablement();
    void updateEnableColour();

    BandParams* bandParams;

    juce::TextButton select;
    juce::TextButton enabled;
    juce::Slider freq;
    juce::Slider gain;
    juce::Slider qfactor;
    juce::ComboBox type;
    RoutingSelector routing;
    juce::ComboBox order;
};


//==============================================================================
/**
*/
class AFEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AFEQAudioProcessorEditor (AFEQAudioProcessor&);
    ~AFEQAudioProcessorEditor() override;

    void setScale(float scale, bool updateSize);
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void responseChanged();
    void syncWithProcessor();
    void setActiveBand(BandParams* bc);
    BandParams* getActiveBand() const;
    juce::AudioProcessorValueTreeState& getAPValueTreeState();
    FFTAnalyser* getAnalyser() const;
    bool isAnalyserEnabled() const;
    AFEQAudioProcessor& getAudioProcessor();

    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboboxAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AFEQAudioProcessor& audioProcessor;

    EQView eqView;
    juce::OwnedArray<BandControls> bandControls;
    std::unique_ptr<BandControls> bcNoSel;
    BandControls* curBandControls = nullptr;
    AFEQLookAndFeel afeqLookAndFeel;

    juce::Image logo;
    juce::TextButton analyserPre;
    juce::TextButton analyserPost;

    juce::Rectangle<int> rectLeft;
    juce::Rectangle<int> rectRight;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AFEQAudioProcessorEditor)
};
