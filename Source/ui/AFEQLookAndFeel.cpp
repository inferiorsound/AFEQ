#include "AFEQLookAndFeel.h"


AFEQLookAndFeel::AFEQLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, findColour(juce::ComboBox::buttonColourId).withAlpha(0.5f));
    setColour(responseColourStereo, juce::Colour(0xFFCC44CC));
    setColour(responseColourLeft, juce::Colour(0xFFCC4444));
    setColour(responseColourRight, juce::Colour(0xFF4444CC));
    setColour(responseColourMid, juce::Colour(0xFF44CC44));
    setColour(responseColourSide, juce::Colour(0xFFCCCC44));
}

//==============================================================================
juce::Font AFEQLookAndFeel::getLabelFont(juce::Label&)
{
    return font;
}

//==============================================================================
void AFEQLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
    int, int, int, int, juce::ComboBox& box)
{
    auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
    juce::Rectangle<int> boxBounds(0, 0, width, height);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    juce::Rectangle<int> arrowZone(width - 23, 0, 20, height);
    juce::Path path;
    path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 3.0f);
    path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 4.0f);
    path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 3.0f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.7f : 0.1f)));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

juce::Font AFEQLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/)
{
    return font;
}

void AFEQLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(1, 1,
        box.getWidth() - 20,
        box.getHeight() - 2);

    label.setFont(getComboBoxFont(box));
}
//==============================================================================
juce::Font AFEQLookAndFeel::getTextButtonFont(juce::TextButton&, int /*buttonHeight*/)
{
    return font;
}

void AFEQLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto cornerSize = 6.0f;
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
        .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

    g.setColour(baseColour);

    auto flatOnLeft = button.isConnectedOnLeft();
    auto flatOnRight = button.isConnectedOnRight();
    auto flatOnTop = button.isConnectedOnTop();
    auto flatOnBottom = button.isConnectedOnBottom();
    const auto thickness = button.getToggleState() ? 0.5f * cornerSize : 1.f;

    if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
    {
        juce::Path path;
        path.addRoundedRectangle(bounds.getX(), bounds.getY(),
            bounds.getWidth(), bounds.getHeight(),
            cornerSize, cornerSize,
            !(flatOnLeft || flatOnTop),
            !(flatOnRight || flatOnTop),
            !(flatOnLeft || flatOnBottom),
            !(flatOnRight || flatOnBottom));

        g.fillPath(path);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.strokePath(path, juce::PathStrokeType(thickness));
    }
    else
    {
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, cornerSize, thickness);
    }
}

//==============================================================================
void AFEQLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
        bounds.getCentreY(),
        arcRadius,
        arcRadius,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true);

    const auto grad = juce::ColourGradient(outline.brighter(0.3f), bounds.getCentre(),
        outline, juce::Point<float>(bounds.getCentreX(), 0.f), true);
    g.setGradientFill(grad);
    g.fillEllipse(bounds.reduced(lineW));

    g.setColour(outline);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
            bounds.getCentreY(),
            arcRadius,
            arcRadius,
            0.0f,
            rotaryStartAngle,
            toAngle,
            true);

        g.setColour(fill);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    auto thumbWidth = lineW * 1.0f;
    auto getPoint = [](juce::Point<float> centre, float radius, float angle) {
        return juce::Point<float>(centre.getX() + radius * std::cos(angle - juce::MathConstants<float>::halfPi),
            centre.getY() + radius * std::sin(angle - juce::MathConstants<float>::halfPi));
    };

    auto thumbPoint = getPoint(bounds.getCentre(), arcRadius, toAngle);
    auto thumbStart = getPoint(bounds.getCentre(), 0.5f * arcRadius, toAngle);

    juce::Path thumbLine;
    thumbLine.startNewSubPath(thumbStart);
    thumbLine.lineTo(thumbPoint);
    juce::PathStrokeType thumbLineStroke(thumbWidth, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded);

    g.setColour(slider.findColour(juce::Slider::thumbColourId).withAlpha(0.5f));
    g.strokePath(thumbLine, thumbLineStroke);
    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

void AFEQLookAndFeel::setScale(float newScale)
{
    font = juce::Font(newScale * 9.f);
}

juce::Font AFEQLookAndFeel::getFont() const
{
    return font;
}
