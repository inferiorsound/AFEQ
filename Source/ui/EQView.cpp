#include "EQView.h"
#include "../PluginEditor.h"
#include "AFEQLookAndFeel.h"

EQView::EQView(AFEQAudioProcessorEditor& afeqeditor, EqBandDspGroup& dspBands)
    :afeqEditor(afeqeditor)
{
    {
        const auto& frb = dspBands[0]->getFreqResBase();
        const auto numFreqs = frb.getNumPoints();
        const auto startFreq = frb.getStartFreq();
        const auto endFreq = frb.getEndFreq();
        resFreqs = AudioFilter::Response::createLogFreqs(startFreq, endFreq, numFreqs);
        resMagsStereo.resize(numFreqs);
        resMagsLeft.resize(numFreqs);
        resMagsRight.resize(numFreqs);
        resMagsMid.resize(numFreqs);
        resMagsSide.resize(numFreqs);
    }

    setOpaque(true);

    for (auto band : dspBands)
        bands.add(std::make_unique<EQBand>(*band, viewRange));

    responseChanged();
    startTimer(100);
}

void EQView::resized()
{
    const auto w = getWidth();
    const auto h = getHeight();
    viewRange.width = w;
    viewRange.height = h;
}

void EQView::paint(juce::Graphics& g)
{
    const auto col1 = juce::Colour(0xFF444444);
    const auto col2 = juce::Colour(0xFF111111);
    const auto colResSt = findColour(AFEQLookAndFeel::responseColourStereo);
    const auto colResL = findColour(AFEQLookAndFeel::responseColourLeft);
    const auto colResR = findColour(AFEQLookAndFeel::responseColourRight);
    const auto colResM = findColour(AFEQLookAndFeel::responseColourMid);
    const auto colResS = findColour(AFEQLookAndFeel::responseColourSide);
    
    const auto h = getHeight();

    const auto& frb = bands[0]->getDsp().getFreqResBase();

    juce::ColourGradient grad(col1, 0.f, 1.f, col1, 0.f, h - 1.f, false);
    grad.addColour(0.5, col2);
    g.setGradientFill(grad);
    g.fillAll();

    drawAnalyser(g);
    drawGrid(g);

    g.setColour(colResSt);
    if (frb.resStereo.used)
        drawResponse(g, resMagsStereo);

    g.setColour(colResL);
    if (frb.resLeft.used)
        drawResponse(g, resMagsLeft);

    g.setColour(colResR);
    if (frb.resRight.used)
        drawResponse(g, resMagsRight);

    g.setColour(colResM);
    if (frb.resMid.used)
        drawResponse(g, resMagsMid);

    g.setColour(colResS);
    if (frb.resSide.used)
        drawResponse(g, resMagsSide);


    for (auto b : bands)
    {
        const auto isActive = &(b->getDsp().getBandParams()) == afeqEditor.getActiveBand();
        b->paint(g, isActive);
    }
}

void EQView::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (getBandNearPoint(e.getMouseDownPosition().toFloat()) != nullptr)
        return;

    auto newBand = getNextDisabledBand();

    if (newBand != nullptr)
    {
        newBand->getDsp().getBandParams().enabledParam->setValueNotifyingHost(1.f);
        newBand->getDsp().getBandParams().routingParam->setValueNotifyingHost(0.f);
        const auto p = e.getPosition().toFloat();
        const auto newFreq = viewRange.getFreqForX(p.getX());
        const auto newGain = viewRange.getGainForY(p.getY());
        auto fp = newBand->getDsp().getBandParams().freqParam;
        auto gp = newBand->getDsp().getBandParams().gainParam;
        const auto freqVal = fp->getNormalisableRange().convertTo0to1(newFreq);
        const auto gainVal = gp->getNormalisableRange().convertTo0to1(newGain);
        fp->beginChangeGesture();
        gp->beginChangeGesture();
        fp->setValueNotifyingHost(freqVal);
        gp->setValueNotifyingHost(gainVal);
        fp->endChangeGesture();
        gp->endChangeGesture();

        afeqEditor.setActiveBand(&(newBand->getDsp().getBandParams()));
        repaint();
    }
}

void EQView::mouseDown(const juce::MouseEvent& e)
{
    dragBand = getBandNearPoint(e.getMouseDownPosition().toFloat());

    if (dragBand != nullptr && e.mods.isLeftButtonDown())
    {
        auto& bandParams = dragBand->getDsp().getBandParams();
        bandParams.freqParam->beginChangeGesture();

        if (isBandGainActive(dragBand))
            bandParams.gainParam->beginChangeGesture();

        afeqEditor.setActiveBand(&bandParams);
    }
    else if (dragBand != nullptr && e.mods.isRightButtonDown())
    {
        menu = getBandMenu(dragBand);
        if (menu != nullptr)
        {
            juce::PopupMenu::Options options;
            options = options.withTargetScreenArea(juce::Rectangle<int>(e.getScreenPosition(), e.getScreenPosition()));
            menu->showMenuAsync(options);
        }
        dragBand = nullptr;
    }
    else if (dragBand == nullptr && e.mods.isRightButtonDown())
    {
        menu = getAnalyserMenu();
        if (menu != nullptr)
        {
            juce::PopupMenu::Options options;
            options = options.withTargetScreenArea(juce::Rectangle<int>(e.getScreenPosition(), e.getScreenPosition()));
            menu->showMenuAsync(options);
        }
    }
    else
    {
        dragBand = nullptr;
    }
}

void EQView::mouseDrag(const juce::MouseEvent& e)
{
    if (dragBand != nullptr)
    {
        const auto p = e.getPosition().toFloat();
        const auto newFreq = viewRange.getFreqForX(p.getX());
        const auto newGain = viewRange.getGainForY(p.getY());
        auto fp = dragBand->getDsp().getBandParams().freqParam;
        auto gp = dragBand->getDsp().getBandParams().gainParam;
        const auto& fpr = fp->getNormalisableRange();
        const auto& gpr = gp->getNormalisableRange();
        const auto freqVal = fpr.convertTo0to1(fpr.snapToLegalValue(newFreq));
        const auto gainVal = gpr.convertTo0to1(gpr.snapToLegalValue(newGain));
        fp->setValueNotifyingHost(freqVal);

        if (isBandGainActive(dragBand))
            gp->setValueNotifyingHost(gainVal);

        repaint();
    }
}

void EQView::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (dragBand != nullptr)
    {
        dragBand->getDsp().getBandParams().freqParam->endChangeGesture();

        if (isBandGainActive(dragBand))
            dragBand->getDsp().getBandParams().gainParam->endChangeGesture();
    }

    dragBand = nullptr;
}

void EQView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    auto band = getBandNearPoint(e.getMouseDownPosition().toFloat());
    if (band == nullptr || ! isBandQActive(band))
        return;

    const auto dir = (wheel.deltaY > 0 ? 1.f : -1.f) * (wheel.isReversed ? -1.f : 1.f);
    const auto val = e.mods.isShiftDown() ? 0.005f : 0.05f;
    auto qp = dynamic_cast<juce::AudioProcessorParameter*> (band->getDsp().getBandParams().qParam);
    qp->setValueNotifyingHost(juce::jlimit(0.f, 1.f, qp->getValue() + val * dir));
}

void EQView::timerCallback()
{
    repaint();
}

void EQView::setScale(float newScale)
{
    viewRange.scale = newScale;
}

void EQView::responseChanged()
{
    const auto& frb = bands[0]->getDsp().getFreqResBase();
    auto dspResSt = frb.resStereo;
    auto dspResL = frb.resLeft;
    auto dspResR = frb.resRight;
    auto dspResM = frb.resMid;
    auto dspResS = frb.resSide;

    jassert(dspResSt.res.size() == resMagsStereo.size() && resMagsStereo.size() == resFreqs.size());

    for (size_t i = 0; i < resMagsStereo.size(); ++i)
        resMagsStereo[i] = 10.f * std::log10(std::max(1e-8f, dspResSt.res[i]));

    if (dspResL.used)
        for (size_t i = 0; i < resMagsLeft.size(); ++i)
            resMagsLeft[i] = 10.f * std::log10(std::max(1e-8f, dspResL.res[i]));

    if (dspResR.used)
        for (size_t i = 0; i < resMagsRight.size(); ++i)
          resMagsRight[i] = 10.f * std::log10(std::max(1e-8f, dspResR.res[i]));

    if (dspResM.used)
        for (size_t i = 0; i < resMagsMid.size(); ++i)
            resMagsMid[i] = 10.f * std::log10(std::max(1e-8f, dspResM.res[i]));

    if (dspResS.used)
        for (size_t i = 0; i < resMagsSide.size(); ++i)
            resMagsSide[i] = 10.f * std::log10(std::max(1e-8f, dspResS.res[i]));

    repaint();
}

EQBand* EQView::getNextDisabledBand()
{
    for (auto b : bands)
        if (! b->getDsp().getBandParamsConst().getEnabled())
            return b;

    return nullptr;
}

EQBand* EQView::getBandNearPoint(juce::Point<float> p)
{
    auto dist = viewRange.scale * 15.f;
    EQBand* band = nullptr;

    for (auto b : bands)
    {
        if (! b->getDsp().getBandParamsConst().getEnabled())
            continue;

        const auto bandPos = b->getPosition();
        const auto bandDist = p.getDistanceFrom(bandPos);
        if (bandDist < dist)
        {
            dist = bandDist;
            band = b;
        }
    }

    return band;
}

bool EQView::isBandQActive(const EQBand* band)
{
    if (band == nullptr)
        return false;

    const auto curType = static_cast<BandParams::Type> (band->getDsp().getBandParamsConst().typeParam->getIndex());
    return BandParams::hasQFactor(curType);
}

bool EQView::isBandGainActive(const EQBand* band)
{
    if (band == nullptr)
        return false;

    const auto curType = static_cast<BandParams::Type> (band->getDsp().getBandParamsConst().typeParam->getIndex());
    return BandParams::hasGain(curType);
}

void EQView::drawAnalyser(juce::Graphics& g)
{

    const auto magCol = juce::Colour(0xFF448822);
    const auto maxCol = juce::Colour(0xFFBB0000);
    const auto analyser = afeqEditor.getAnalyser();
    if (analyser == nullptr || ! afeqEditor.isAnalyserEnabled())
        return;

    const auto& freqs = analyser->getFreqs();
    const auto& mags = analyser->getMags();
    const auto& peaks = analyser->getPeakMags();

    jassert(freqs.size() == mags.size() && freqs.size() == peaks.size());

    auto lastX = viewRange.getXForFreq(freqs[0]);
    auto yMag = viewRange.getYForAnalyserMag(mags[0]);
    auto yPeak = viewRange.getYForAnalyserMag(peaks[0]);
    const juce::Colour col(0xFFEEEEEE);
    g.setColour(maxCol);
    const auto thickness = 1.f * std::sqrt(viewRange.scale);
    juce::Path magPath;
    juce::Path maxPath;
    magPath.startNewSubPath(-thickness, yMag);
    magPath.lineTo(lastX, yMag);
    maxPath.startNewSubPath(lastX, yPeak);

    for (size_t i = 1; i < freqs.size(); ++i)
    {
        const auto x = viewRange.getXForFreq(freqs[i]);
        yMag = viewRange.getYForAnalyserMag(mags[i]);
        yPeak = viewRange.getYForAnalyserMag(peaks[i]);
        jassert(std::abs(yMag) < 100000 &&std::abs(yPeak) < 100000);
        magPath.lineTo(x, yMag);
        maxPath.lineTo(x, yPeak);

        lastX = x;
    }

    magPath.lineTo(getWidth() + thickness, yMag);
    magPath.lineTo(getWidth() + thickness, getHeight() + thickness);
    magPath.lineTo(-thickness, getHeight() + thickness);
    magPath.closeSubPath();

    g.setColour(magCol.withMultipliedAlpha(0.3f));
    g.fillPath(magPath);
    g.setColour(magCol);
    g.strokePath(magPath, juce::PathStrokeType(thickness));

    g.setColour(maxCol);
    g.strokePath(maxPath, juce::PathStrokeType(thickness));
}

void EQView::drawGrid(juce::Graphics& g)
{
    const auto gridCol = juce::Colour(0xFFBBBBBB);
    const auto& vr = viewRange;
    const auto font = juce::Font(8.f * std::sqrt(vr.scale));
    const auto textHeight = static_cast<int> (font.getHeight() * 1.3f);
    const auto textDist = static_cast<int> (3 * vr.scale);
    g.setFont(font);

    const auto gridFreqs = std::array<float, 8> {50.f, 100.f, 200.f, 500.f, 1e3f, 2e3f, 5e3f, 10e3f};
    g.setColour(gridCol);
    auto gft = [](float f) -> juce::String {
        return f < 1000 ? juce::String(static_cast<int> (f)) : juce::String(static_cast<int> (0.001f * f)) + "k";
    };
    for (auto f : gridFreqs)
    {
        const auto x = static_cast<int> (vr.getXForFreq(f) + 0.5f);
        g.drawVerticalLine(x, static_cast<float> (vr.offsetY), static_cast<float> (vr.offsetY + vr.height));
        g.drawText(gft(f), x + textDist, vr.offsetY, vr.offsetX + vr.width - x, textHeight, juce::Justification::centredLeft, false);
        g.drawText(gft(f), x + textDist, vr.offsetY + vr.height - textHeight, vr.offsetX + vr.width - x, textHeight, juce::Justification::centredLeft, false);
    }

    const auto dg = vr.gainRange.getLength() > 40 ? 6.f : vr.gainRange.getLength() > 20 ? 3.f : 2.f;
    for (auto gain = vr.gainRange.getStart() + dg; gain < vr.gainRange.getEnd() - dg / 2; gain += dg)
    {
        const auto y = static_cast<int> (vr.getYForGain(gain) + 0.5f);
        g.drawHorizontalLine(y, static_cast<float> (vr.offsetX), static_cast<float> (vr.offsetX + vr.width));
        g.drawText(juce::String(gain), vr.offsetX + textDist, y, vr.width, textHeight, juce::Justification::centredLeft, false);
        g.drawText(juce::String(gain), vr.offsetX, y, vr.width - vr.offsetX - textDist, textHeight, juce::Justification::centredRight, false);
    }

    g.setColour(gridCol);
    g.drawRect(getLocalBounds());
}

void EQView::drawResponse(juce::Graphics& g, const std::vector<float>& mags)
{
    jassert(mags.size() == resFreqs.size());

    auto lastX = viewRange.getXForFreq(resFreqs[0]);
    auto lastY = viewRange.getYForGain(mags[0]);
    const auto thickness = 1.f * std::sqrt(viewRange.scale);

    for (size_t i = 1; i < mags.size(); ++i)
    {
        auto x = viewRange.getXForFreq(resFreqs[i]);
        auto y = viewRange.getYForGain(mags[i]);

        g.drawLine(lastX, lastY, x, y, thickness);

        lastX = x;
        lastY = y;
    }
}

std::unique_ptr<juce::PopupMenu> EQView::getAnalyserMenu()
{
    auto analyser = afeqEditor.getAnalyser();
    auto& curAnProc = afeqEditor.getAudioProcessor().analyserProc;
    if (!analyser)
        return nullptr;

    std::unique_ptr<juce::PopupMenu> men = std::make_unique<juce::PopupMenu>();
    men->addItem("Clear Peaks", true, false, [this]() {
        afeqEditor.getAnalyser()->clearPeaks();
    });

    men->addSeparator();
    men->addItem("Analyser Pre EQ", true, curAnProc == AFEQAudioProcessor::kAnalyserPre, [&, this]() {
        curAnProc = curAnProc == AFEQAudioProcessor::kAnalyserDisabled ? AFEQAudioProcessor::kAnalyserPre : AFEQAudioProcessor::kAnalyserDisabled;
        afeqEditor.syncWithProcessor();
    });
    men->addItem("Analyser Post EQ", true, curAnProc == AFEQAudioProcessor::kAnalyserPost, [&, this]() {
        curAnProc = curAnProc == AFEQAudioProcessor::kAnalyserDisabled ? AFEQAudioProcessor::kAnalyserPost : AFEQAudioProcessor::kAnalyserDisabled;
        afeqEditor.syncWithProcessor();
    });

    juce::PopupMenu rangeMinMenu;
    juce::PopupMenu rangeLenMenu;

    const auto curMin = viewRange.analyserRange.getStart();
    const auto curLen = viewRange.analyserRange.getLength();

    for (float val = -40.f; val > -121.f; val -= 20)
        rangeMinMenu.addItem(juce::String(static_cast<int> (val)) + " dB", true, curMin == val, [this, val]() {
            viewRange.analyserRange = juce::Range<float>(val, val + viewRange.analyserRange.getLength());
        });

    for (float val = 40.f; val < 121.f; val += 20)
        rangeLenMenu.addItem(juce::String(static_cast<int> (val)) + " dB", true, curLen == val, [this, val]() {
            viewRange.analyserRange = juce::Range<float>(viewRange.analyserRange.getStart(), viewRange.analyserRange.getStart() + val);
        });

    men->addSeparator();
    men->addSubMenu("Analyser Range Min", rangeMinMenu);
    men->addSubMenu("Analyser Range Length", rangeLenMenu);

    return men;
}

std::unique_ptr<juce::PopupMenu> EQView::getBandMenu(EQBand* band)
{
    jassert(band != nullptr);
    auto& dsp = band->getDsp();
    auto type = dsp.getBandParams().getType();
    auto routing = dsp.getBandParams().getRouting();
    auto order = dsp.getBandParams().getOrder();

    std::unique_ptr<juce::PopupMenu> men = std::make_unique<juce::PopupMenu>();
    juce::PopupMenu menuType;
    juce::PopupMenu menuRouting;
    juce::PopupMenu menuOrder;

    for (int i = 0; i < BandParams::bandNumTypes; ++i)
    {
        auto val = (i / (static_cast<float> (BandParams::bandNumTypes) - 1.f));;
        const auto o = static_cast<BandParams::Type> (i);
        const auto name = BandParams::getGroupNameForType(o) + " " + BandParams::getNameForType(o);
        menuType.addItem(name, true, i == static_cast<int> (type),
            [&dsp, val]() {
            dsp.getBandParams().typeParam->setValueNotifyingHost(val);
        });
    }

    for (int i = 0; i < BandParams::routeNumRoutings; ++i)
    {
        auto val = (i / (static_cast<float> (BandParams::routeNumRoutings) - 1.f));
        menuRouting.addItem(BandParams::getNameForRouting(static_cast<BandParams::Routing> (i)), true, i == static_cast<int> (routing),
            [&dsp, val]() {
            dsp.getBandParams().routingParam->setValueNotifyingHost(val);
        });
    }

    for (int i = 0; i < AFEQAudioProcessor::maxOrder; ++i)
    {
        auto val = (i / (static_cast<float> (AFEQAudioProcessor::maxOrder) - 1.f));
        menuOrder.addItem(juce::String((i+1)*6) + " dB/Oct", true, i+1 == static_cast<int> (order),
            [&dsp, val]() {
            dsp.getBandParams().orderParam->setValueNotifyingHost(val);
        });
    }

    men->addSubMenu("Type", menuType, true);
    men->addSubMenu("Routing", menuRouting, true);
    men->addSubMenu("Order", menuOrder, BandParams::hasOrder(type));

    return men;
}
