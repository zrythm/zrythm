#include "FloatParameter.h"


FloatParameter::FloatParameter(String name)
{
	this->name = name;
	this->value = 0.0f;
}

FloatParameter::~FloatParameter(void)
{
}

float FloatParameter::getParameter(void)
{
	return value;
}

void FloatParameter::setParameter(float value)
{
	this->value = value;
}

String FloatParameter::getName(void)
{
	return name;
}
