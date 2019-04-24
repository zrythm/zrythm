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


ITunesLibraryParser::ITunesLibraryParser (const File& iTunesLibraryFileToUse, const ValueTree& elementToFill,
                                          const CriticalSection& lockToUse)
    : Thread ("iTunesLibraryParser"),
      lock (lockToUse),
      iTunesLibraryFile (iTunesLibraryFileToUse),
      treeToFill (elementToFill),
      numAdded (0),
      finished (false)
{
	startThread (1);
}

ITunesLibraryParser::~ITunesLibraryParser()
{
	stopThread (5000);
}

void ITunesLibraryParser::run()
{
    // parse the iTunes xml first
    iTunesDatabase = XmlDocument::parse (iTunesLibraryFile);
    if (! iTunesDatabase->hasTagName ("plist")
        || iTunesDatabase->getStringAttribute ("version") != "1.0")
    {
        jassertfalse; // not a vlid iTunesLibrary file!
        finished = true;
        return;
    }
	
    // find start of tracks library
    iTunesLibraryTracks = iTunesDatabase->getFirstChildElement()->getChildByName ("dict");
    currentElement = iTunesLibraryTracks->getFirstChildElement();
    
    // find any existing elements
    Array<int> existingIds;
    Array<ValueTree> existingItems;

    if (! threadShouldExit())
    {
        const ScopedLock sl (lock);

        if (treeToFill.hasType (MusicColumns::libraryIdentifier))
        {
            for (int i = 0; i < treeToFill.getNumChildren(); ++i)
            {
                ValueTree currentItem (treeToFill.getChild (i));
                int idOfChild = int (currentItem.getProperty (MusicColumns::columnNames[MusicColumns::ID]));
                
                existingIds.add (idOfChild);
                existingItems.add (currentItem);
            }
        }
    }

	while (! threadShouldExit())
	{
        int currentItemId = -1;
        ValueTree newElement;

        bool alreadyExists = false;
        bool needToModify = false;
        bool isAudioFile = false;
        
        while (currentElement != nullptr)
        {
            if (currentElement->getTagName() == "key")
            {
                currentItemId = currentElement->getAllSubText().getIntValue();  // e.g. <key>13452</key>
                alreadyExists = existingIds.contains (currentItemId);
                
                if (alreadyExists)
                {
                    // first get the relevant tree item
                    lock.enter();
                    const int index = existingIds.indexOf (currentItemId);
                    ValueTree existingElement (existingItems.getUnchecked (index));
                    lock.exit();

                    // and then check the modification dates
                    XmlElement* trackDetails = currentElement->getNextElement(); // move on to the <dict>
                    
                    forEachXmlChildElement (*trackDetails, e)
                    {
                        if (e->getAllSubText() == MusicColumns::iTunesNames[MusicColumns::Modified])
                        {
                            const int64 newModifiedTime = parseITunesDateString (e->getNextElement()->getAllSubText()).toMilliseconds();
                            const int64 currentModifiedTime = int64 (existingElement.getProperty (MusicColumns::columnNames[MusicColumns::Modified]));
                                                        
                            if (newModifiedTime > currentModifiedTime)
                            {
                                const ScopedLock sl (lock);
                                treeToFill.removeChild (existingElement, nullptr);
                                needToModify = true;
                            }
                            
                            break;
                        }
                    }
                }

                newElement = ValueTree (MusicColumns::libraryItemIdentifier);
                newElement.setProperty (MusicColumns::columnNames[MusicColumns::ID], currentItemId, nullptr);
            }

            currentElement = currentElement->getNextElement(); // move on to the <dict>
            
            if (! alreadyExists || needToModify)
                break;
        }
        
        // check to see if we have reached the end
        if (currentElement == nullptr)
		{
			finished = true;
			signalThreadShouldExit();
            break;
        }            
        
		if (currentElement->getTagName() == "dict")
		{
            // cycle through items of each track
			forEachXmlChildElement (*currentElement, e2)
			{	
                const String elementKey (e2->getAllSubText());
                //const String elementValue (e2->getNextElement()->getAllSubText());
                
				if (elementKey == "Kind")
                {
					if (e2->getNextElement()->getAllSubText().contains ("audio file"))
					{
                        isAudioFile = true;
						newElement.setProperty (MusicColumns::columnNames[MusicColumns::LibID], numAdded, nullptr);
						numAdded++;
					}
				}
				else if (elementKey == "Track Type")
                {
                    // this is a file in iCloud, not a local, readable one
					if (e2->getNextElement()->getAllSubText().contains ("Remote"))
					{
                        isAudioFile = false;
                        break;
					}
				}
				
                // and check the entry against each column
				for(int i = 2; i < MusicColumns::numColumns; i++)
				{					
					if (elementKey == MusicColumns::iTunesNames[i])
					{
                        const String elementValue = e2->getNextElement()->getAllSubText();
						
						if (i == MusicColumns::Length
							|| i == MusicColumns::BPM
							|| i == MusicColumns::LibID)
						{
							newElement.setProperty (MusicColumns::columnNames[i], elementValue.getIntValue(), nullptr);
						}
                        else if (i == MusicColumns::Added
                                 || i == MusicColumns::Modified)
                        {            
                            const int64 timeInMilliseconds (parseITunesDateString (elementValue).toMilliseconds());
                            newElement.setProperty (MusicColumns::columnNames[i], timeInMilliseconds, nullptr);
                        }
						else
						{
                            String textEntry (elementValue);
                            
							if (i == MusicColumns::Location)
								textEntry = stripFileProtocolForLocal (elementValue);

							newElement.setProperty (MusicColumns::columnNames[i], textEntry, nullptr);
						}
					}
				}
			}

			if (isAudioFile == true)
            {
				const ScopedLock sl (lock);
				treeToFill.addChild (newElement, -1, nullptr);
			}
            
            currentElement = currentElement->getNextElement(); // move to next track
		}		
	}
}
