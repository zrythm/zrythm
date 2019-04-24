/*
  ==============================================================================

    Common.h
    Created: 10 Jun 2012 7:30:05pm
    Author:  David Rowland

  ==============================================================================
*/

#ifndef __PLUGIN_HELPERS_H__
#define __PLUGIN_HELPERS_H__

//==============================================================================
namespace PresetHelpers
{
    static void savePreset (AudioProcessor& processor, MemoryBlock& destData) noexcept
    {
        const String presetTitle (String (processor.getName() + " SETTINGS").replaceCharacter (' ', '_'));
        ValueTree preset (presetTitle);
        
        for (int i = 0; i < processor.getNumParameters(); ++i)
        {
            const String storedParamName (processor.getParameterName (i).replaceCharacter (' ', '_'));
            
            preset.setProperty (storedParamName, processor.getParameter (i), nullptr);
        }
        
        MemoryOutputStream stream (destData, false);
        preset.writeToStream (stream);
    }
    
    static void loadPreset (AudioProcessor& processor, const void* data, int sizeInBytes) noexcept
    {
        ValueTree preset (ValueTree::readFromData (data, sizeInBytes));
        
        if (preset.isValid())
        {
            String presetTitle (String (processor.getName() + " SETTINGS").replaceCharacter (' ', '_'));
            
            // make sure that it's actually our type of XML object..
            if (preset.hasType (presetTitle))
            {
                for (int i = 0; i < processor.getNumParameters(); ++i)
                {
                    const String storedParamName (processor.getParameterName (i).replaceCharacter (' ', '_'));
                    const float storedParamValue = (float) preset.getProperty (storedParamName, processor.getParameter (i));
                    
                    processor.setParameter (i, storedParamValue);
                }
            }
        }
    }
}

#endif  // __PLUGIN_HELPERS_H__
