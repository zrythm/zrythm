#ifndef TOMATL_WINDOW_FUNCTION
#define TOMATL_WINDOW_FUNCTION

#include <functional>

namespace tomatl { namespace dsp{

template <typename T> class WindowFunction
{
private:
	size_t mLength = 0;
	std::function<T(const int&, const size_t&)> mFunction;
	T* mPrecalculated = NULL;
	T mScalingFactor = 0.;

	TOMATL_DECLARE_NON_MOVABLE_COPYABLE(WindowFunction);
public:
	WindowFunction(size_t length, std::function<T(const int&, const size_t&)> func, bool periodicMode = false) : mFunction(func)
	{
		mPrecalculated = new T[length];
		mLength = length;
		memset(mPrecalculated, 0x0, sizeof(T)* length);

		if (periodicMode) ++length;

		for (int i = 0; i < mLength; ++i)
		{
			mPrecalculated[i] = mFunction(i, length);
			mScalingFactor += mPrecalculated[i];
		}

		mScalingFactor /= length;
	}

	forcedinline void applyFunction(T* signal, size_t start, size_t length = 1, bool scale = false)
	{
		int end = start + length;

		int sample = 0;

		for (int i = start; i < end; ++i)
		{
			if (i >= 0 && i < mLength)
			{
				signal[sample] *= mPrecalculated[i];

				if (scale) signal[sample] /= mScalingFactor;
			}
			else
			{
				signal[sample] = 0.;
			}

			++sample;
		}
	}

	// TODO: handle negative indices?
	forcedinline T applyPeriodic(T signal, size_t position)
	{
		T temp = signal;

		applyFunction(&temp, position % getLength());

		return temp;
	}

	forcedinline const size_t& getLength() { return mLength; }

	// After windowing input signal, it obviously becomes "more quiet", so we're need to compensate that sometimes (http://alpha.science.unitn.it/~bassi/Signal/NInotes/an041.pdf)
	// If we'll take signal consisting of all samples being equal to on one, its power (sum of all samples divided by sample count) will be equal to one
	// Applying window function to this signal and measuring its power will tell us how much power is being lost. This is how this thing can be calculated - 
	// Sum all precalculated window samples (on large size like 40960 for accuracy) and divide by sample count.
	forcedinline const T& getNormalizationFactor() { return mScalingFactor; }

	virtual ~WindowFunction()
	{
		TOMATL_BRACE_DELETE(mPrecalculated);
	}
};

class WindowFunctionFactory
{
private:
	WindowFunctionFactory(){}
public:
	enum FunctionType
	{
		windowRectangle = 0,
		windowBlackmanHarris,
		windowHann,
		windowBarlett,
		windowMystic
	};

	template <typename T> static std::function<T(const int&, const size_t&)> getWindowCalculator(FunctionType type)
	{
		if (type == windowRectangle)
		{
			return [](const int& i, const size_t& length) { return 1.; };
		}
		else if (type == windowHann)
		{
			return [](const int& i, const size_t& length) { return 0.5 * (1. - std::cos(2 * TOMATL_PI * i / (length - 1))); };
		}
		else if (type == windowBarlett)
		{
			return [](const int& i, const size_t& length)
			{
				double a = ((double)length - 1.) / 2.;

				return 1. - std::abs((i - a) / a); 
			};
		}
		else if (type == windowBlackmanHarris) // Four-term Blackman-Harris window
		{
			return[](const int& i, const size_t& length)
			{
				T a0 = 0.35875;
				T a1 = 0.48829;
				T a2 = 0.14128;
				T a3 = 0.01168;

				return 
					a0 -
					a1 * std::cos(2 * TOMATL_PI * i / (length - 1)) +
					a2 * std::cos(4 * TOMATL_PI * i / (length - 1)) -
					a3 * std::cos(6 * TOMATL_PI * i / (length - 1));
			};
		}
		else if (type == windowMystic)
		{
			return [](const int& i, const size_t& length)
			{
				double a = std::sin(TOMATL_PI * i + TOMATL_PI / 2 * length);

				return std::sin(TOMATL_PI / 2 * a * a);
			};
		}
		else
		{
			throw 20; // TODO: exception
		}
	}
};

}}

#endif