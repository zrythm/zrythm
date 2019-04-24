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

#if DROWAUDIO_USE_SOUNDTOUCH



AudioFileDropTarget::AudioFileDropTarget (AudioFilePlayerExt* audioFilePlayerToControl,
                                          Component* componentToAttachTo)
    :  audioFilePlayer (audioFilePlayerToControl),
       attachedComponent (componentToAttachTo),
       dragTested (false),
       interestedInDrag (false),
       bezelColour (Colours::orange)
{
    if (attachedComponent != nullptr)
    {
        attachedComponent->addAndMakeVisible (this);
        attachedComponent->addComponentListener (this);
        attachedComponent->addMouseListener (this, false);
    }
}

AudioFileDropTarget::~AudioFileDropTarget()
{
    if (attachedComponent != nullptr)
    {
        attachedComponent->removeComponentListener (this);
        attachedComponent->removeMouseListener (this);
    }
}

void AudioFileDropTarget::setBezelColour (Colour& newColour)
{
    bezelColour = newColour;
    repaint();
}

//==============================================================================
void AudioFileDropTarget::paint (Graphics& g)
{
    if (interestedInDrag) 
    {
        g.setColour (bezelColour);
        g.drawRect (0, 0, getWidth(), getHeight(), 2);
    }
}

bool AudioFileDropTarget::hitTest (int /*x*/, int /*y*/)
{
    if (! dragTested || interestedInDrag)
        return true;

    return false;
}

void AudioFileDropTarget::mouseEnter (const MouseEvent& /*e*/)
{
    dragTested = true;

    repaint();
}

void AudioFileDropTarget::mouseExit (const MouseEvent& /*e*/)
{
    dragTested = false;
    
    repaint();
}

//==============================================================================
void AudioFileDropTarget::componentMovedOrResized (Component& /*component*/,
                                                   bool /*wasMoved*/,
                                                   bool /*wasResized*/)
{
    if (attachedComponent != nullptr)
        setBounds (attachedComponent->getLocalBounds());
}

//==============================================================================
bool AudioFileDropTarget::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    var description = dragSourceDetails.description;
    
    if (description.isArray())
        description = dragSourceDetails.description[0];

    if (description.isObject())
    {
        ReferenceCountedValueTree::Ptr libraryTree (dynamic_cast<ReferenceCountedValueTree*> (description.getObject()));
        
        if (libraryTree != nullptr && libraryTree->getValueTree().hasType (MusicColumns::libraryItemIdentifier))
        {
            interestedInDrag = true;
            setMouseCursor (MouseCursor::CopyingCursor);
            repaint();
            
            return true;
        }
    }
    
    return false;	
}

void AudioFileDropTarget::itemDragExit (const SourceDetails& /*dragSourceDetails*/)
{
    if (interestedInDrag)
    {
        interestedInDrag = false;
        setMouseCursor (MouseCursor::NormalCursor);	
        
        repaint();
    }
}

void AudioFileDropTarget::itemDropped (const SourceDetails& dragSourceDetails)
{
    if (interestedInDrag && dragSourceDetails.description.isArray()) 
    {
        ReferenceCountedValueTree::Ptr childTree (dynamic_cast<ReferenceCountedValueTree*> (dragSourceDetails.description[0].getObject()));
        if (childTree != nullptr) 
        {
            ValueTree itemTree (childTree->getValueTree());
            File newFile (itemTree.getProperty (MusicColumns::columnNames[MusicColumns::Location]));
            
            if (newFile.existsAsFile()) 
            {
                audioFilePlayer->setLibraryEntry (itemTree);
                audioFilePlayer->setFile (newFile.getFullPathName());
            }
        }
    }
    
    interestedInDrag = false;
    repaint();
}

//==============================================================================
bool AudioFileDropTarget::isInterestedInFileDrag (const StringArray &files)
{
    if (matchesAudioWildcard (File (files[0]).getFileExtension(), 
                              audioFilePlayer->getAudioFormatManager()->getWildcardForAllFormats(), true))
    {
        interestedInDrag = true;
        setMouseCursor (MouseCursor::CopyingCursor);
        
        repaint();

        return true;
    }

    return false;
}

void AudioFileDropTarget::fileDragExit (const StringArray& /*files*/)
{
    if (interestedInDrag)
    {
        interestedInDrag = false;
        setMouseCursor (MouseCursor::NormalCursor);	
        
        repaint();
    }
}

void AudioFileDropTarget::filesDropped (const StringArray &files, int /*x*/, int /*y*/)
{
    if (interestedInDrag)
    {
        interestedInDrag = false;
        audioFilePlayer->setFile (files[0]);
        setMouseCursor (MouseCursor::NormalCursor);
        
        repaint();
    }
}

//==============================================================================



#endif