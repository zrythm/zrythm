/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004 by Julian Storer.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#include "XEQPlugin.h"
#include "XEQComponent.h"

//==============================================================================
int XEQPlugin::numInstances = 0;

//==============================================================================
XEQPlugin::XEQPlugin()
  : equalizer (1),
    denormalBuffer (1, 1),
    inputBuffers (2, 1),
    tempBuffers (2, 1)
{
    DBG ("XEQPlugin::XEQPlugin");

    setNumParameters (NUM_PARAMETERS);

    // GAIN
    registerParameter (PAR_GAIN, &params[PAR_GAIN].name ("Gain").unit ("%")
                                 .set (MakeDelegate (this, &XEQPlugin::setGain))
                                 .get (MakeDelegate (this, &XEQPlugin::getGain)));

    // DRY WET
    registerParameter (PAR_DRYWET, &params[PAR_DRYWET].name ("Dry/Wet").unit ("%")
                                   .set (MakeDelegate (this, &XEQPlugin::setDryWet))
                                   .get (MakeDelegate (this, &XEQPlugin::getDryWet)));

#if 1

    // THRESHOLD
    registerParameter (PAR_THRESHOLD, &params[PAR_THRESHOLD].part (0).name ("Threshold").unit ("%")
                                      .set (MakeDelegate (&limiter, &Limiter::setParameter))
                                      .get (MakeDelegate (&limiter, &Limiter::getParameter)));

    // KNEE
    registerParameter (PAR_KNEE, &params[PAR_KNEE].part (4).name ("Knee").unit ("%")
                                 .set (MakeDelegate (&limiter, &Limiter::setParameter))
                                 .get (MakeDelegate (&limiter, &Limiter::getParameter)));

    // TRIM
    registerParameter (PAR_TRIM, &params[PAR_TRIM].part (1).name ("Trim").unit ("%")
                                 .set (MakeDelegate (&limiter, &Limiter::setParameter))
                                 .get (MakeDelegate (&limiter, &Limiter::getParameter)));

    // ATTACK
    registerParameter (PAR_ATTACK, &params[PAR_ATTACK].part (2).name ("Attack").unit ("%")
                                   .set (MakeDelegate (&limiter, &Limiter::setParameter))
                                   .get (MakeDelegate (&limiter, &Limiter::getParameter)));

    // RELEASE
    registerParameter (PAR_RELEASE, &params[PAR_RELEASE].part (3).name ("Release").unit ("%")
                                    .set (MakeDelegate (&limiter, &Limiter::setParameter))
                                    .get (MakeDelegate (&limiter, &Limiter::getParameter)));

#endif

    // BAND 1
    registerParameter (PAR_BAND1GAIN, &params[PAR_BAND1GAIN].part (0).name ("B-1 Gain").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandGain))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandGain)));

    registerParameter (PAR_BAND1FREQ, &params[PAR_BAND1FREQ].part (0).name ("B-1 Freq").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandFreq))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandFreq)));

    registerParameter (PAR_BAND1BW, &params[PAR_BAND1BW].part (0).name ("B-1 Bw").unit ("%")
                                    .set (MakeDelegate (this, &XEQPlugin::setBandBw))
                                    .get (MakeDelegate (this, &XEQPlugin::getBandBw)));

    // BAND 2
    registerParameter (PAR_BAND2GAIN, &params[PAR_BAND2GAIN].part (1).name ("B-2 Gain").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandGain))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandGain)));

    registerParameter (PAR_BAND2FREQ, &params[PAR_BAND2FREQ].part (1).name ("B-2 Freq").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandFreq))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandFreq)));

    registerParameter (PAR_BAND2BW, &params[PAR_BAND2BW].part (1).name ("B-2 Bw").unit ("%")
                                    .set (MakeDelegate (this, &XEQPlugin::setBandBw))
                                    .get (MakeDelegate (this, &XEQPlugin::getBandBw)));

    // BAND 3
    registerParameter (PAR_BAND3GAIN, &params[PAR_BAND3GAIN].part (2).name ("B-3 Gain").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandGain))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandGain)));

    registerParameter (PAR_BAND3FREQ, &params[PAR_BAND3FREQ].part (2).name ("B-3 Freq").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandFreq))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandFreq)));

    registerParameter (PAR_BAND3BW, &params[PAR_BAND3BW].part (2).name ("B-3 Bw").unit ("%")
                                    .set (MakeDelegate (this, &XEQPlugin::setBandBw))
                                    .get (MakeDelegate (this, &XEQPlugin::getBandBw)));

    // BAND 4
    registerParameter (PAR_BAND4GAIN, &params[PAR_BAND4GAIN].part (3).name ("B-4 Gain").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandGain))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandGain)));

    registerParameter (PAR_BAND4FREQ, &params[PAR_BAND4FREQ].part (3).name ("B-4 Freq").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandFreq))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandFreq)));

    registerParameter (PAR_BAND4BW, &params[PAR_BAND4BW].part (3).name ("B-4 Bw").unit ("%")
                                    .set (MakeDelegate (this, &XEQPlugin::setBandBw))
                                    .get (MakeDelegate (this, &XEQPlugin::getBandBw)));

    // BAND 5
    registerParameter (PAR_BAND5GAIN, &params[PAR_BAND5GAIN].part (4).name ("B-5 Gain").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandGain))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandGain)));

    registerParameter (PAR_BAND5FREQ, &params[PAR_BAND5FREQ].part (4).name ("B-5 Freq").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandFreq))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandFreq)));

    registerParameter (PAR_BAND5BW, &params[PAR_BAND5BW].part (4).name ("B-5 Bw").unit ("%")
                                    .set (MakeDelegate (this, &XEQPlugin::setBandBw))
                                    .get (MakeDelegate (this, &XEQPlugin::getBandBw)));

    // BAND 6
    registerParameter (PAR_BAND6GAIN, &params[PAR_BAND6GAIN].part (5).name ("B-6 Gain").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandGain))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandGain)));

    registerParameter (PAR_BAND6FREQ, &params[PAR_BAND6FREQ].part (5).name ("B-6 Freq").unit ("%")
                                      .set (MakeDelegate (this, &XEQPlugin::setBandFreq))
                                      .get (MakeDelegate (this, &XEQPlugin::getBandFreq)));

    registerParameter (PAR_BAND6BW, &params[PAR_BAND6BW].part (5).name ("B-6 Bw").unit ("%")
                                    .set (MakeDelegate (this, &XEQPlugin::setBandBw))
                                    .get (MakeDelegate (this, &XEQPlugin::getBandBw)));

    // initialise effect section
    initialiseToDefault ();
}

XEQPlugin::~XEQPlugin()
{
    DBG ("XEQPlugin::~XEQPlugin");
}

//==============================================================================
float XEQPlugin::getGain (int n) { return equalizer.getParameter (0) / 127.0f; }
void XEQPlugin::setGain (int n, float value) { equalizer.setParameter (0, (int) (value * 127.0f)); }
float XEQPlugin::getDryWet (int n) { return equalizer.getParameter (1) / 127.0f; }
void XEQPlugin::setDryWet (int n, float value) { equalizer.setParameter (1, (int) (value * 127.0f)); }

//==============================================================================
float XEQPlugin::getBandGain (int band) { return equalizer.getParameter ((band * 5 + 12)) / 127.0f; }
void XEQPlugin::setBandGain (int band, float value) { equalizer.setParameter ((band * 5 + 12), (int) (value * 127.0f)); }

float XEQPlugin::getBandFreq (int band) { return equalizer.getParameter ((band * 5 + 11)) / 127.0f; }
void XEQPlugin::setBandFreq (int band, float value) { equalizer.setParameter ((band * 5 + 11), (int) (value * 127.0f)); }

float XEQPlugin::getBandBw (int band) { return equalizer.getParameter ((band * 5 + 13)) / 127.0f; }
void XEQPlugin::setBandBw (int band, float value) { equalizer.setParameter ((band * 5 + 13), (int) (value * 127.0f)); }

//==============================================================================
void XEQPlugin::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    DBG ("XEQPlugin::prepareToPlay");

    int numSamples = samplesPerBlock * 2;

    inputBuffers.setSize (2, numSamples, false, true);
    tempBuffers.setSize (2, numSamples, false, true);
    denormalBuffer.setSize (1, numSamples, false, true);

    for (int i = 0; i < numSamples; i++)
        *denormalBuffer.getWritePointer (0, i) = ((rand() / (float) RAND_MAX) - 0.5) * 1e-16;

    equalizer.prepareToPlay (sampleRate, samplesPerBlock);
}

void XEQPlugin::releaseResources()
{
    DBG ("XEQPlugin::releaseResources");

    equalizer.releaseResources ();
}

void XEQPlugin::processBlock (AudioSampleBuffer& buffer,
                              MidiBuffer& midiMessages)
{
    // process incoming midi
    midiAutomatorManager.handleMidiMessageBuffer (midiMessages);

    // overall samples to process
    int blockSamples = buffer.getNumSamples();

    // get buffers
    const float* inl = buffer.getReadPointer (0);
    const float* inr = buffer.getReadPointer (1);
    float* inputL = inputBuffers.getWritePointer (0);
    float* inputR = inputBuffers.getWritePointer (1);
    float* denL = denormalBuffer.getWritePointer (0);
    float* denR = denormalBuffer.getWritePointer (0);

    // this is needed when using jack buffers directly
    for (int i = 0; i < blockSamples; i++)
    {
        *inputL++ = (*inl++) + (*denL++);
        *inputR++ = (*inr++) + (*denR++);
    }

    // process equalizer
    equalizer.out (inputBuffers.getReadPointer (0),
                   inputBuffers.getReadPointer (1),
                   buffer.getWritePointer (0),
                   buffer.getWritePointer (1),
                   tempBuffers.getWritePointer (0),
                   tempBuffers.getWritePointer (1),
                   blockSamples);
#if 0
    // process limiter (if enabled)
    limiter.out (buffer, blockSamples);
#endif
}

//==============================================================================
void XEQPlugin::getStateInformation (MemoryBlock& destData)
{
    DBG ("XEQPlugin::getStateInformation");

    try
    {
        MemoryBlock tempBlock;
        XmlElement xml ("main");

        equalizer.addToXML (&xml);

        String xmlString = xml.createDocument (String());
        destData.append ((const char*) xmlString.toUTF8(), xmlString.length());
    }
    catch (...)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     "Error !",
                                     "Something bad occurred while saving presets data !");
    }
}

void XEQPlugin::setStateInformation (const void* data, int sizeInBytes)
{
    DBG ("XEQPlugin::setStateInformation");

    suspendProcessing (true);

    try
    {
        if (data && sizeInBytes > 0)
        {
            XmlDocument xmlDoc ((char*) data);
            XmlElement* xml = xmlDoc.getDocumentElement();

            if (xml == 0 || ! xml->hasTagName ("main"))
            {
                String errString = xmlDoc.getLastParseError();
                printf ("Error restoring preset: %s \n", (const char*) errString.toUTF8());
            }
            else
            {
                equalizer.updateFromXML (xml);
                delete xml;
            }

            sendChangeMessage ();
        }
    }
    catch (...)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     "Error !",
                                     "Something bad occurred while restoring presets data !");
    }

    suspendProcessing (false);
}

//==============================================================================
void XEQPlugin::initialiseToDefault ()
{
    // prepare effect
    int band;

    // lowshelf --
    band = 0; equalizer.setParameter ((band * 5 + 10), 8);
              equalizer.setParameter ((band * 5 + 11), 64); // freq
              equalizer.setParameter ((band * 5 + 13), 52); // bw
              equalizer.setParameter ((band * 5 + 14), 1);  // stages
    // peak --
    band = 1; equalizer.setParameter ((band * 5 + 10), 7);
              equalizer.setParameter ((band * 5 + 11), 35); // freq
              equalizer.setParameter ((band * 5 + 13), 46); // bw
              equalizer.setParameter ((band * 5 + 14), 1);  // stages
    // peak --
    band = 2; equalizer.setParameter ((band * 5 + 10), 7);
              equalizer.setParameter ((band * 5 + 11), 50); // freq
              equalizer.setParameter ((band * 5 + 13), 46); // bw
              equalizer.setParameter ((band * 5 + 14), 1);  // stages
    // peak --
    band = 3; equalizer.setParameter ((band * 5 + 10), 7);
              equalizer.setParameter ((band * 5 + 11), 65); // freq
              equalizer.setParameter ((band * 5 + 13), 46); // bw
              equalizer.setParameter ((band * 5 + 14), 1);  // stages
    // peak --
    band = 4; equalizer.setParameter ((band * 5 + 10), 7);
              equalizer.setParameter ((band * 5 + 11), 80); // freq
              equalizer.setParameter ((band * 5 + 13), 46); // bw
              equalizer.setParameter ((band * 5 + 14), 1);  // stages
    // hishelf --
    band = 5; equalizer.setParameter ((band * 5 + 10), 9);
              equalizer.setParameter ((band * 5 + 11), 94); // freq
              equalizer.setParameter ((band * 5 + 13), 46); // bw
              equalizer.setParameter ((band * 5 + 14), 1);  // stages
}


//==============================================================================
Equalizer* XEQPlugin::getEqualizer()
{
    return &equalizer;
}

XEQComponent* XEQPlugin::getEditor()
{
    return (XEQComponent*) getActiveEditor();
}

AudioProcessorEditor* XEQPlugin::createEditor()
{
    return new XEQComponent (this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter ()
{
    return new XEQPlugin();
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter (const String& createArgs)
{
    return new XEQPlugin();
}
