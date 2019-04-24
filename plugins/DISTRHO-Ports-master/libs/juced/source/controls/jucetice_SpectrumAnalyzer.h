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

#ifndef __JUCETICE_SPECTRUMANAYZER_HEADER__
#define __JUCETICE_SPECTRUMANAYZER_HEADER__

#if 0

//==============================================================================
/**
     
*/
class SpectrumAnalyzerState
{
public:

    SpectrumAnalyzerState ();
    ~SpectrumAnalyzerState ();

    /**
        This has to be called before any attempt to process data cause it
        will reset the internal state of the spectrum analyzer
    */
    void reset (double sampleRate, int samplesPerBlock);

    /**
        This will copy the next block of samples to the shared region previously
        set by the main component
    */
    void processNextBlock (AudioSampleBuffer& buffer);

    /** */
    void setDataBuffer (float* a, float* b, int ipsize, int ipstep);

private:

    CriticalSection dataLock;

    uint32         _fsamp;
    uint32         _fsize;
    int            _inputA;
    int            _inputB;
    float         *_dataA;
    float         *_dataB;
    int            _dind;
    int            _size;
    int            _step;
    int            _scnt;
};

#endif

#endif // __JUCETICE_SPECTRUMANAYZER_HEADER__
