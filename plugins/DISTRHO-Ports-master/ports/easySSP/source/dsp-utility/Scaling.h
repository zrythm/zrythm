#ifndef TOMATL_SCALING
#define TOMATL_SCALING

namespace tomatl { namespace dsp {

template <typename T> struct SingleBound
{
	SingleBound()
	{
		
	}

	SingleBound(T low, T high)
	{
		mLow = low;
		mHigh = high;
	}

	T mLow;
	T mHigh;
};

template <typename T> struct Bound2D
{
	Bound2D() : X(0, 0), Y(0, 0)
	{

	}

	Bound2D(T xl, T xh, T yl, T yh) : X(xl, xh), Y(yl, yh)
	{

	}

	bool areEqual(const Bound2D<T>& other)
	{
		return (other.X.mLow == X.mLow && other.X.mHigh == X.mHigh && other.Y.mLow == Y.mLow && other.Y.mHigh == Y.mHigh);
	}

	SingleBound<T> X;
	SingleBound<T> Y;
};

template <typename T> class IScale
{
public:
	virtual int scale(int length, SingleBound<T> bound, T val, bool limit = false) = 0;
	virtual double unscale(int length, SingleBound<T> bound, int val, bool limit = false) = 0;
	
	static T doLimitValue(T value, T min, T max)
	{
		if (value <= min)
		{
			return min;
		}
		else if (value >= max)
		{
			return max;
		}
		else
		{
			return value;
		}
	}

	static double log(double x, double base)
	{
		return std::log(x) / std::log(base);
	}
};

class LogScale : public IScale<double>
{
public: 
	LogScale() { }

	int scale(int length, SingleBound<double> bound, double val, bool limit = false)
	{
		if (limit)
		{
			val = doLimitValue(val, bound.mLow, bound.mHigh);
		}

		double range = std::abs(bound.mHigh - bound.mLow);

		double expBase = std::pow(range, (1.0 / (double)length));
		
		return (int)round(log(val - bound.mLow + 1, expBase));
	}

	double unscale(int length, SingleBound<double> bound, int val, bool limit = false)
	{
		double range = std::abs(bound.mHigh - bound.mLow);

		double expBase = std::pow(range, (1.0 / (double)length));

		double value = std::pow(expBase, ((double)val)) + bound.mLow - 1;

		if (limit)
		{
			value = doLimitValue(value, bound.mLow, bound.mHigh);
		}

		return value;
	}
};

class OctaveScale : public IScale<double>
{
public:
	OctaveScale() { }

	virtual int scale(int length, SingleBound<double> bound, double val, bool limit = false)
	{
		if (limit)
		{
			val = doLimitValue(val, bound.mLow, bound.mHigh);
		}

		double octaveCount = std::log2(bound.mHigh) - std::log2(bound.mLow);
		double pixelPerOctave = length / octaveCount;
		double offset = std::round(std::log2(bound.mLow / pixelPerOctave) * pixelPerOctave);
		
		return (int)std::round(std::log2(val / pixelPerOctave) * pixelPerOctave - offset);
	}

	virtual double unscale(int length, SingleBound<double> bound, int val, bool limit = false)
	{
		double octaveCount = std::log2(bound.mHigh) - std::log2(bound.mLow);
		double pixelPerOctave = length / octaveCount;
		double offset = std::round(std::log2(bound.mLow / pixelPerOctave) * pixelPerOctave);

		double value = pixelPerOctave * std::pow(2., ((val + offset) / pixelPerOctave));

		if (limit)
		{
			value = doLimitValue(value, bound.mLow, bound.mHigh);
		}

		return value;
	}
};

class LinearScale : public IScale<double>
{
public:
	LinearScale() { }

	int scale(int length, SingleBound<double> bound, double val, bool limit = false)
	{
		if (limit)
		{
			val = doLimitValue(val, bound.mLow, bound.mHigh);
		}

		double increment = std::abs(bound.mHigh - bound.mLow) / ((double)length);

		int result = (int)std::round((val - bound.mLow) / increment);

		if (limit)
		{
			result = doLimitValue(result, 0, length);
		}

		return result;
	}

	double unscale(int length, SingleBound<double> bound, int val, bool limit = false)
	{
		double increment = std::abs(bound.mHigh - bound.mLow) / ((double)length);

		double value = increment * ((double)val) + bound.mLow;

		if (limit)
		{
			value = doLimitValue(value, bound.mLow, bound.mHigh);
		}

		return value;
	}
};
}}
#endif
