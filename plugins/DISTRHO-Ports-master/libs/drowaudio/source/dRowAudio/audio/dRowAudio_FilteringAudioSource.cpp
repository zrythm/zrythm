/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/



namespace
{
    // modelled on a Pioneer DJM-800 mixer
    const float defaultSettings[FilteringAudioSource::numFilters][FilteringAudioSource::numFilterSettings] = {
        {70,   0.25f},
        {1000,  0.25f},
        {13000,  0.25f}
    };
}

//========================================================================
FilteringAudioSource::FilteringAudioSource (AudioSource* inputSource,
                                            bool deleteInputWhenDeleted)
    : input         (inputSource, deleteInputWhenDeleted),
	  sampleRate    (44100.0),
	  filterSource  (true)
{
    jassert (input != nullptr);
    
    gains[Low] = 1.0f;
    gains[Mid] = 1.0f;
    gains[High] = 1.0f;

	// configure the filters
    resetFilters();
}

FilteringAudioSource::~FilteringAudioSource()
{
    releaseResources();
}

//==============================================================================
void FilteringAudioSource::setGain (FilterType setting, float newGain)
{
    switch (setting)
    {
        case Low:
        {
            gains[Low] = newGain;
            const IIRCoefficients lowCoeff (IIRCoefficients::makeLowShelf       (sampleRate, defaultSettings[Low][CF], defaultSettings[Low][Q], gains[Low]));
            filter[0][Low].setCoefficients (lowCoeff);
            filter[1][Low].setCoefficients (lowCoeff);
            break;
        }
        case Mid:
        {
            gains[Mid] = newGain;
            const IIRCoefficients midCoeff (IIRCoefficients::makePeakFilter     (sampleRate, defaultSettings[Mid][CF], defaultSettings[Mid][Q], gains[Mid]));
            filter[0][Mid].setCoefficients (midCoeff);
            filter[1][Mid].setCoefficients (midCoeff);
            break;
        }
        case High:
        {
            gains[High] = newGain;
            const IIRCoefficients highCoeff (IIRCoefficients::makeHighShelf     (sampleRate, defaultSettings[High][CF], defaultSettings[High][Q], gains[High]));
            filter[0][High].setCoefficients (highCoeff);
            filter[1][High].setCoefficients (highCoeff);
            break;
        }
        default:
            break;
    }
}

void FilteringAudioSource::setFilterSource (bool shouldFilter)
{
    filterSource = shouldFilter;
}

//==============================================================================
void FilteringAudioSource::prepareToPlay (int samplesPerBlockExpected,
                                          double sampleRate_)
{
    sampleRate = sampleRate_;

    resetFilters();
    
    if (input != nullptr)
        input->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void FilteringAudioSource::releaseResources()
{
    if (input != nullptr)
        input->releaseResources();
}

void FilteringAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
    input->getNextAudioBlock (info);

    if (filterSource && info.buffer->getNumChannels() > 0)
    {
        const int bufferNumSamples = info.numSamples;
        float* sampleDataL = info.buffer->getWritePointer (0, info.startSample);
        
        filter[0][Low].processSamples   (sampleDataL, bufferNumSamples);
        filter[0][Mid].processSamples   (sampleDataL, bufferNumSamples);
        filter[0][High].processSamples  (sampleDataL, bufferNumSamples);

        if (info.buffer->getNumChannels() > 1)
        {
            float* sampleDataR = info.buffer->getWritePointer (1, info.startSample);

            filter[1][Low].processSamples   (sampleDataR, bufferNumSamples);
            filter[1][Mid].processSamples   (sampleDataR, bufferNumSamples);
            filter[1][High].processSamples  (sampleDataR, bufferNumSamples);
        }
    }
}

void FilteringAudioSource::resetFilters()
{
    const IIRCoefficients lowCoeff (IIRCoefficients::makeLowShelf       (sampleRate, defaultSettings[Low][CF], defaultSettings[Low][Q], gains[Low]));
    const IIRCoefficients midCoeff (IIRCoefficients::makePeakFilter     (sampleRate, defaultSettings[Mid][CF], defaultSettings[Mid][Q], gains[Mid]));
    const IIRCoefficients highCoeff (IIRCoefficients::makeHighShelf     (sampleRate, defaultSettings[High][CF], defaultSettings[High][Q], gains[High]));

    filter[0][Low].setCoefficients (lowCoeff);
    filter[1][Low].setCoefficients (lowCoeff);
    filter[0][Mid].setCoefficients (midCoeff);
    filter[1][Mid].setCoefficients (midCoeff);
    filter[0][High].setCoefficients (highCoeff);
    filter[1][High].setCoefficients (highCoeff);
}

