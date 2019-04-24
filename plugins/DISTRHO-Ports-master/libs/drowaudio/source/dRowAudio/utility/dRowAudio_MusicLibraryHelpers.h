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

#ifndef __DROWAUDIO_MUSICLIBRARYHELPERS_H__
#define __DROWAUDIO_MUSICLIBRARYHELPERS_H__

#include "dRowAudio_Utility.h"

namespace LoopAndCueHelpers
{
    /** Returns the time from a give cue point index in a cue point tree.
        The index starts at 0 and will return 0.0 if the index is out of range.
     */
    inline double getTimeFromCueTree (ValueTree& cueTree, int index)
    {
        if (index < cueTree.getNumProperties())
        {
            const String property(cueTree.getProperty(cueTree.getPropertyName(index)).toString());
            return property.upToFirstOccurrenceOf(",", false, false).getDoubleValue();
        }
        
        return 0.0;
    }
    
    /** Returns the time from a give cue point index in a cue point tree.
        The index starts at 0 and will return white if the index is out of range.
     */
    inline uint32 getColourFromCueTree (ValueTree& cueTree, int index)
    {
        if (index < cueTree.getNumProperties())
        {
            const String property(cueTree.getProperty(cueTree.getPropertyName(index)).toString());
            return (uint32)property.fromLastOccurrenceOf(",", false, false).getLargeIntValue();
        }
        
        return 0xffffffff;
    }
    
    /** Returns the start time, end time and Colour of a give loop point in a loop tree.
        The index starts at 0 and will return 0.0's if the index is out of range.
     */
    inline void getTimeAndColourFromLoopTree (ValueTree& loopTree, int index, double &startTime, double &endTime, uint32& colour)
    {
        if (index < loopTree.getNumProperties())
        {
            const String property(loopTree.getProperty(loopTree.getPropertyName(index)).toString());
            startTime = property.upToFirstOccurrenceOf(",", false, false).getDoubleValue();
            endTime = property.fromFirstOccurrenceOf(",", false, false).upToLastOccurrenceOf(",", false, false).getDoubleValue();
            colour = (uint32)property.fromLastOccurrenceOf(",", false, false).getLargeIntValue();
            return;
        }
        
        startTime = endTime = 0.0;
        colour = 0xffffffff;
        
        return;
    }
}

/**	Details the colums of the table.
 */
namespace MusicColumns {
    
    static const Identifier libraryIdentifier ("MUSICLIBRARY");
    static const Identifier libraryItemIdentifier ("ITEM");
    static const Identifier libraryCuePointIdentifier ("CUE");
    static const Identifier libraryLoopIdentifier ("LOOP");

	enum colums {
		Dummy,
		LibID,
		ID,
		Artist,
		Song,
		Album,
		Rating,
		BPM,
		Genre,
		SubGenre,
		Label,
		Key,
		Length,
		Kind,
		Added,
        Modified,
		Location,
		Score,
		numColumns
	};
	
	static const Identifier columnNames[] = { 
		"Dummy",
		"LibID",
		"ID",						
		"Artist",
		"Song",
		"Album",
		"Rating",
		"BPM",
		"Genre",
		"Sub_Genre",
		"Label",
		"Key",
		"Length",
		"Kind",
		"Added",
        "Modified",
		"Location",
		"Score"
	};
	
	static const UNUSED_NOWARN char *iTunesNames[] = {
		"",
		"",
		"Track ID",
		"Artist",
		"Name",
		"Album",
		"Rating",
		"BPM",
		"Genre",
		"Grouping",
		"Comments",
		"Series",
		"Total Time",
		"Kind",
		"Date Added",
        "Date Modified",
		"Location",
		"Score"
	};
	
	static const int columnWidths[] = {
		1,
		30,
		50,
		150,
		200,
		150,
		60,
		60,
		80,
		100,
		100,
		30,
		80,
		60,
		125,
		125,
		300,
		60
	};
}

#endif  // __DROWAUDIO_MUSICLIBRARYHELPERS_H__