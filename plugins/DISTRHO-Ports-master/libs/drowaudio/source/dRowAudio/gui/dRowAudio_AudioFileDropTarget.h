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

#ifndef __DROWAUDIO_AUDIOFILEDROPTARGET_H__
#define __DROWAUDIO_AUDIOFILEDROPTARGET_H__

#if DROWAUDIO_USE_SOUNDTOUCH

#include "../audio/dRowAudio_AudioFilePlayer.h"

//==============================================================================
/** A Component that acts as a drag and drop target for audio files and
    MusicLibraryTable drag sources. This will draw a coloured bezel if it can
    read the drag source provided.
 */
class AudioFileDropTarget : public Component,
                            public ComponentListener,
                            public DragAndDropTarget,
                            public FileDragAndDropTarget
{
public:
    //==============================================================================
    /** Creates an AudioFileDropTarget, which controls an AudioFilePlayer.
        
        If you supply a component to attach itself to, the AudioFileDropTarget
        will automatically position itself around that component and pass any
        mouse events which are not drags onto it.
     
        @see AudioFilePlayer
     */
    AudioFileDropTarget (AudioFilePlayerExt* audioFilePlayerToControl,
                         Component* componentToAttachTo = nullptr);
    
    /** Destructor.
     */
    ~AudioFileDropTarget();
    
    /** Sets the colour of the bezel to be drawn.
     */
    void setBezelColour (Colour& newColour);

    /** Retruns the current bezel colour being used.
     */
    const Colour getBezelColour()            {   return bezelColour; }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g);
    
    /** @internal */
    bool hitTest (int x, int y);

    /** @internal */
    void mouseEnter (const MouseEvent& e);

    /** @internal */
    void mouseExit (const MouseEvent& e);
    
    //==============================================================================
    /** @internal */
    void componentMovedOrResized (Component& component,
                                  bool wasMoved,
                                  bool wasResized);

    //==============================================================================
    /** @internal */
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails);
    /** @internal */
    void itemDragExit (const SourceDetails& dragSourceDetails);
    /** @internal */
    void itemDropped (const SourceDetails& dragSourceDetails);
    
    //==============================================================================
    /** @internal */
    bool isInterestedInFileDrag (const StringArray& files);
    /** @internal */
    void fileDragExit (const StringArray& files);
    /** @internal */
    void filesDropped (const StringArray& files, int x, int y);

private:
    //==============================================================================
    AudioFilePlayerExt* audioFilePlayer;
    SafePointer<Component> attachedComponent;
    bool dragTested, interestedInDrag;
    Colour bezelColour;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileDropTarget);
};

#endif
#endif  // __DROWAUDIO_AUDIOFILEDROPTARGET_H__
