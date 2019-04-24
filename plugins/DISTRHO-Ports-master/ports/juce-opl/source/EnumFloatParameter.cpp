#include "EnumFloatParameter.h"


EnumFloatParameter::EnumFloatParameter(String name, StringArray values)
:FloatParameter(name)
{
	this->values = values;
}


EnumFloatParameter::~EnumFloatParameter(void)
{
}

int EnumFloatParameter::getParameterIndex(void)
{
	int i = (int)(this->value * values.size() + 0.5f);
	if (i >= values.size())
		i = values.size() - 1;
	return i;
}

void EnumFloatParameter::setParameterIndex(int i)
{
	this->value = (float)i/(float)values.size();
	if (this->value < 0.0f)
		this->value = 0.0f;
	else if (this->value > 1.0f)
		this->value = 1.0f;
}

String EnumFloatParameter::getParameterText(void)
{
	return values[this->getParameterIndex()];
}
