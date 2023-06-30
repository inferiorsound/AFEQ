/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
BandControls::BandControls(AFEQAudioProcessorEditor* parent, BandParams* bandparams)
    : bandParams(bandparams)
{
    auto& apvts = parent->getAPValueTreeState();
    freq.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    qfactor.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    enabled.setClickingTogglesState(true);
    enabled.setButtonText("Enabled");

    routing.addItemList(BandParams::getRoutingNames(), 1);
    type.addItemList(BandParams::getTypeNames(), 1);

    if (bandParams != nullptr)
    {
        for (int i = 1; i <= bandParams->maxOrder; ++i)
            order.addItem(juce::String(6 * i) + " dB/oct", i);

        parent->addAndMakeVisible(select);
    }
    else
    {
        order.addItem("12 dB/Oct", 1);
    }

    parent->addChildComponent(enabled);
    parent->addChildComponent(type);
    parent->addChildComponent(routing);
    parent->addChildComponent(freq);
    parent->addChildComponent(gain);
    parent->addChildComponent(qfactor);
    parent->addChildComponent(order);

    if (bandParams != nullptr)
    {
        parent->comboboxAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, bandParams->typeId.toString(), type));
        parent->comboboxAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, bandParams->routingId.toString(), routing));
        parent->comboboxAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, bandParams->orderId.toString(), order));
        parent->buttonAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, bandParams->enableId.toString(), enabled));
        parent->sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, bandParams->freqId.toString(), freq));
        parent->sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, bandParams->gainId.toString(), gain));
        parent->sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, bandParams->qId.toString(), qfactor));

        bandParams->typeParam->addListener(this);
        bandParams->enabledParam->addListener(this);
        bandParams->routingParam->addListener(this);
        select.onClick = [parent, this]() { parent->setActiveBand(bandParams); };
    }
    else
    {
        auto freqRange = juce::NormalisableRange<double>(20., 20e3,
            [](double s, double e, double v) { return juce::jlimit(s, e, s * std::exp(std::log(e / s) * v)); },
            [](double s, double e, double v) { return juce::jlimit(0., 1., std::log(v / s) / std::log(e / s)); },
            [](double s, double e, double v) { v = juce::jlimit(s, e, v); return v < 1000 ? 0.1f * std::round(10.f * v) : std::round(v); });
        auto qRange = juce::NormalisableRange<double>(0.1f, 10.f,
            [](double s, double e, double v) { return juce::jlimit(s, e, s * std::exp(std::log(e / s) * v)); },
            [](double s, double e, double v) { return juce::jlimit(0., 1., std::log(v / s) / std::log(e / s)); },
            [](double s, double e, double v) { v = juce::jlimit(s, e, v); return 0.01f * std::round(100.f * v); });
        auto freqToStr = [](double v) {
            return v < 1e3f ? juce::String(static_cast<int> (v)) : juce::String(v*0.001f, 2) + "k";
        };

        enabled.setEnabled(false);
        type.setEnabled(false);
        routing.setEnabled(false);
        freq.setEnabled(false);
        freq.textFromValueFunction = freqToStr;
        gain.setEnabled(false);
        qfactor.setEnabled(false);
        qfactor.textFromValueFunction = [](double v) { return juce::String(v, 3); };
        order.setEnabled(false);
        freq.setNormalisableRange(freqRange);
        freq.setValue(1000, juce::dontSendNotification);
        gain.setRange(-24., 24., 0.1);
        gain.setValue(0, juce::dontSendNotification);
        qfactor.setNormalisableRange(qRange);
        qfactor.setValue(0.707, juce::dontSendNotification);
        routing.setSelectedId(1, juce::dontSendNotification);
        order.setSelectedId(1, juce::dontSendNotification);
        type.setSelectedId(1, juce::dontSendNotification);
    }
}

BandControls::~BandControls()
{
    if (bandParams != nullptr)
    {
        bandParams->typeParam->removeListener(this);
        bandParams->enabledParam->removeListener(this);
        bandParams->routingParam->removeListener(this);
    }
}

void BandControls::setSelected(bool selected)
{
    enabled.setVisible(selected);
    freq.setVisible(selected);
    gain.setVisible(selected);
    qfactor.setVisible(selected);
    type.setVisible(selected);
    routing.setVisible(selected);
    order.setVisible(selected);
    select.setToggleState(selected, juce::dontSendNotification);
    updateEnablement();
}

void BandControls::parameterValueChanged(int parameterIndex, float /*newValue*/)
{
    const bool isType = (parameterIndex % 7) == 1;
    const bool isEnabled = (parameterIndex % 7) == 0;
    const bool isRouting = (parameterIndex % 7) == 2;
    if (isType)
        updateEnablement();

    if (isEnabled || isRouting)
        updateEnableColour();
}

void BandControls::updateEnablement()
{
    if (bandParams == nullptr)
        return;

    const auto curType = static_cast<BandParams::Type> (bandParams->typeParam->getIndex());
    const auto hasGain = BandParams::hasGain(curType);
    const auto hasQFactor = BandParams::hasQFactor(curType);
    const auto hasOrder = BandParams::hasOrder(curType);

    gain.setEnabled(hasGain);
    qfactor.setEnabled(hasQFactor);
    order.setEnabled(hasOrder);
}

void BandControls::updateEnableColour()
{
    auto& lnf = enabled.getLookAndFeel();
    const auto rt = bandParams->routingParam->getIndex();
    const auto colId = rt == 1 ? AFEQLookAndFeel::responseColourLeft
        : rt == 2 ? AFEQLookAndFeel::responseColourRight
        : rt == 3 ? AFEQLookAndFeel::responseColourMid
        : rt == 4 ? AFEQLookAndFeel::responseColourSide
        : AFEQLookAndFeel::responseColourStereo;
    const auto en = bandParams->enabledParam->get();
    const auto col = lnf.isColourSpecified(colId) ? lnf.findColour(colId).withAlpha(en ? 0.3f : 0.f) : juce::Colours::transparentBlack;

    select.setColour(juce::TextButton::buttonColourId, lnf.findColour(juce::TextButton::buttonColourId).overlaidWith(col));
    select.setColour(juce::TextButton::buttonOnColourId, lnf.findColour(juce::TextButton::buttonOnColourId).overlaidWith(col));
}

//==============================================================================
AFEQAudioProcessorEditor::AFEQAudioProcessorEditor (AFEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), eqView(*this, p.eqBands)
    , analyserPre("PRE"), analyserPost("POST")
{
    setLookAndFeel(&afeqLookAndFeel);
    setResizable(false, true);

    addAndMakeVisible(eqView);
    addAndMakeVisible(analyserPre);
    addAndMakeVisible(analyserPost);
    analyserPre.setConnectedEdges(juce::TextButton::ConnectedOnRight);
    analyserPost.setConnectedEdges(juce::TextButton::ConnectedOnLeft);
    analyserPre.setClickingTogglesState(true);
    analyserPost.setClickingTogglesState(true);
    analyserPre.onClick = [this]() {
        audioProcessor.analyserProc = analyserPre.getToggleState() ? AFEQAudioProcessor::kAnalyserPre : AFEQAudioProcessor::kAnalyserDisabled;
        analyserPost.setToggleState(false, juce::dontSendNotification);
    };
    analyserPost.onClick = [this]() {
        audioProcessor.analyserProc = analyserPost.getToggleState() ? AFEQAudioProcessor::kAnalyserPost : AFEQAudioProcessor::kAnalyserDisabled;
        analyserPre.setToggleState(false, juce::dontSendNotification);
    };

    for (auto b : audioProcessor.eqBands)
    {
        auto bc = bandControls.add(std::make_unique<BandControls>(this, &(b->getBandParams())));
        bc->select.setButtonText(juce::String(bandControls.size()));
    }

    bcNoSel = std::make_unique<BandControls>(this, nullptr);
    if (auto c = getConstrainer())
        c->setFixedAspectRatio(4.f / 3.f);

    setScale(p.guiScale, false);
    setResizeLimits(400, 300, 4000, 3000);
    setActiveBand(nullptr);
}

AFEQAudioProcessorEditor::~AFEQAudioProcessorEditor()
{
    setLookAndFeel(nullptr);

    comboboxAttachments.clear();
    sliderAttachments.clear();
    buttonAttachments.clear();
}

void AFEQAudioProcessorEditor::setScale(float scale, bool updateSize)
{
    afeqLookAndFeel.setScale(scale);
    audioProcessor.guiScale = scale;
    eqView.setScale(scale);

    if (updateSize)
        setSize(static_cast<int> (400.f * scale), static_cast<int> (300.f * scale));
}

//==============================================================================
void AFEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.drawImageAt(logo, eqView.getX(), (eqView.getY() - logo.getHeight()) / 2);
    const auto font = afeqLookAndFeel.getFont();
    g.setFont(font);

    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        const auto version = juce::String("  Version ") + ProjectInfo::versionString;
        const auto versionRect = juce::Rectangle<int>(eqView.getX() + logo.getWidth(),
            (eqView.getY() - logo.getHeight()) / 2,
            font.getStringWidth(version), logo.getHeight());
        g.drawText(version, versionRect, juce::Justification::bottomLeft, false);
    }

    if (auto b = curBandControls)
    {
        const auto col = b->order.findColour(juce::ComboBox::textColourId);
        g.setColour(col);
        const auto textHeight = static_cast<int> (1.1f * font.getHeight());
        g.drawText("Gain", b->gain.getX(), b->gain.getY() - textHeight, b->gain.getWidth(), textHeight, juce::Justification::centred, false);
        g.drawText("Freq", b->freq.getX(), b->freq.getY() - textHeight, b->freq.getWidth(), textHeight, juce::Justification::centred, false);
        g.drawText("Q", b->qfactor.getX(), b->qfactor.getY() - textHeight, b->qfactor.getWidth(), textHeight, juce::Justification::centred, false);

        g.drawText("Band Selection", rectLeft.getX(), rectLeft.getY() - textHeight, rectLeft.getWidth(), textHeight, juce::Justification::centred, false);
        g.drawText("Band Settings", rectRight.getX(), rectRight.getY() - textHeight, rectRight.getWidth(), textHeight, juce::Justification::centred, false);
    }    

    g.drawText("Analyser  ", 0, analyserPre.getY(), analyserPre.getX(), analyserPre.getHeight(), juce::Justification::bottomRight, false);

    const auto scale = audioProcessor.guiScale;
    const auto rad = 5.f * scale;
    const auto thickness = 1.f * scale;
    g.setColour(juce::Colours::black.withAlpha(0.1f));
    g.fillRoundedRectangle(rectLeft.toFloat(), rad);
    g.fillRoundedRectangle(rectRight.toFloat(), rad);
    g.setColour(juce::Colours::lightgrey.withAlpha(0.2f));
    g.drawRoundedRectangle(rectLeft.toFloat(), rad, thickness);
    g.drawRoundedRectangle(rectRight.toFloat(), rad, thickness);
}

void AFEQAudioProcessorEditor::resized()
{
    const auto w = getWidth();
    const auto h = getHeight();
    setScale(w / 400.f, false);
    const auto scale = audioProcessor.guiScale;

    const auto distX = static_cast<int> (scale * 10);
    const auto distTop = static_cast<int> (scale * 20);
    const auto distBot = static_cast<int> (scale * 80);
    const auto textBoxHeight = static_cast<int> (scale * 12);

    eqView.setBounds(distX, distTop, w - 2 * distX, h - distTop - distBot);

    {
        auto baseLogo = juce::ImageCache::getFromMemory(BinaryData::afeq_logo_png, BinaryData::afeq_logo_pngSize);
        const auto logoHeight = eqView.getY() * 8 / 10;
        const auto logoWidth = logoHeight * baseLogo.getWidth() / baseLogo.getHeight();
        logo = baseLogo.rescaled(logoWidth, logoHeight);
    }

    {
        const auto wAnBut = static_cast<int> (scale * 30);
        const auto y = (eqView.getY() + logo.getHeight()) / 2 - textBoxHeight;
        analyserPost.setBounds(eqView.getRight() - wAnBut, y, wAnBut, textBoxHeight);
        analyserPre.setBounds(analyserPost.getX() - wAnBut, y, wAnBut, textBoxHeight);
    }

    {
        const auto wFreq = 2 * static_cast<int> (scale * 25);
        const auto wQg = static_cast<int> (scale * 40);
        const auto dist = static_cast<int> (scale * 5);
        const auto yBot = h - dist;
        const auto xFreq = (w - wFreq) / 2;
        const auto yFreq = yBot - wFreq - textBoxHeight;
        const auto xGain = xFreq - 2 * dist - wQg;
        const auto xQ = xFreq + wFreq + 2 * dist;
        const auto yQG = yBot - wQg - textBoxHeight;
        const auto wCbBut = static_cast<int> (scale * 75);
        const auto xSelect = xGain - 5 * dist - wCbBut;
        const auto xBandSet = xQ + wQg + 5 * dist;
        const auto yEnabled = yFreq;
        const auto yOrder = yBot - textBoxHeight;
        const auto yType = yOrder - dist/2 - textBoxHeight;
        const auto yRouting = yType - dist / 2 - textBoxHeight;

        const auto selDist = (yBot - yFreq - 4 * textBoxHeight) / 3;
        const auto wSel = (wCbBut - 2 * dist) / 3;
        auto index = 0;
        const auto numBands = bandControls.size();

        rectLeft = juce::Rectangle<int>(xSelect, yFreq, wCbBut, yOrder + textBoxHeight - yFreq).expanded(textBoxHeight, textBoxHeight/4);
        rectRight = juce::Rectangle<int>(xBandSet, yFreq, wCbBut, yOrder + textBoxHeight - yFreq).expanded(textBoxHeight, textBoxHeight/4);

        auto setControlBounds = [&](BandControls* b)
        {
            b->freq.setBounds(xFreq, yFreq, wFreq, wFreq + textBoxHeight);
            b->freq.setTextBoxStyle(juce::Slider::TextBoxBelow, false, wFreq, textBoxHeight);
            b->gain.setBounds(xGain, yQG, wQg, wQg + textBoxHeight);
            b->gain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, wQg, textBoxHeight);
            b->qfactor.setBounds(xQ, yQG, wQg, wQg + textBoxHeight);
            b->qfactor.setTextBoxStyle(juce::Slider::TextBoxBelow, false, wQg, textBoxHeight);
            b->enabled.setBounds(xBandSet, yEnabled, wCbBut, textBoxHeight);
            b->type.setBounds(xBandSet, yType, wCbBut, textBoxHeight);
            b->routing.setBounds(xBandSet, yRouting, wCbBut, textBoxHeight);
            b->order.setBounds(xBandSet, yOrder, wCbBut, textBoxHeight);

            const auto xSel = xSelect + (index % (numBands / 4)) * (wSel + dist);
            const auto ySel = index < numBands / 2 ? yFreq + (index / (numBands / 4)) * (textBoxHeight + selDist)
                : yBot - textBoxHeight - (3 - index / (numBands / 4)) * (textBoxHeight + selDist);
            b->select.setBounds(xSel, ySel, wSel, textBoxHeight);
            ++index;
        };

        for (auto b : bandControls)
            setControlBounds(b);

        if (bcNoSel != nullptr)
            setControlBounds(bcNoSel.get());
    }
}

void AFEQAudioProcessorEditor::responseChanged()
{
    eqView.responseChanged();
}

void AFEQAudioProcessorEditor::syncWithProcessor()
{
    setScale(audioProcessor.guiScale, true);
    analyserPre.setToggleState(audioProcessor.analyserProc == AFEQAudioProcessor::kAnalyserPre, juce::dontSendNotification);
    analyserPost.setToggleState(audioProcessor.analyserProc == AFEQAudioProcessor::kAnalyserPost, juce::dontSendNotification);
}

void AFEQAudioProcessorEditor::setActiveBand(BandParams* bc)
{
    curBandControls = bcNoSel.get();

    for (int i = 0; i < bandControls.size(); ++i)
    {
        const auto isSelected = bc == bandControls[i]->bandParams;
        bandControls[i]->setSelected(isSelected);

        if (isSelected)
            curBandControls = bandControls[i];
    }

    bcNoSel->setSelected(curBandControls == bcNoSel.get());

    repaint();
    resized();
}

BandParams* AFEQAudioProcessorEditor::getActiveBand() const
{
    return curBandControls != nullptr ? curBandControls->bandParams : nullptr;
}

juce::AudioProcessorValueTreeState& AFEQAudioProcessorEditor::getAPValueTreeState()
{
    return audioProcessor.getAPValueTreeState();
}

FFTAnalyser* AFEQAudioProcessorEditor::getAnalyser() const
{
    return  audioProcessor.fftAnalyser.get();
}
bool AFEQAudioProcessorEditor::isAnalyserEnabled() const
{
    return audioProcessor.analyserProc != AFEQAudioProcessor::kAnalyserDisabled;
}

AFEQAudioProcessor& AFEQAudioProcessorEditor::getAudioProcessor()
{
    return audioProcessor;
}


