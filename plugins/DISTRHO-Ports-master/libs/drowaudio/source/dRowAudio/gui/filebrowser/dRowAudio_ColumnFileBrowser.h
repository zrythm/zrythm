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

#ifndef __DROWAUDIO_COLUMNFILEBROWSER_H__
#define __DROWAUDIO_COLUMNFILEBROWSER_H__

#include "dRowAudio_BasicFileBrowser.h"

class ColumnFileBrowserContents;
class ColumnFileBrowserLookAndFeel;

//==================================================================================
/** A type of FileBrowser persented in columns.
 
    This is similar to OSX's Finder column view. You can navigate around the columns 
    using the keyboard or the mouse. Highligting a number of files and then dragging 
    them will perform an external drag and drop procedure.
 */
class ColumnFileBrowser :	public Viewport
{
public:
    //==================================================================================
    /** Creates a ColumnFileBrowser with a given file filter.
     */
	ColumnFileBrowser (WildcardFileFilter* filesToDisplay);
	
    /** Destructor.
     */
	~ColumnFileBrowser();
	
    /** Sets the highlight colour for the active column.
        For the rest of the colours, use the normal DirectoryContentsDisplayComponent
        colourIds.
     */
    void setActiveColumHighlightColour (const Colour& colour);
    
    /** @internal */
	void resized();
		
    /** @internal */
	void visibleAreaChanged (const Rectangle<int>& newVisibleArea);
	
    /** @internal */
	void mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel);
	
private:
    //==================================================================================
	ScopedPointer<WildcardFileFilter> wildcard;
	ColumnFileBrowserContents* fileBrowser;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColumnFileBrowser);
};

#endif //__DROWAUDIO_COLUMNFILEBROWSER_H__