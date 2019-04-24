#ifndef TOMATL_COORD_UTILITY
#define TOMATL_COORD_UTILITY

#include <vector>
#include <cmath>

namespace tomatl { namespace dsp {

template<typename T> class Coord
{
public:
	static void toPolar(T& x, T& y)
	{
		T r = std::sqrt(x * x + y * y);
		T a = std::atan2(x, y);

		std::swap(x, r);
		std::swap(y, a);
	}

	static void toPolar(std::pair<T, T>& subject)
	{
		toPolar(subject.first, subject.second);
	}

	static void toCartesian(T& r, T& a)
	{
		T x = std::sin(a) * r;
		T y = std::cos(a) * r;

		std::swap(x, r);
		std::swap(y, a);
	}

	static void toCartesian(std::pair<T, T>& subject)
	{
		toCartesian(subject.first, subject.second);
	}

	static void rotatePolarDegrees(std::pair<T, T>& subject, T angle)
	{
		rotatePolarRadians(subject, angle * (2. * TOMATL_PI * (1. / 360.)));
	}

	static void rotatePolarRadians(std::pair<T, T>& subject, T angle)
	{
		subject.second += angle;
	}

private:
	Coord()
	{
	}

	Coord(Coord& c)
	{
	}

	~Coord()
	{
	}
};

}}

#endif