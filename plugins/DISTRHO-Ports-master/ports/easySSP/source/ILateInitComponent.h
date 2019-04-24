/*
  ==============================================================================

    ILateInitComponent.h
    Created: 14 Apr 2014 11:07:57pm
    Author:  Kykc

  ==============================================================================
*/

#ifndef ILATEINITCOMPONENT_H_INCLUDED
#define ILATEINITCOMPONENT_H_INCLUDED


class ILateInitComponent : public Component
{
public:
	virtual void init(juce::Rectangle<int> bounds) = 0;
};


#endif  // ILATEINITCOMPONENT_H_INCLUDED
