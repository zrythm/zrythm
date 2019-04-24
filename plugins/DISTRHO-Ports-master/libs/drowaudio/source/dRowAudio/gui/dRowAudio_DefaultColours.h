/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#ifndef __DROWAUDIO_DEFAULTCOLOURS_H__
#define __DROWAUDIO_DEFAULTCOLOURS_H__

//==============================================================================
/*
    This class is used internally by the module to handle it's own colourIds and
    maintain compatibility with the normal JUCE setColour etc. methods. You
    should never have to use this yourself.
 */
class DefaultColours
{
public:
    //==============================================================================
    DefaultColours()
    {
        fillDefaultColours();
    }

    Colour findColour (const Component& source, const int colourId) const noexcept
    {
        if (source.isColourSpecified (colourId))
            return source.findColour (colourId);
        else if (source.getLookAndFeel().isColourSpecified (colourId))
            return source.getLookAndFeel().findColour (colourId);
        else
            return colourIds.contains (colourId) ? colours[colourIds.indexOf (colourId)]
                                                    : Colours::black;
    }
    
private:
    //==============================================================================
    Array <int> colourIds;
    Array <Colour> colours;
    
    //==============================================================================
    void fillDefaultColours() noexcept
    {
        static const uint32 standardColours[] =
        {
            MusicLibraryTable::backgroundColourId,                      Colour::greyLevel (0.2f).getARGB(),
            MusicLibraryTable::unfocusedBackgroundColourId,             Colour::greyLevel (0.2f).getARGB(),
            MusicLibraryTable::selectedBackgroundColourId,              0xffff8c00,//Colour (Colours::darkorange).getARGB(),
            MusicLibraryTable::selectedUnfocusedBackgroundColourId,     Colour::greyLevel (0.6f).getARGB(),
            MusicLibraryTable::textColourId,                            Colour::greyLevel (0.9f).getARGB(),
            MusicLibraryTable::selectedTextColourId,                    Colour::greyLevel (0.2f).getARGB(),
            MusicLibraryTable::unfocusedTextColourId,                   Colour::greyLevel (0.9f).getARGB(),
            MusicLibraryTable::selectedUnfocusedTextColourId,           Colour::greyLevel (0.9f).getARGB(),
            MusicLibraryTable::outlineColourId,                         Colour::greyLevel (0.9f).withAlpha (0.2f).getARGB(),
            MusicLibraryTable::selectedOutlineColourId,                 Colour::greyLevel (0.9f).withAlpha (0.2f).getARGB(),
            MusicLibraryTable::unfocusedOutlineColourId,                Colour::greyLevel (0.9f).withAlpha (0.2f).getARGB(),
            MusicLibraryTable::selectedUnfocusedOutlineColourId,        Colour::greyLevel (0.9f).withAlpha (0.2f).getARGB()
        };
        
        for (int i = 0; i < numElementsInArray (standardColours); i += 2)
        {
            colourIds.add ((int) standardColours [i]);
            colours.add (Colour ((uint32) standardColours [i + 1]));
        }
    }
};

static const DefaultColours defaultColours;


#endif // __DROWAUDIO_DEFAULTCOLOURS_H__
