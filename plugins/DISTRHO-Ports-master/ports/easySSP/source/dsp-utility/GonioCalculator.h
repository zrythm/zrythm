#ifndef TOMATL_GONIO_CALCULATOR
#define TOMATL_GONIO_CALCULATOR

namespace tomatl { namespace dsp {


template<typename T> class GonioCalculator
{
public:
	GonioCalculator(size_t segmentLength = 512, size_t sampleRate = 48000, std::pair<double, double> autoAttackRelease = std::pair<double, double>(0.01, 5000))
		: mData(NULL), mProcCounter(0), mCustomScaleEnabled(false), mCustomScale(1.), mLastScale(1.)
	{
		setSegmentLength(segmentLength);
		mSqrt2 = std::pow(2., 0.5);
		mEnvelope.setAttackSpeed(autoAttackRelease.first);
		mEnvelope.setReleaseSpeed(autoAttackRelease.second);
		mEnvelope.setSampleRate(sampleRate);
		
	}

	void setReleaseSpeed(double value)
	{
		mEnvelope.setReleaseSpeed(value);
	}

	std::pair<T, T>* handlePoint(const std::pair<T, T>& subject, size_t sampleRate)
	{
		std::pair<T, T> point(subject);
		mEnvelope.setSampleRate(sampleRate);
		
		Coord<T>::toPolar(point);
		Coord<T>::rotatePolarDegrees(point, -45.);

		// Scale auto-adjusting. We tend to use max space available even if our signal is not normalized to 0dB
		double m = 1. / std::max(0.01, mEnvelope.process(point.first)); // 0.01 is limit not to expand beneath -40dB
		
		mLastScale = mCustomScaleEnabled ? (1. / mCustomScale) : m;

		point.first *= mLastScale;

		Coord<T>::toCartesian(point);

		point.first = std::min(1., std::max(-1., point.first));
		point.second = std::min(1., std::max(-1., point.second));

		mData[mProcCounter] = point;

		++mProcCounter;

		if (mProcCounter >= mSegmentLength)
		{
			mProcCounter = 0;

			return mData;
		}
		else
		{
			return NULL;
		}
	}

	std::pair<T, T>* handlePoint(const T& x, const T& y, size_t sampleRate)
	{
		std::pair<T, T> point(x, y);

		return handlePoint(point, sampleRate);
	}

	GonioCalculator& setSegmentLength(size_t segmentLength)
	{
		TOMATL_DELETE(mData);
		mSegmentLength = segmentLength;

		mData = new std::pair<T, T>[mSegmentLength];

		memset(mData, 0, sizeof(std::pair<T, T>) * mSegmentLength);
		mProcCounter = 0;

		return *this;
	}

	size_t getSegmentLength()
	{
		return mSegmentLength;
	}

	double getCurrentScaleValue()
	{
		return mLastScale;
	}

	void setCustomScaleEnabled(bool value) { mCustomScaleEnabled = value; }
	void setCustomScale(double value) { mCustomScale = TOMATL_BOUND_VALUE(value, 0., 1.); }

private:
	size_t mSegmentLength;
	std::pair<T, T>* mData;
	unsigned int mProcCounter;
	double mSqrt2;
	bool mCustomScaleEnabled;
	double mCustomScale;
	double mLastScale;
	tomatl::dsp::EnvelopeWalker mEnvelope;
};

}}

#endif