/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_AUDIOSCOPECOMPONENT_HEADER__
#define __JUCETICE_AUDIOSCOPECOMPONENT_HEADER__

#include "../base/jucetice_StandardHeader.h"

//==============================================================================
/**

*/
class AudioScopeComponent  : public Component,
                             public Timer,
                             public AudioIODeviceCallback
{
public:

    AudioScopeComponent()
    {
        bufferPos = 0;
        bufferSize = 2048;
        circularBuffer = (float*) ::calloc (sizeof (float) * bufferSize, sizeof (float));
        currentInputLevel = 0.0f;
        numSamplesIn = 0;

        setOpaque (true);
        startTimer (1000 / 50);  // repaint every 1/50 of a second
    }

    ~AudioScopeComponent()
    {
        ::free (circularBuffer);
    }

    void paint (Graphics& g)
    {
        g.fillAll (Colours::black);

        LookAndFeel_V2::drawBevel (g,
                                0,
                                0,
                                getWidth(),
                                getHeight(),
                                1,
                                Colours::white,
                                Colours::grey,
                                false);

        g.setColour (Colours::lightgreen);

        const float halfHeight = getHeight() * 0.5f;

        int bp = bufferPos;

        for (int x = getWidth(); --x >= 0;)
        {
            const int samplesAgo = getWidth() - x;
            const float level = circularBuffer [(bp + bufferSize - samplesAgo) % bufferSize];

            if (level > 0.01f)
                g.drawLine ((float) x, halfHeight - halfHeight * level,
                            (float) x, halfHeight + halfHeight * level);
        }
    }

    void timerCallback()
    {
        repaint();
    }

    void addSample (const float sample)
    {
        currentInputLevel += fabsf (sample);

        const int samplesToAverage = 128; // 128

        if (++numSamplesIn > samplesToAverage)
        {
            circularBuffer [bufferPos++ % bufferSize] = currentInputLevel / samplesToAverage * 2.0;

            numSamplesIn = 0;
            currentInputLevel = 0.0f;
        }
    }

    void audioDeviceIOCallback (const float** inputChannelData,
                                int totalNumInputChannels,
                                float** outputChannelData,
                                int totalNumOutputChannels,
                                int numSamples)
    {
        for (int i = 0; i < totalNumInputChannels; ++i)
        {
            if (inputChannelData [i] != 0)
            {
                for (int j = 0; j < numSamples; ++j)
                    addSample (inputChannelData [i][j]);

                break;
            }
        }
    }

    void audioDeviceAboutToStart (double sampleRate, int numSamplesPerBlock)
    {
        zeromem (circularBuffer, sizeof (float) * bufferSize);
    }

    void audioDeviceStopped()
    {
        zeromem (circularBuffer, sizeof (float) * bufferSize);
    }

    //==============================================================================
    juce_UseDebuggingNewOperator

private:

    float* circularBuffer;
    float currentInputLevel;
    int volatile bufferPos, bufferSize, numSamplesIn;
};


#endif // __JUCETICE_AUDIOSCOPECOMPONENT_HEADER__

