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

   @author  Paul Nasca
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

extern "C" {
  #include "../../dependancies/kissfft/kiss_fft.c"
  #include "../../dependancies/kissfft/kiss_fftr.c"
}

BEGIN_JUCE_NAMESPACE

//==============================================================================
class FFTWrapperImpl
{
public:

    FFTWrapperImpl (int fftsize_)
    {
        fftsize = fftsize_;

        tmpfftdata1 = new kiss_fft_scalar [fftsize + 2];
        tmpfftdata2 = new kiss_fft_cpx [fftsize + 2];

        planfftw = kiss_fftr_alloc (fftsize, 0, NULL, NULL);
        planfftw_inv = kiss_fftr_alloc (fftsize, 1, NULL, NULL);
    }

    ~FFTWrapperImpl()
    {
        free (planfftw);
        free (planfftw_inv);

        kiss_fft_cleanup();

        delete[] tmpfftdata1;
        delete[] tmpfftdata2;
    }

    forcedinline void smps2freqs (float *smps, FFTFrequencies freqs)
    {
        for (int i = 0; i < fftsize; i++)
            tmpfftdata1[i] = smps[i];

        kiss_fftr (planfftw, tmpfftdata1, tmpfftdata2);

        for (int i = 0; i < fftsize / 2; i++)
        {
            freqs.c[i] = tmpfftdata2[i].r;
            if (i != 0)
                freqs.s[i] = tmpfftdata2[i].i;
        }

    //    tmpfftdata2[fftsize/2].r=0.0;
    //    tmpfftdata2[fftsize/2].i=0.0;
    }

    forcedinline void freqs2smps (FFTFrequencies freqs, float *smps)
    {
    //    tmpfftdata2[fftsize/2].r=0.0;
    //    tmpfftdata2[fftsize/2].i=0.0;

        for (int i = 0; i < fftsize / 2; i++)
        {
            tmpfftdata2[i].r = freqs.c[i];
            if (i != 0)
                tmpfftdata2[i].i = freqs.s[i];
        }

        kiss_fftri (planfftw_inv, tmpfftdata2, tmpfftdata1);

        for (int i = 0; i < fftsize; i++)
            smps[i] = tmpfftdata1[i];
    }


private:

    int fftsize;

    kiss_fft_scalar *tmpfftdata1;
    kiss_fft_cpx *tmpfftdata2;

    kiss_fftr_cfg planfftw, planfftw_inv;
};


//==============================================================================
FFTWrapper::FFTWrapper (int fftsize)
  : fft (new FFTWrapperImpl (fftsize))
{
}

FFTWrapper::~FFTWrapper()
{
    deleteAndZero (fft);
}

void FFTWrapper::smps2freqs (float *smps, FFTFrequencies freqs)
{
    fft->smps2freqs (smps, freqs);
}

void FFTWrapper::freqs2smps (FFTFrequencies freqs, float *smps)
{
    fft->freqs2smps (freqs, smps);
}

END_JUCE_NAMESPACE
