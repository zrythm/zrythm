#ifndef TOMATL_SPECTRO_CALCULATOR
#define TOMATL_SPECTRO_CALCULATOR

#include <limits>
#include <memory>

namespace tomatl { namespace dsp {

	struct SpectrumBlock
	{
		SpectrumBlock()
		{
			mLength = 0;
			mData = NULL;
			mIndex = 0;
			mSampleRate = 0;
			mFramesRendered = 0;
		}

		SpectrumBlock(size_t size, std::pair<double, double>* data, size_t index, size_t sampleRate)
		{
			mLength = size;
			mData = data;
			mIndex = index;
			mSampleRate = sampleRate;
			mFramesRendered = 0;
		}

		size_t mLength;
		size_t mIndex;
		size_t mSampleRate;
		size_t mFramesRendered;
		std::pair<double, double>* mData;
	};

	template <typename T> class SpectroCalculator
	{
	public:
		SpectroCalculator(double sampleRate, std::pair<double, double> attackRelease, size_t index, size_t fftSize = 1024, size_t channelCount = 2) : 
			mWindowFunction(new WindowFunction<T>(fftSize, WindowFunctionFactory::getWindowCalculator<double>(WindowFunctionFactory::windowHann), true))
		{
			mData = new std::pair<double, double>[fftSize];
			memset(mData, 0x0, sizeof(std::pair<double, double>) * fftSize);
			mChannelCount = channelCount;
			mFftSize = fftSize;
			mIndex = index;
			mSampleRate = sampleRate;
			mChannelCount = 0;
			checkChannelCount(channelCount);

			setAttackSpeed(attackRelease.first);
			setReleaseSpeed(attackRelease.second);
		}

		~SpectroCalculator()
		{
			TOMATL_BRACE_DELETE(mData);

			for (int i = 0; i < mChannelCount; ++i)
			{
				TOMATL_DELETE(mBuffers[i]);
			}

			mBuffers.clear();
		}

		bool checkChannelCount(size_t channelCount)
		{
			if (channelCount != mChannelCount)
			{
				mChannelCount = channelCount;

				for (int i = 0; i < mBuffers.size(); ++i)
				{
					TOMATL_DELETE(mBuffers[i]);
				}

				mBuffers.clear();

				for (int i = 0; i < mChannelCount; ++i)
				{
					mBuffers.push_back(new OverlappingBufferSequence<T>(mFftSize * 2, mFftSize));
				}

				setReleaseSpeed(mReleaseMs);
				setAttackSpeed(mAttackMs);

				return true;
			}

			return false;
		}

		bool checkSampleRate(double sampleRate)
		{
			if (sampleRate != mSampleRate)
			{
				mSampleRate = sampleRate;
				setAttackSpeed(mAttackMs);
				setReleaseSpeed(mReleaseMs);

				return true;
			}

			return false;
		}

		void setReleaseSpeed(double speed)
		{
			mReleaseMs = speed;
			mAttackRelease.second = tomatl::dsp::EnvelopeWalker::calculateCoeff(speed, mSampleRate / mFftSize / mBuffers[0]->getOverlappingFactor() * mChannelCount);
		}

		void setAttackSpeed(double speed)
		{
			mAttackMs = speed;
			mAttackRelease.first = tomatl::dsp::EnvelopeWalker::calculateCoeff(speed, mSampleRate / mFftSize / mBuffers[0]->getOverlappingFactor() * mChannelCount);
		}

		SpectrumBlock process(T* channels)
		{
			bool processed = false;

			for (int i = 0; i < mChannelCount; ++i)
			{
				// As our signal is built entirely from real numbers, imaginary part will always be zero
				mBuffers[i]->putOne(channels[i]);
				auto chData = mBuffers[i]->putOne(0.);

				processed = processed || calculateSpectrumFromChannelBufferIfReady(std::get<0>(chData));
			}

			if (processed)
			{
				return SpectrumBlock(mFftSize / 2., mData, mIndex, mSampleRate);
			}
			else
			{
				return SpectrumBlock();
			}
		}

	private:

		bool calculateSpectrumFromChannelBufferIfReady(T* chData)
		{
			if (chData != NULL)
			{
				// Apply window function to buffer
				for (int s = 0; s < mFftSize; ++s)
				{
					mWindowFunction->applyFunction(chData + s * 2, s, 1, true);
				}

				// In-place calculate FFT
				FftCalculator<T>::calculateFast(chData, mFftSize);

				// Calculate frequency-magnitude pairs (omitting phase information, as we won't need it) for all frequency bins
				for (int bin = 0; bin < (mFftSize / 2.); ++bin)
				{
					T ampl = 0.;

					T* ftResult = chData;

					// FFT bin in rectangle form
					T mFftSin = ftResult[bin * 2];
					T mFftCos = ftResult[bin * 2 + 1];

					// http://www.dsprelated.com/showmessage/69952/1.php or see below
					mFftSin *= 2;
					mFftCos *= 2;
					mFftSin /= mFftSize;
					mFftCos /= mFftSize;

					// Partial conversion to polar coordinates: we calculate radius vector length, but don't calculate angle (aka phase) as we won't need it
					T nw = std::sqrt(mFftSin * mFftSin + mFftCos * mFftCos);

					ampl = std::max(nw, ampl);

					double prev = mData[bin].second;

					// Special case - hold spectrum (aka infinite release time)
					if (mAttackRelease.second == std::numeric_limits<double>::infinity())
					{
						prev = std::max(prev, ampl);
					}
					else // Time smoothing/averaging is being done here
					{
						EnvelopeWalker::staticProcess(ampl, &prev, mAttackRelease.first, mAttackRelease.second);
					}

					mData[bin].first = bin;
					mData[bin].second = prev;
				}

				return true;
			}

			return false;
		}

		std::vector<OverlappingBufferSequence<T>*> mBuffers;
		std::pair<double, double>* mData;
		std::pair<double, double> mAttackRelease;
		std::unique_ptr<WindowFunction<T>> mWindowFunction;
		size_t mChannelCount;
		size_t mFftSize;
		size_t mIndex;
		double mSampleRate;
		double mAttackMs;
		double mReleaseMs;
	};

}}

/*

What do you expect? I assume you have an N length sinusoidal of
the form

x[n] = A sin(wn)

and find a spectrum coefficient of magnitude A*N/2 (provided
the frquency w is an integer fraction of the sampling frequency
w = m/N, m < N/2).

First the factor 1/2. The spectrum of a real-valued signal is
conjugate symmetric, meaning one real-valed sinusoidal is
represented as two complex-valued sinusoidals according
to Eulers formula,

A*sin(x) = A*(exp(jx)-exp(-jx))/j2.

These two complex-valued sinusoidals have magnitde
A/2, so if you plot only the range [0,fs/2] you need to
scale by a factor 2 to recover A.

Second, the factor N. The DFT is a set of vector products
between the input signal x and N complex elements of
unit magnitude:

X[k] = sum_n=0^N-1 A*exp(j*2*pi*k*n/N)*exp(-j*2*pi*k*n/N)
= N*A

To recover A, one needs to divide by N.

*/

#endif