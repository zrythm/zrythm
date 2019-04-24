#ifndef TOMATL_FFT_CALCULATOR
#define TOMATL_FFT_CALCULATOR

namespace tomatl { namespace dsp {

template <typename T> class FftCalculator
{
private:
	FftCalculator(){}
	TOMATL_DECLARE_NON_MOVABLE_COPYABLE(FftCalculator);
public:

	static void scaleAfterFft(T* fftBuffer, long fftFrameSize)
	{
		size_t size = fftFrameSize * 2;
		T factor = 1. / (T)fftFrameSize;

		for (int i = 0; i < size; ++i)
		{
			fftBuffer[i] *= factor;
		}
	}

	// For testing purposes only. Should not be used in RT processing, as it allocates memory
	static void calculateReal(T* fftBuffer, long length)
	{
		T* buffer = new T[length * 2];

		for (int i = 0; i < length; ++i)
		{
			buffer[i * 2] = fftBuffer[i];
			buffer[i * 2 + 1] = 0.;
		}

		calculateFast(buffer, length, false);
		memcpy(fftBuffer, buffer, sizeof(T) * length);

		delete[] buffer;
	}

	static void calculateFast(T* fftBuffer, long fftFrameSize, bool inverse = false)
		/*
		FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)
		Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the time domain data in fftBuffer[0...2*fftFrameSize-1].
		The FFT array takes and returns the cosine and sine parts in an interleaved manner, ie. fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf.
		fftFrameSize must be a power of 2. It expects a complex input signal (see footnote 2),
		ie. when working with 'common' audio signals our input signal has to be passed as {in[0],0.,in[1],0.,in[2],0.,...} asf.
		In that case, the transform of the frequencies of interest is in fftBuffer[0...fftFrameSize].
		*/
	{
		long sign = (!inverse) ? -1 : 1;
		T wr, wi, arg, *p1, *p2, temp;
		T tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
		long i, bitm, j, le, le2, k, logN;
		logN = (long)(log(fftFrameSize) / log(2.) + .5);

		for (i = 2; i < 2 * fftFrameSize - 2; i += 2)
		{
			for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1)
			{
				if (i & bitm) j++;
				j <<= 1;

			}

			if (i < j)
			{
				p1 = fftBuffer + i; p2 = fftBuffer + j;
				temp = *p1; *(p1++) = *p2;
				*(p2++) = temp; temp = *p1;
				*p1 = *p2; *p2 = temp;
			}
		}

		for (k = 0, le = 2; k < logN; k++)
		{
			le <<= 1;
			le2 = le >> 1;
			ur = 1.0;
			ui = 0.0;
			arg = TOMATL_PI / (le2 >> 1);
			wr = cos(arg);
			wi = sign*sin(arg);

			for (j = 0; j < le2; j += 2)
			{
				p1r = fftBuffer + j; p1i = p1r + 1;
				p2r = p1r + le2; p2i = p2r + 1;

				for (i = j; i < 2 * fftFrameSize; i += le)
				{
					tr = *p2r * ur - *p2i * ui;
					ti = *p2r * ui + *p2i * ur;
					*p2r = *p1r - tr; *p2i = *p1i - ti;
					*p1r += tr; *p1i += ti;
					p1r += le; p1i += le;
					p2r += le; p2i += le;
				}

				tr = ur*wr - ui*wi;
				ui = ur*wi + ui*wr;
				ur = tr;
			}
		}
	}
};

}}

#endif
