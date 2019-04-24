#ifndef __PARAMETERS__
#define __PARAMETERS__

#include "JuceHeader.h"

class Parameters
{
public:

	Parameters(const String& id_)
		: id(id_)
	{
	}

	virtual ~Parameters()
	{
	}

	virtual void setParam(int index, double val) = 0;
	virtual double getParam(int index) = 0;
	void setParamNormalized(int index, float val)
	{
		setParam(index, normalizedToPlain(index, val));
	}

	float getParamNormalized(int index)
	{
		return plainToNormalized(index, getParam(index));
	}

	String getParamName(int index)
	{
		return paramNames[index];
	}

	Range<double> getParamRange(int index)
	{
		return Range<double> (mins[index], maxs[index]);
	}

	double getSkew(int index)
	{
		return skews[index];
	}

	double getResetValue(int index)
	{
		return resets[index];
	}

	void addParameter(int paramIndex, const String& newParam, double minVal, double maxVal, double valueAtMidPoint, double resetVal = -1e10)
	{
		jassert(paramIndex == paramNames.size());
		paramNames.add(newParam);
		
		mins.add(minVal);
		maxs.add(maxVal);

		const bool enableSkew = valueAtMidPoint>0 && minVal>0 && maxVal>0;
		const double skew = enableSkew ? log (0.5) / log ((valueAtMidPoint - minVal) / (maxVal - minVal)) : 1;
		skews.add(skew);

		resets.add(resetVal == -1e10 ? normalizedToPlain(paramIndex, 0.5) : resetVal);

		setParam(paramIndex, resetVal);
	}

	double normalizedToPlain(int index, double val)
	{
		const double minVal = mins[index];
		const double maxVal = maxs[index];
		const double skew = skews[index];
		return minVal + (maxVal - minVal) * exp (log (val) / skew);
	}

	float plainToNormalized(int index, double val)
	{
		const double minVal = mins[index];
		const double maxVal = maxs[index];
		const double skew = skews[index];

    const double n = (val - minVal) / (maxVal - minVal);
    return (float) (skew == 1.0 ? n : pow (n, skew));
	}

	int getNumParameters()
	{
		return paramNames.size();
	}

	virtual XmlElement* getState()
	{
		XmlElement* xml = new XmlElement(id);

		for (int i=0; i<getNumParameters(); ++i)
			xml->setAttribute(getParamName(i), getParam(i));

		return xml;
	}

	virtual void setState(XmlElement* xml)
	{
		jassert(xml != 0);

		if (xml == 0)
			return;

		if (! xml->hasTagName(id))
		{
			forEachXmlChildElement(*xml, e)
			{
				XmlElement* newXml = e->hasTagName(id) ? e : e->getChildByName(id);

				if (newXml != 0)
				{
					xml = newXml;
					break;
				}
			}
		}

		for (int i=0; i<getNumParameters(); ++i)
			setParam(i, (float) xml->getDoubleAttribute(getParamName(i), getResetValue(i)));
	}

	String getId()
	{
		return id;
	}

	virtual String getParamText(int index)
	{
		return String(getParam(index), 3);
	}

	virtual String getParamUnit(int /*index*/)
	{
		return String();
	}

private:
	const String id;

	StringArray paramNames;
	Array<double> mins;
	Array<double> maxs;
	Array<double> skews;
	Array<double> resets;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Parameters);


};



#endif // __PARAMETERS__