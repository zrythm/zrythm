#pragma once
#include "JuceHeader.h"

class FloatParameter
{
public:
	FloatParameter(String name);
	virtual ~FloatParameter(void);
	float getParameter(void);
	void setParameter(float value);
	String getName(void);
	virtual String getParameterText(void) = 0;
protected:	
	float value;
private:
	String name;
};
