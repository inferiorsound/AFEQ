# Overview

AFEQ is a parametric EQ that also works as a demo to the [AudioFilter]([GitHub - inferiorsound/AudioFilter](https://github.com/inferiorsound/AudioFilter)) library using [JUCE]([GitHub - juce-framework/JUCE: JUCE is an open-source cross-platform C++ application framework for desktop and mobile applications, including VST, VST3, AU, AUv3, RTAS and AAX audio plug-ins.](https://github.com/juce-framework/JUCE/)). It features various filter types (cuts, peak, shelves and higher order butterworth) as well as a basic FFT spectrum analyser for displaying the input or output spectrum. Each band can be processed in stereo or only left/right/mid/side.

This is a [KVR Developer Challenge 2023]([KVR Audio Developer Challenge 2023 - Free Plugins Competition](https://www.kvraudio.com/kvr-developer-challenge/2023/)) entry. Binaries can be downloaded on its kvr product page.

# Building

To build AFEQ run throug these steps:

* Clone this repository including submodules: 
  `git clone --recursive https://github.com/inferiorsound/AFEQ`
* Build Projucer from `juce/extras/Projucer` with a supported IDE of your choice
* Open `AFEQ.jucer` in ProJucer
* If needed, add an exporter for your IDE
* Select "Save Project and Open in IDE.." and build project

For the KVR entry Visual Studio 2017 was used. If the SDKs are available other plugin formats (VST2, AAX) can be added in Projucer, too.

# License

AFEQ is GPL3 licensed.
