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

#ifndef __JUCETICE_BEATDETECTOR_HEADER__
#define __JUCETICE_BEATDETECTOR_HEADER__

/*

class BeatDetector
{
public:

    BeatDetector();
    ~BeatDetector();

    void processAudioReader (AudioFormatReader* reader);

    int getBeatAtIndex (const int index) const;
    int getNumBeats () const;

protected:

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void processBlock (float* input, int blockSize, int offsetSamples, int lengthInSamples);
    void releaseResources();

    Array<int> samplesBeats;

    bool BeatPulse;           // Beat detector output
    float KBeatFilter;        // Filter coefficient
    float Filter1Out, Filter2Out;
    float BeatRelease;        // Release time coefficient
    float PeakEnv;            // Peak enveloppe follower
    bool BeatTrigger;         // Schmitt trigger output
    bool PrevBeatPulse;       // Rising edge memory
};

*/

#endif

