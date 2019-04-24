#pragma once
#include "FloatParameter.h"
class IntFloatParameter :
	public FloatParameter
{
public:
	IntFloatParameter(String name, int min, int max);
	~IntFloatParameter(void);
	String getParameterText(void);
	int getParameterValue(void);
	void setParameterValue(int i);
private:
	int min;
	int max;
};

