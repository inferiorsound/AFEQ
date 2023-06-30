#pragma once

#include "JuceHeader.h"

class AFEQLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum ColourIDs
    {
        responseColourStereo = 0x3001000,
        responseColourLeft = 0x3001001,
        responseColourRight = 0x3001002,
        responseColourMid = 0x3001003,
        responseColourSide = 0x3001004,
    };

    AFEQLookAndFeel();
    //==============================================================================
    juce::Font getLabelFont(juce::Label&) override;

    //==============================================================================
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    //==============================================================================
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void AFEQLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    //==============================================================================
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider&) override;

    void setScale(float newScale);
    juce::Font getFont() const;

protected:
    juce::Font font;
};