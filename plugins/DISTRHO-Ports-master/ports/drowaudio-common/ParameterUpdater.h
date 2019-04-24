/*
  ==============================================================================

    Common.h
    Created: 10 Jun 2012 7:30:05pm
    Author:  David Rowland

  ==============================================================================
*/

#ifndef __PARAMETERUPDATER_H__
#define __PARAMETERUPDATER_H__

//==============================================================================
class ParameterUpdater
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() {}
        
        virtual void parameterUpdated (int /*index*/) {}
    };

    //==============================================================================
    ParameterUpdater (Listener& owner)
        : listener (owner)
    {
    }
    
    ~ParameterUpdater() {}
    
    void addParameter (int index)
    {
        paramtersToUpdate.addIfNotAlreadyThere (index);
    }
    
    void dispatchParameters()
    {
        while (paramtersToUpdate.size() > 0)
            listener.parameterUpdated (paramtersToUpdate.removeAndReturn (paramtersToUpdate.size() - 1));
    }
    
private:
    //==============================================================================
    Array<int, CriticalSection> paramtersToUpdate;
    Listener& listener;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterUpdater);
};

#endif  // __PARAMETERUPDATER_H__
