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

   @author  dspmaster@free.fr
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

/*

#define FREQ_LP_BEAT 150.0f                     // Low Pass filter frequency
#define T_FILTER 1.0f/(2.0f*M_PI*FREQ_LP_BEAT)  // Low Pass filter time constant
#define BEAT_RTIME 0.02f                        // Release time of envelope detector
                                                // in second

//==============================================================================
BeatDetector::BeatDetector()
    : Filter1Out (0.0),
      Filter2Out (0.0),
      PeakEnv (0.0),
      BeatTrigger (false),
      PrevBeatPulse (false)
{
    prepareToPlay (44100, 512);
}

BeatDetector::~BeatDetector()
{
}

int BeatDetector::getBeatAtIndex (const int index) const
{
    return samplesBeats.getUnchecked (index);
}

int BeatDetector::getNumBeats () const
{
    return samplesBeats.size ();
}

void BeatDetector::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Compute all sample frequency related coeffs
    KBeatFilter = 1.0 / (sampleRate * T_FILTER);
    BeatRelease = (float) exp (-1.0f / (sampleRate * BEAT_RTIME));
    
    samplesBeats.clear ();
}

void BeatDetector::releaseResources()
{
}

void BeatDetector::processBlock (float* input, int blockSize, int offsetSamples, int lengthInSamples)
{
    for (int i = 0; i < blockSize; i++)
    {
        // Step 1 : 2nd order low pass filter (made of two 1st order RC filter)
        Filter1Out = Filter1Out + (KBeatFilter * (*input++ - Filter1Out));
        Filter2Out = Filter2Out + (KBeatFilter * (Filter1Out - Filter2Out));

        // Step 2 : peak detector
        float EnvIn = fabs(Filter2Out);
        if (EnvIn > PeakEnv)
        {
            PeakEnv = EnvIn;  // Attack time = 0
        }
        else
        {
            PeakEnv *= BeatRelease;
            PeakEnv += (1.0f - BeatRelease) * EnvIn;
        }

        // Step 3 : Schmitt trigger
        if (! BeatTrigger)
        {
            if (PeakEnv > 0.3) BeatTrigger = true;
        }
        else
        {
            if (PeakEnv < 0.15) BeatTrigger = false;
        }

        // Step 4 : rising edge detector
        BeatPulse = false;

        if (BeatTrigger && ! PrevBeatPulse)
        {
            BeatPulse = true;

            samplesBeats.add (offsetSamples + i);
        }

        PrevBeatPulse = BeatTrigger;
    }
}

void BeatDetector::processAudioReader (AudioFormatReader* reader)
{
    AudioSampleBuffer buffer (2, reader->lengthInSamples);

    int blockSize = 1024;
    int currentSample = 0;
    int lengthInSamples = reader->lengthInSamples;
    int remainingSamples = lengthInSamples;
    
    prepareToPlay (reader->sampleRate, reader->lengthInSamples);

    while (remainingSamples > 0)
    {
        int numSamples = jmin (blockSize, lengthInSamples - currentSample);
    
        buffer.readFromAudioReader (reader,
                                    0,
                                    numSamples,
                                    currentSample,
                                    true, true);

        processBlock (buffer.getSampleData (0),
                      numSamples,
                      currentSample,
                      lengthInSamples);
        
        remainingSamples -= numSamples;
        currentSample += numSamples;
    }
    
    releaseResources ();
}

*/

END_JUCE_NAMESPACE

