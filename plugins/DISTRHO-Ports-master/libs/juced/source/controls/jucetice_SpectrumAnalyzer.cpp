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

BEGIN_JUCE_NAMESPACE

#if 0

//==============================================================================
SpectrumAnalyzerState::SpectrumAnalyzerState()
  : _fsamp (44100),
    _fsize (512),
    _inputA (-1),
    _inputB (-1),
    _dataA (0),
    _dataB (0),
    _dind (0),
    _size (8192),
    _step (2),
    _scnt (0)
{
}

SpectrumAnalyzerState::~SpectrumAnalyzerState()
{
}

//==============================================================================
void SpectrumAnalyzerState::reset (double sampleRate, int samplesPerBlock)
{
    _fsamp = sampleRate;
    _fsize = samplesPerBlock;
    _dind  = 0;
    _scnt  = 0;
}

//==============================================================================
void SpectrumAnalyzerState::processNextBlock (AudioSampleBuffer& buffer)
{
    const ScopedTryLock scopedTryLock (dataLock);

    if (scopedTryLock.isLocked() && _dataA)
    {
        float  *pA, *pB;
        unsigned int  m, n, nframes = buffer.getNumSamples();
        
        pA = (_inputA >= 0) ? buffer.getSampleData (0) : 0;
        pB = (_inputB >= 0) ? buffer.getSampleData (1) : 0;

    	m = nframes;
        n = _size - _dind;

        if (m >= n)
	    {
            if (pA) memcpy (_dataA + _dind, pA, sizeof(float) * n);
	        else    memset (_dataA + _dind, 0,  sizeof(float) * n);

            if (pB) memcpy (_dataB + _dind, pB, sizeof(float) * n);
	        else    memset (_dataB + _dind, 0,  sizeof(float) * n);

            m -= n;
            pA += n;
            pB += n;
            _dind = 0;
        }

        if (m)
	    {
            if (pA) memcpy (_dataA + _dind, pA, sizeof(float) * m);
	        else    memset (_dataA + _dind, 0,  sizeof(float) * m);

            if (pB) memcpy (_dataB + _dind, pB, sizeof(float) * m);
	        else    memset (_dataB + _dind, 0,  sizeof(float) * m);

            _dind += m;
    	}

        _scnt += nframes;

        int k = _scnt / _step;
        if (k) _scnt -= k * _step;
    }
}

//==============================================================================
void SpectrumAnalyzerState::setDataBuffer (float* a, float* b, int ipsize, int ipstep)
{
    const ScopedLock sl (dataLock);

    _dataA = a;
    _dataB = b;
    
    _inputA = a != 0;
    _inputB = b != 0;

    _size  = ipsize;
    _step  = ipstep; 
    _dind  = 0;
    _scnt  = 0;
}

#endif

END_JUCE_NAMESPACE
