
#include "juce_PluginHeaders.h"
#include "KeyboardButton.h"

KeyboardButton::KeyboardButton(const String& buttonText):
	RotatingToggleButton(buttonText), parentHeight(0)
{
	//ctor
}

KeyboardButton::~KeyboardButton()
{
	//dtor
}

void KeyboardButton::parentHierarchyChanged()
{
	Component *parent= getParentComponent();
	if(parent && (!parentHeight)) parentHeight= parent->getHeight();
	RotatingToggleButton::parentHierarchyChanged();
}

void KeyboardButton::setRotationState(float rotState)
{
	RotatingToggleButton::setRotationState(rotState);
	Component *parent= getParentComponent(); //->getParentComponent();
	if(parentHeight)
	parent->setSize(parent->getWidth(), parentHeight-(1.0-rotationState)*56);
}
