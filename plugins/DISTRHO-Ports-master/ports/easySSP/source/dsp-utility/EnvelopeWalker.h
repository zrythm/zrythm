#ifndef TOMATL_ENVELOPE_WALKER
#define TOMATL_ENVELOPE_WALKER

#include <vector>

namespace tomatl { namespace dsp {

class EnvelopeWalker
{
public:
	EnvelopeWalker()
	{
		setAttackSpeed(1.).setReleaseSpeed(10.).setSampleRate(44100.);
	}

	~EnvelopeWalker(void)
	{
	}

	EnvelopeWalker& setAttackSpeed(double attackMs)
	{
		m_attackMs = attackMs;
		m_attackCoef = calculateCoeff(attackMs, m_sampleRate);

		return *this;
	}

	EnvelopeWalker& setReleaseSpeed(double releaseMs)
	{
		m_releaseMs = releaseMs;
		m_releaseCoef = calculateCoeff(releaseMs, m_sampleRate);

		return *this;
	}

	EnvelopeWalker& setSampleRate(double sampleRate)
	{
		if (sampleRate != m_sampleRate)
		{
			m_sampleRate = sampleRate;
			setAttackSpeed(m_attackMs);
			setReleaseSpeed(m_releaseMs);
		}

		return *this;
	}

	static double calculateCoeff(double coeffInMs, double sampleRate)
	{
		return exp(log(0.01) / (coeffInMs * sampleRate * 0.001));
	}

	static void staticProcess(const double& in, double* current, const double& attackCoef, const double& releaseCoef)
	{
		double tmp = fabs(in);

		if (tmp > *current)
		{
			*current = attackCoef * (*current - tmp) + tmp;
		}
		else
		{
			*current = releaseCoef * (*current - tmp) + tmp;
		}
	}

	double process(double in)
	{
		staticProcess(in, &m_currentValue, m_attackCoef, m_releaseCoef);

		return m_currentValue;
	}

	double getCurrentValue()
	{
		return m_currentValue;
	}

private:
	double m_attackMs;
	double m_releaseMs;
	double m_attackCoef;
	double m_releaseCoef;
	double m_currentValue;
	double m_sampleRate;
};
}}
#endif
