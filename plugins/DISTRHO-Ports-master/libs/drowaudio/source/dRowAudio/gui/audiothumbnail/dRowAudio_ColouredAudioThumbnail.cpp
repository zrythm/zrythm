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

#undef min
#undef max

using ::std::numeric_limits;

//==============================================================================
namespace
{
    //==============================================================================
    template <typename SampleType>
    static void getStereoMinAndMax (SampleType* const* channels, const int numChannels, const int numSamples,
                                    SampleType& lmin, SampleType& lmax, SampleType& rmin, SampleType& rmax)
    {
        SampleType bufMin, bufMax;
        findMinAndMax (channels[0], numSamples, bufMin, bufMax);
        lmax = jmax (lmax, bufMax);
        lmin = jmin (lmin, bufMin);
        
        if (numChannels > 1)
        {
            findMinAndMax (channels[1], numSamples, bufMin, bufMax);
            rmax = jmax (rmax, bufMax);
            rmin = jmin (rmin, bufMin);
        }
        else
        {
            rmax = lmax;
            rmin = lmin;
        }
    }
}

//==============================================================================
struct ColouredAudioThumbnail::MinMaxColourValue
{
    char minValue;
    char maxValue;
	Colour colour; //drow

    MinMaxColourValue() : minValue (0), maxValue (0), colour(Colour::fromHSV(0.0, 1.0f, 1.0f, 1.0f))
    {
    }

    inline void set (const char newMin, const char newMax) throw()
    {
        minValue = newMin;
        maxValue = newMax;
    }

    inline void setFloat (const float newMin, const float newMax) throw()
    {
        minValue = (char) jlimit (-128, 127, roundFloatToInt (newMin * 127.0f));
        maxValue = (char) jlimit (-128, 127, roundFloatToInt (newMax * 127.0f));

        if (maxValue == minValue)
            maxValue = (char) jmin (127, maxValue + 1);
    }

	inline void setColour(const Colour& newColour) throw()
	{
		colour = newColour.withBrightness(1.0f);
	}
	
    inline char getMinValue() const noexcept        { return minValue; }
    inline char getMaxValue() const noexcept        { return maxValue; }

    inline bool isNonZero() const throw()
    {
        return maxValue > minValue;
    }

    inline int getPeak() const throw()
    {
        return jmax (::std::abs ((int) minValue),
                     ::std::abs ((int) maxValue));
    }

    inline void read (InputStream& input)
    {
		//DBG("read from MinMaxColourValue");
        minValue = input.readByte();
        maxValue = input.readByte();
		input.read ((void*)&colour, sizeof(Colour));
    }

    inline void write (OutputStream& output)
    {
        output.writeByte (minValue);
        output.writeByte (maxValue);
		output.write ((void*)&colour, sizeof(Colour));
    }
};

//==============================================================================
class ColouredAudioThumbnail::LevelDataSource   :	public TimeSliceClient,
													public Timer
{
public:
    LevelDataSource (ColouredAudioThumbnail& owner_, AudioFormatReader* newReader, int64 hash)
        : lengthInSamples (0), numSamplesFinished (0), sampleRate (0), numChannels (0),
          hashCode (hash), owner (owner_), reader (newReader),
          tempSampleBufferSize (0), tempFilteredBufferSize (0)
    {
    }

    LevelDataSource (ColouredAudioThumbnail& owner_, InputSource* source_)
        : lengthInSamples (0), numSamplesFinished (0), sampleRate (0), numChannels (0),
          hashCode (source_->hashCode()), owner (owner_), source (source_),
          tempSampleBufferSize (0), tempFilteredBufferSize (0)
    {
    }

    ~LevelDataSource()
    {
        owner.cache.getTimeSliceThread().removeTimeSliceClient (this);
    }

    enum { timeBeforeDeletingReader = 2000 };

    void initialise (int64 numSamplesFinished_)
    {
        const ScopedLock sl (readerLock);

        numSamplesFinished = numSamplesFinished_;

        createReader();

        if (reader != 0)
        {
            lengthInSamples = reader->lengthInSamples;
            numChannels = reader->numChannels;
            sampleRate = reader->sampleRate;

			filterLow.makeBandPass (reader->sampleRate, 130.0, 2);
			filterLowMid.makeBandPass (reader->sampleRate, 650.0, 2.0);
			filterHighMid.makeBandPass (reader->sampleRate, 1300.0, 2.0);
			filterHigh.makeHighPass (reader->sampleRate, 2700.0, 0.5);
			
            filterLow.reset();
			filterLowMid.reset();
			filterHighMid.reset();
			filterHigh.reset();

            if (lengthInSamples <= 0)
                reader = 0;
            else if (! isFullyLoaded())
                owner.cache.getTimeSliceThread().addTimeSliceClient (this);
        }
    }

    void getLevels (int64 startSample, int numSamples, Array<float>& levels, Array<Colour>& colours)
    {
        const ScopedLock sl (readerLock);
        createReader();
        
        if (reader != 0)
        {
            float l[4] = { 0 };
			Colour colourLeft, colourRight;
            
            readMaxLevelsFilteringWithColour (startSample, numSamples,
											  l[0], l[1], l[2], l[3], colourLeft, colourRight);
            levels.clearQuick();
            levels.addArray ((const float*) l, 4);

            //*** not finding colour here
            colours.clearQuick();
            colours.add (colourLeft);
            colours.add (colourRight);
        }
    }
	
    void releaseResources()
    {
        const ScopedLock sl (readerLock);
        reader = 0;
    }

    int useTimeSlice()
    {
        if (isFullyLoaded())
        {
            if (reader != 0 && source != 0)
                startTimer (timeBeforeDeletingReader);

            owner.cache.getTimeSliceThread().removeTimeSliceClient (this);
            return false;
        }

        stopTimer();

        bool justFinished = false;

        {
            const ScopedLock sl (readerLock);

            createReader();

            if (reader != 0)
            {
                if (! readNextBlock())
                    return true;

                justFinished = true;
            }
        }

        if (justFinished)
            owner.cache.storeThumb (owner, hashCode);

        return false;
    }

    void timerCallback()
    {
        stopTimer();
        releaseResources();
    }

    bool isFullyLoaded() const throw()
    {
        return numSamplesFinished >= lengthInSamples;
    }

    inline int sampleToThumbSample (const int64 originalSample) const throw()
    {
        return (int) (originalSample / owner.samplesPerThumbSample);
    }

    int64 lengthInSamples, numSamplesFinished;
    double sampleRate;
    int numChannels;
    int64 hashCode;

private:
    ColouredAudioThumbnail& owner;
    ScopedPointer <InputSource> source;
    ScopedPointer <AudioFormatReader> reader;
    CriticalSection readerLock;
    BiquadFilter filterLow, filterLowMid, filterHighMid, filterHigh;
    HeapBlock<int> tempSampleBuffer, tempFilteredBuffer;
    int tempSampleBufferSize, tempFilteredBufferSize;

    void createReader()
    {
        if (reader == 0 && source != 0)
        {
            InputStream* audioFileStream = source->createInputStream();

            if (audioFileStream != 0)
                reader = owner.formatManagerToUse.createReaderFor (audioFileStream);
        }
    }

    bool readNextBlock()
    {
        jassert (reader != 0);

        if (! isFullyLoaded())
        {
            const int numToDo = (int) jmin (256 * (int64) owner.samplesPerThumbSample, lengthInSamples - numSamplesFinished);

            if (numToDo > 0)
            {
                int64 startSample = numSamplesFinished;

                const int firstThumbIndex = sampleToThumbSample (startSample);
                const int lastThumbIndex  = sampleToThumbSample (startSample + numToDo);
                const int numThumbSamps = lastThumbIndex - firstThumbIndex;

                HeapBlock<MinMaxColourValue> levelData (numThumbSamps * 2);
                MinMaxColourValue* levels[2] = { levelData, levelData + numThumbSamps };

                for (int i = 0; i < numThumbSamps; ++i)
                {
                    float lowestLeft, highestLeft, lowestRight, highestRight;
					Colour colourLeft, colourRight;
					
                    readMaxLevelsFilteringWithColour ((firstThumbIndex + i) * owner.samplesPerThumbSample, owner.samplesPerThumbSample,
													  lowestLeft, highestLeft, lowestRight, highestRight,
													  colourLeft, colourRight);
					
                    levels[0][i].setFloat (lowestLeft, highestLeft);
                    levels[1][i].setFloat (lowestRight, highestRight);
					
					levels[0][i].setColour(colourLeft);
					levels[1][i].setColour(colourRight);
                }

                {
                    const ScopedUnlock su (readerLock);
                    owner.setLevels (levels, firstThumbIndex, 2, numThumbSamps);
                }

                numSamplesFinished += numToDo;
            }
        }

        return isFullyLoaded();
    }
    
    void readMaxLevelsFilteringWithColour (int64 startSampleInFile,
                                           int64 numSamples,
                                           float& lowestLeft, float& highestLeft,
                                           float& lowestRight, float& highestRight,
                                           Colour &colourLeft, Colour &colourRight)
    {
        if (numSamples <= 0)
        {
            lowestLeft = 0;
            lowestRight = 0;
            highestLeft = 0;
            highestRight = 0;
            
            colourLeft = Colours::white;
            colourRight = Colours::white;
            
            return;
        }
        
        const int bufferSize = (int) jmin (numSamples, (int64) 4096);
        const int newTempSampleBufferSize = bufferSize * 2 + 64;

        if (tempSampleBufferSize < newTempSampleBufferSize)
        {
            tempSampleBuffer.malloc (newTempSampleBufferSize);
            tempSampleBufferSize = newTempSampleBufferSize;
        }
        
        int* tempSpace = tempSampleBuffer.getData();
        int* tempBuffer[3] = {&tempSpace[0],
                                &tempSpace[bufferSize],
                                nullptr};
        
        const int filteredBlockSize = bufferSize * 4;
        if (tempFilteredBufferSize < filteredBlockSize)
        {
            tempFilteredBuffer.malloc (filteredBlockSize);
            tempFilteredBufferSize = filteredBlockSize;
        }
        
        int* filteredBlock = tempFilteredBuffer.getData();
        int* filteredArray[4] = {&filteredBlock[0],
                                &filteredBlock[bufferSize],
                                &filteredBlock[bufferSize * 2],
                                &filteredBlock[bufferSize * 3]};
        
        float avgLow = 0.0f, avgMid = 0.0f, avgHigh = 0.0f;
        
        if (reader->usesFloatingPointData)
        {
            float lmin = std::numeric_limits<float>::max();
            float lmax = -lmin;
            float rmin = lmin;
            float rmax = lmax;
            
            while (numSamples > 0)
            {
                const int numToDo = (int) jmin (numSamples, (int64) bufferSize);
                if (! reader->read (tempBuffer, 2, startSampleInFile, numToDo, false))
                    break;
                
                // copy samples to buffers ready to be filtered
                memcpy (filteredArray[0], tempBuffer[0], sizeof (int) * numToDo);
                memcpy (filteredArray[1], tempBuffer[0], sizeof (int) * numToDo);
                memcpy (filteredArray[2], tempBuffer[0], sizeof (int) * numToDo);
                memcpy (filteredArray[3], tempBuffer[0], sizeof (int) * numToDo);
                
                // filter buffers
                filterLow.processSamples (reinterpret_cast<float*> (filteredArray[0]), numToDo);
                filterLowMid.processSamples (reinterpret_cast<float*> (filteredArray[1]), numToDo);
                filterHighMid.processSamples (reinterpret_cast<float*> (filteredArray[2]), numToDo);
                filterHigh.processSamples (reinterpret_cast<float*> (filteredArray[3]), numToDo);
                
                // calculate colour
                for (int i = 0; i < numToDo; i++)
                {
                    float low = fabsf ((reinterpret_cast<float*> (filteredArray[0]))[i]);
                    float mid = fabsf ((reinterpret_cast<float*> (filteredArray[1]))[i])
                                + fabsf ((reinterpret_cast<float*> (filteredArray[2]))[i]);
                    float high = fabsf ((reinterpret_cast<float*> (filteredArray[3]))[i]);
                    
                    if (low > avgLow)   avgLow = low;
                    if (mid > avgMid)   avgMid = mid;
                    if (high > avgHigh) avgHigh = high;
                }
                
                numSamples -= numToDo;
                startSampleInFile += numToDo;

                getStereoMinAndMax (reinterpret_cast<float**> (&tempBuffer[0]), reader->numChannels, numToDo,
                                    lmin, lmax, rmin, rmax);
            }
            
            lowestLeft = lmin;
            highestLeft = lmax;
            lowestRight = rmin;
            highestRight = rmax;
        }
        else
        {
            int lmax = std::numeric_limits<int>::min();
            int lmin = std::numeric_limits<int>::max();
            int rmax = std::numeric_limits<int>::min();
            int rmin = std::numeric_limits<int>::max();

            while (numSamples > 0)
            {
                const int numToDo = (int) jmin (numSamples, (int64) bufferSize);
                if (! reader->read (tempBuffer, 2, startSampleInFile, numToDo, false))
                    break;
                
                // copy samples to buffers ready to be filtered
                memcpy (filteredArray[0], tempBuffer[0], sizeof (int) * numToDo);
                memcpy (filteredArray[1], tempBuffer[0], sizeof (int) * numToDo);
                memcpy (filteredArray[2], tempBuffer[0], sizeof (int) * numToDo);
                memcpy (filteredArray[3], tempBuffer[0], sizeof (int) * numToDo);
                
                // filter buffers
                filterLow.processSamples ((filteredArray[0]), numToDo);
                filterLowMid.processSamples ((filteredArray[1]), numToDo);
                filterHighMid.processSamples ((filteredArray[2]), numToDo);
                filterHigh.processSamples ((filteredArray[3]), numToDo);
                
                // calculate colour
                for (int i = 0; i < numToDo; i++)
                {
                    int low = abs (filteredArray[0][i]);
                    int mid = abs (filteredArray[1][i]) + abs (filteredArray[2][i]);
                    int high = abs (filteredArray[3][i]);
                    
                    if (low > avgLow)   avgLow = (float) low;
                    if (mid > avgMid)   avgMid = (float) mid;
                    if (high > avgHigh) avgHigh = (float) high;
                }
                
                numSamples -= numToDo;
                startSampleInFile += numToDo;

                getStereoMinAndMax (reinterpret_cast<int**> (&tempBuffer[0]), reader->numChannels, numToDo,
                                    lmin, lmax, rmin, rmax);
            }
            
            lowestLeft = lmin / (float) std::numeric_limits<int>::max();
            highestLeft = lmax / (float) std::numeric_limits<int>::max();
            lowestRight = rmin / (float) std::numeric_limits<int>::max();
            highestRight = rmax / (float) std::numeric_limits<int>::max();
            
            avgLow = avgLow / (float) ::std::numeric_limits<int>::max();
            avgMid = avgMid / (float) ::std::numeric_limits<int>::max();
            avgHigh = avgHigh / (float) ::std::numeric_limits<int>::max();
        }
        
        uint8 maxSize = ::std::numeric_limits<uint8>::max();
        colourLeft = Colour::fromRGB ((uint8) (avgLow * maxSize),
                                      (uint8) (avgMid * maxSize * 0.66f),
                                      (uint8) (avgHigh * maxSize * 0.33f));
        colourRight = colourLeft;
    }
};

//==============================================================================
/*	Holds the data for 1 cache sample and the methods required to rescale those.
 */
class ColouredAudioThumbnail::ThumbData
{
public:
    ThumbData (const int numThumbSamples)
        : peakLevel (-1)
    {
        ensureSize (numThumbSamples);
    }

    inline MinMaxColourValue* getData (const int thumbSampleIndex) throw()
    {
        jassert (thumbSampleIndex < data.size());
        return data.getRawDataPointer() + thumbSampleIndex;
    }

    int getSize() const throw()
    {
        return data.size();
    }

    void getMinMax (int startSample, int endSample, MinMaxColourValue& result) const noexcept
    {
        if (startSample >= 0)
        {
            endSample = jmin (endSample, data.size() - 1);

            char mx = -128;
            char mn = 127;

            while (startSample <= endSample)
            {
                const MinMaxColourValue& v = data.getReference (startSample);

                if (v.minValue < mn)  mn = v.minValue;
                if (v.maxValue > mx)  mx = v.maxValue;

                ++startSample;
            }

            if (mn <= mx)
            {
                result.set (mn, mx);
                return;
            }
        }

        result.set (1, 0);
    }

	void getColour (int startSample, int endSample,  MinMaxColourValue& result) throw()
	{
		const int numSamples = endSample - startSample;

		uint8 red = 0, green = 0, blue = 0;

		if (startSample >= 0)
		{
			endSample = jmin (endSample, data.size() - 1);

			while (startSample <= endSample)
			{
				const MinMaxColourValue& v = data.getReference (startSample);

				if (numSamples == 1)
				{
					result.colour = v.colour;
					return;
				}

				if (v.colour.getRed() > red) {
					red = v.colour.getRed();
				}
				if (v.colour.getGreen() > green) {
					green = v.colour.getGreen();
				}
				if (v.colour.getBlue() > blue) {
					blue = v.colour.getBlue();
				}
				
				++startSample;
			}
		}

		result.colour = Colour(red, green, blue);
	}

    void write (const MinMaxColourValue* const source, const int startIndex, const int numValues)
    {
        resetPeak();

        if (startIndex + numValues > data.size())
            ensureSize (startIndex + numValues);

        MinMaxColourValue* const dest = getData (startIndex);

        for (int i = 0; i < numValues; ++i)
            dest[i] = source[i];
    }

    void resetPeak()
    {
        peakLevel = -1;
    }

    int getPeak()
    {
        if (peakLevel < 0)
        {
            for (int i = 0; i < data.size(); ++i)
            {
                const int peak = data[i].getPeak();
                if (peak > peakLevel)
                    peakLevel = peak;
            }
        }

        return peakLevel;
    }
	
private:
    Array <MinMaxColourValue> data;
    int peakLevel;

    void ensureSize (const int thumbSamples)
    {
        const int extraNeeded = thumbSamples - data.size();
        if (extraNeeded > 0)
            data.insertMultiple (-1, MinMaxColourValue(), extraNeeded);
    }
};

//==============================================================================
class ColouredAudioThumbnail::CachedWindow
{
public:
    CachedWindow()
        : cachedStart (0), cachedTimePerPixel (0),
          numChannelsCached (0), numSamplesCached (0),
          cacheNeedsRefilling (true)
    {
    }

    void invalidate()
    {
        cacheNeedsRefilling = true;
    }

    void drawChannel (Graphics& g, const Rectangle<int>& area,
                      const double startTime, const double endTime,
                      const int channelNum, const float verticalZoomFactor,
                      const double sampleRate, const int numChannels, const int samplesPerThumbSample,
                      LevelDataSource* levelData, const OwnedArray<ThumbData>& channels)
    {
        refillCache (area.getWidth(), startTime, endTime, sampleRate,
                     numChannels, samplesPerThumbSample, levelData, channels);

        if (isPositiveAndBelow (channelNum, numChannelsCached))
        {
            const Rectangle<int> clip (g.getClipBounds().getIntersection (area.withWidth (jmin (numSamplesCached, area.getWidth()))));

            if (! clip.isEmpty())
            {
                const float topY = (float) area.getY();
                const float bottomY = (float) area.getBottom();
                const float midY = (topY + bottomY) * 0.5f;
                const float vscale = verticalZoomFactor * (bottomY - topY) / 256.0f;

                const MinMaxColourValue* cacheData = getData (channelNum, clip.getX() - area.getX());

                int x = clip.getX();
                for (int w = clip.getWidth(); --w >= 0;)
                {
                    if (cacheData->isNonZero())
                        g.drawVerticalLine (x, jmax (midY - cacheData->maxValue * vscale - 0.3f, topY),
                                               jmin (midY - cacheData->minValue * vscale + 0.3f, bottomY));

                    ++x;
                    ++cacheData;
                }
            }
        }
    }

	void drawColouredChannel (Graphics& g, const Rectangle<int>& area,
							  const double startTime, const double endTime,
							  const int channelNum, const float verticalZoomFactor,
							  const double sampleRate, const int numChannels, const int samplesPerThumbSample,
							  LevelDataSource* levelData, const OwnedArray<ThumbData>& channels)
    {
        refillCache (area.getWidth(), startTime, endTime, sampleRate,
                     numChannels, samplesPerThumbSample, levelData, channels);
		
        if (isPositiveAndBelow (channelNum, numChannelsCached))
        {
            const Rectangle<int> clip (g.getClipBounds().getIntersection (area.withWidth (jmin (numSamplesCached, area.getWidth()))));
			
            if (! clip.isEmpty())
            {
                const float topY = (float) area.getY();
                const float bottomY = (float) area.getBottom();
                const float midY = (topY + bottomY) * 0.5f;
                const float vscale = verticalZoomFactor * (bottomY - topY) / 256.0f;
				
                const MinMaxColourValue* cacheData = getData (channelNum, clip.getX() - area.getX());
				
                int x = clip.getX();
                for (int w = clip.getWidth(); --w >= 0;)
                {
                    if (cacheData->isNonZero())
					{
						// set colour of line //drow
						g.setColour (cacheData->colour);
                        g.drawVerticalLine (x, jmax (midY - cacheData->maxValue * vscale - 0.3f, topY),
											jmin (midY - cacheData->minValue * vscale + 0.3f, bottomY));
					}
					
                    ++x;
                    ++cacheData;
                }
            }
        }
    }
	
private:
    Array <MinMaxColourValue> data;
    double cachedStart, cachedTimePerPixel;
    int numChannelsCached, numSamplesCached;
    bool cacheNeedsRefilling;

    void refillCache (const int numSamples, double startTime, const double endTime,
                      const double sampleRate, const int numChannels, const int samplesPerThumbSample,
                      LevelDataSource* levelData, const OwnedArray<ThumbData>& channels)
    {
        const double timePerPixel = (endTime - startTime) / numSamples;

        if (numSamples <= 0 || timePerPixel <= 0.0 || sampleRate <= 0)
        {
            invalidate();
            return;
        }

        if (numSamples == numSamplesCached
             && numChannelsCached == numChannels
             && startTime == cachedStart
             && timePerPixel == cachedTimePerPixel
             && ! cacheNeedsRefilling)
        {
            return;
        }

        numSamplesCached = numSamples;
        numChannelsCached = numChannels;
        cachedStart = startTime;
        cachedTimePerPixel = timePerPixel;
        cacheNeedsRefilling = false;

        ensureSize (numSamples);

        if (timePerPixel * sampleRate <= samplesPerThumbSample && levelData != 0)
        {
            int sample = roundToInt (startTime * sampleRate);
            Array<float> levels;
			Array<Colour> colours;

            int i;
            for (i = 0; i < numSamples; ++i)
            {
                const int nextSample = roundToInt ((startTime + timePerPixel) * sampleRate);

                if (sample >= 0)
                {
                    if (sample >= levelData->lengthInSamples)
                        break;

                    levelData->getLevels (sample, jmax (1, nextSample - sample), levels, colours);

                    const int numChans = jmin (levels.size() / 2, numChannelsCached);

                    for (int chan = 0; chan < numChans; ++chan) 
					{
                        getData (chan, i)->setFloat (levels.getUnchecked (chan * 2),
                                                     levels.getUnchecked (chan * 2 + 1));
						getData (chan, i)->setColour(colours.getUnchecked(chan));
					}
                }

                startTime += timePerPixel;
                sample = nextSample;
            }

            numSamplesCached = i;
        }
        else
        {
            jassert (channels.size() == numChannelsCached);
			
            for (int channelNum = 0; channelNum < numChannelsCached; ++channelNum)
            {
                ThumbData* channelData = channels.getUnchecked (channelNum);
                MinMaxColourValue* cacheData = getData (channelNum, 0);

                const double timeToThumbSampleFactor = sampleRate / (double) samplesPerThumbSample;

                startTime = cachedStart;
                int sample = roundToInt (startTime * timeToThumbSampleFactor);

                for (int i = numSamples; --i >= 0;)
                {
                    const int nextSample = roundToInt ((startTime + timePerPixel) * timeToThumbSampleFactor);

                    channelData->getMinMax (sample, nextSample, *cacheData);
					channelData->getColour(sample, nextSample, *cacheData);

                    ++cacheData;
                    startTime += timePerPixel;
                    sample = nextSample;
                }
            }
        }
    }

    MinMaxColourValue* getData (const int channelNum, const int cacheIndex) throw()
    {
        jassert (isPositiveAndBelow (channelNum, numChannelsCached) && isPositiveAndBelow (cacheIndex, data.size()));

        return data.getRawDataPointer() + channelNum * numSamplesCached
                                        + cacheIndex;
    }
	
    void ensureSize (const int numSamples)
    {
        const int itemsRequired = numSamples * numChannelsCached;

        if (data.size() < itemsRequired)
            data.insertMultiple (-1, MinMaxColourValue(), itemsRequired - data.size());
    }
};

//==============================================================================
ColouredAudioThumbnail::ColouredAudioThumbnail (const int originalSamplesPerThumbnailSample,
												AudioFormatManager& formatManagerToUse_,
												AudioThumbnailCache& cacheToUse)
:	formatManagerToUse (formatManagerToUse_),
    cache (cacheToUse),
    window (new CachedWindow()),
    samplesPerThumbSample (originalSamplesPerThumbnailSample),
    totalSamples (0),
    numChannels (0),
    sampleRate (0)
{
}

ColouredAudioThumbnail::~ColouredAudioThumbnail()
{
    clear();
}

void ColouredAudioThumbnail::clear()
{
    source = 0;

    const ScopedLock sl (lock);
    window->invalidate();
    channels.clear();
    totalSamples = numSamplesFinished = 0;
    numChannels = 0;
    sampleRate = 0;

    sendChangeMessage();
}

void ColouredAudioThumbnail::reset (int newNumChannels, double newSampleRate, int64 totalSamplesInSource)
{
    clear();

    numChannels = newNumChannels;
    sampleRate = newSampleRate;
    totalSamples = totalSamplesInSource;

    createChannels (1 + (int) (totalSamplesInSource / samplesPerThumbSample));
}

void ColouredAudioThumbnail::createChannels (const int length)
{
    while (channels.size() < numChannels)
        channels.add (new ThumbData (length));
}

//==============================================================================
bool ColouredAudioThumbnail::loadFrom (InputStream& input)
{
    clear();

    if (input.readByte() != 'j' || input.readByte() != 'a' || input.readByte() != 't' || input.readByte() != 'm')
        return false;

    samplesPerThumbSample = input.readInt();
    totalSamples = input.readInt64();             // Total number of source samples.
    numSamplesFinished = input.readInt64();       // Number of valid source samples that have been read into the thumbnail.
    int32 numThumbnailSamples = input.readInt();  // Number of samples in the thumbnail data.
    numChannels = input.readInt();                // Number of audio channels.
    sampleRate = input.readInt();                 // Source sample rate.
    input.skipNextBytes (16);                     // reserved area

    createChannels (numThumbnailSamples);

    for (int i = 0; i < numThumbnailSamples; ++i)
        for (int chan = 0; chan < numChannels; ++chan)
            channels.getUnchecked(chan)->getData(i)->read (input);
    
    return true;
}

void ColouredAudioThumbnail::saveTo (OutputStream& output) const
{
    const ScopedLock sl (lock);

    const int numThumbnailSamples = channels.size() == 0 ? 0 : channels.getUnchecked(0)->getSize();

    output.write ("jatm", 4);
    output.writeInt (samplesPerThumbSample);
    output.writeInt64 (totalSamples);
    output.writeInt64 (numSamplesFinished);
    output.writeInt (numThumbnailSamples);
    output.writeInt (numChannels);
    output.writeInt ((int) sampleRate);
    output.writeInt64 (0);
    output.writeInt64 (0);

    for (int i = 0; i < numThumbnailSamples; ++i)
        for (int chan = 0; chan < numChannels; ++chan)
            channels.getUnchecked(chan)->getData(i)->write (output);
}

//==============================================================================
bool ColouredAudioThumbnail::setDataSource (LevelDataSource* newSource)
{
    jassert (MessageManager::getInstance()->currentThreadHasLockedMessageManager());

    numSamplesFinished = 0;

    if (cache.loadThumb (*this, newSource->hashCode) && isFullyLoaded())
    {
        source = newSource; // (make sure this isn't done before loadThumb is called)

        source->lengthInSamples = totalSamples;
        source->sampleRate = sampleRate;
        source->numChannels = numChannels;
        source->numSamplesFinished = numSamplesFinished;
    }
    else
    {
        source = newSource; // (make sure this isn't done before loadThumb is called)

        const ScopedLock sl (lock);
        source->initialise (numSamplesFinished);

        totalSamples = source->lengthInSamples;
        sampleRate = source->sampleRate;
        numChannels = source->numChannels;

        createChannels (1 + (int) (totalSamples / samplesPerThumbSample));
    }

    return sampleRate > 0 && totalSamples > 0;
}

bool ColouredAudioThumbnail::setSource (InputSource* const newSource)
{
    clear();

    return newSource != 0 && setDataSource (new LevelDataSource (*this, newSource));
}

void ColouredAudioThumbnail::setReader (AudioFormatReader* newReader, int64 hash)
{
    clear();

    if (newReader != 0)
        setDataSource (new LevelDataSource (*this, newReader, hash));
}

int64 ColouredAudioThumbnail::getHashCode() const
{
    return source == 0 ? 0 : source->hashCode;
}

void ColouredAudioThumbnail::addBlock (const int64 startSample, const AudioSampleBuffer& incoming,
									   int startOffsetInBuffer, int numSamples)
{
    jassert (startSample >= 0);

    const int firstThumbIndex = (int) (startSample / samplesPerThumbSample);
    const int lastThumbIndex  = (int) ((startSample + numSamples + (samplesPerThumbSample - 1)) / samplesPerThumbSample);
    const int numToDo = lastThumbIndex - firstThumbIndex;

    if (numToDo > 0)
    {
        const int numChans = jmin (channels.size(), incoming.getNumChannels());

        const HeapBlock<MinMaxColourValue> thumbData (numToDo * numChans);
        const HeapBlock<MinMaxColourValue*> thumbChannels (numChans);

        for (int chan = 0; chan < numChans; ++chan)
        {
            const float* const sourceData = incoming.getReadPointer (chan, startOffsetInBuffer);
            MinMaxColourValue* const dest = thumbData + numToDo * chan;
            thumbChannels [chan] = dest;

            for (int i = 0; i < numToDo; ++i)
            {
                float low, high;
                const int start = i * samplesPerThumbSample;
                findMinAndMax (sourceData + start, jmin (samplesPerThumbSample, numSamples - start), low, high);
                dest[i].setFloat (low, high);
            }
        }

        setLevels (thumbChannels, firstThumbIndex, numChans, numToDo);
    }
}

void ColouredAudioThumbnail::setLevels (const MinMaxColourValue* const* values, int thumbIndex, int numChans, int numValues)
{
    const ScopedLock sl (lock);

    for (int i = jmin (numChans, channels.size()); --i >= 0;)
        channels.getUnchecked(i)->write (values[i], thumbIndex, numValues);

    numSamplesFinished = jmax (numSamplesFinished, (thumbIndex + numValues) * (int64) samplesPerThumbSample);
    totalSamples = jmax (numSamplesFinished, totalSamples);
    window->invalidate();
    sendChangeMessage();
}

//==============================================================================
int ColouredAudioThumbnail::getNumChannels() const throw()
{
    return numChannels;
}

double ColouredAudioThumbnail::getTotalLength() const throw()
{
    return totalSamples / sampleRate;
}

bool ColouredAudioThumbnail::isFullyLoaded() const throw()
{
    return numSamplesFinished >= totalSamples - samplesPerThumbSample;
}

int64 ColouredAudioThumbnail::getNumSamplesFinished() const throw()
{
    return numSamplesFinished;
}

float ColouredAudioThumbnail::getApproximatePeak() const
{
    int peak = 0;

    for (int i = channels.size(); --i >= 0;)
        peak = jmax (peak, channels.getUnchecked(i)->getPeak());

    return jlimit (0, 127, peak) / 127.0f;
}

void ColouredAudioThumbnail::getApproximateMinMax (const double startTime, const double endTime, const int channelIndex,
                                                   float& minValue, float& maxValue) const noexcept
{
    const ScopedLock sl (lock);
    MinMaxColourValue result;
    const ThumbData* const data = channels [channelIndex];
    
    if (data != nullptr && sampleRate > 0)
    {
        const int firstThumbIndex = (int) ((startTime * sampleRate) / samplesPerThumbSample);
        const int lastThumbIndex  = (int) (((endTime * sampleRate) + samplesPerThumbSample - 1) / samplesPerThumbSample);
        
        data->getMinMax (jmax (0, firstThumbIndex), lastThumbIndex, result);
    }
    
    minValue = result.getMinValue() / 128.0f;
    maxValue = result.getMaxValue() / 128.0f;
}

void ColouredAudioThumbnail::drawChannel (Graphics& g, const Rectangle<int>& area, double startTime,
                                  double endTime, int channelNum, float verticalZoomFactor)
{
    const ScopedLock sl (lock);

    drawColouredChannel (g, area, startTime, endTime, channelNum, verticalZoomFactor);
}

void ColouredAudioThumbnail::drawColouredChannel (Graphics& g, const Rectangle<int>& area, double startTime,
												  double endTime, int channelNum, float verticalZoomFactor)
{
    const ScopedLock sl (lock);

    window->drawColouredChannel (g, area, startTime, endTime, channelNum, verticalZoomFactor,
								 sampleRate, numChannels, samplesPerThumbSample, source, channels);
}

void ColouredAudioThumbnail::drawChannels (Graphics& g, const Rectangle<int>& area, double startTimeSeconds,
                                           double endTimeSeconds, float verticalZoomFactor)
{
    for (int i = 0; i < numChannels; ++i)
    {
        const int y1 = roundToInt ((i * area.getHeight()) / numChannels);
        const int y2 = roundToInt (((i + 1) * area.getHeight()) / numChannels);

        drawChannel (g, Rectangle<int> (area.getX(), area.getY() + y1, area.getWidth(), y2 - y1),
                     startTimeSeconds, endTimeSeconds, i, verticalZoomFactor);
    }
}
