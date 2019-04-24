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

MusicLibraryTable::MusicLibraryTable()
    : font              (12.0f),
      currentLibrary    (nullptr),
      dataList          (MusicColumns::libraryIdentifier),
      filteredNumRows   (dataList.getNumChildren()),
      finishedLoading   (true)
{
	// Create our table component and add it to this component..
	addAndMakeVisible (&table);
    table.setModel (this);
    table.setMultipleSelectionEnabled (true);
//	table.setColour (ListBox::backgroundColourId, Colour::greyLevel (0.2f));
	table.setHeaderHeight (18);
	table.setRowHeight (16);
	table.getViewport()->setScrollBarThickness (10);

	// give it a border
//	table.setColour (ListBox::outlineColourId, Colours::grey);
	table.setOutlineThickness (1);

	// Add some MusicColumns to the table header
    for (int i = 1; i < MusicColumns::numColumns; i++)
    {
        table.getHeader().addColumn (MusicColumns::columnNames[i].toString(),
                                     i,
                                     MusicColumns::columnWidths[i],
                                     50,
                                     800,
                                     TableHeaderComponent::defaultFlags);
    }
        
	// we could now change some initial settings..
	table.getHeader().setSortColumnId (MusicColumns::Artist, true); // sort forwards by the Artist column

	table.getHeader().setColumnVisible (MusicColumns::LibID, false);
	table.getHeader().setColumnVisible (MusicColumns::ID, false);
	table.getHeader().setColumnVisible (MusicColumns::Rating, false);
	table.getHeader().setColumnVisible (MusicColumns::Location, false);
	table.getHeader().setColumnVisible (MusicColumns::Modified, false);
	
	setFilterText (String());
}

MusicLibraryTable::~MusicLibraryTable()
{
	if (currentLibrary != nullptr)
		currentLibrary->removeListener(this);
}

void MusicLibraryTable::setLibraryToUse (ITunesLibrary* library)
{
	currentLibrary = library;
	
	filteredDataList = dataList = library->getLibraryTree();
	dataList = library->getLibraryTree();
	library->addListener(this);
}

void MusicLibraryTable::setFilterText (const String& filterString)
{
    currentFilterText = filterString;
    
    if (currentLibrary != nullptr)
        currentLibrary->getParserLock().enter();
    
	if (filterString.isEmpty())
	{
		filteredDataList = dataList;
		filteredNumRows = filteredDataList.getNumChildren();
	}
	else
	{
		filteredDataList = ValueTree (dataList.getType());
		
		for (int e = 0; e < dataList.getNumChildren(); ++e)
		{
			for (int i = 0; i < dataList.getChild (e).getNumProperties(); i++)
			{
				if (dataList.getChild (e)[MusicColumns::columnNames[i]].toString().containsIgnoreCase (filterString))
				{
					filteredDataList.addChild (dataList.getChild(e).createCopy(), -1, 0);
					break;
				}
			}
		}
		
		filteredNumRows = filteredDataList.getNumChildren();
	}
	
    if (currentLibrary != nullptr)
        currentLibrary->getParserLock().exit();

	table.getHeader().reSortTable();
}

//==============================================================================
void MusicLibraryTable::libraryChanged (ITunesLibrary* library)
{
	if (library == currentLibrary) 
	{
		finishedLoading = false;
		filteredDataList = dataList = currentLibrary->getLibraryTree();
        updateTableFilteredAndSorted();
    }
}

void MusicLibraryTable::libraryUpdated (ITunesLibrary* library)
{
	if (library == currentLibrary) 
        updateTableFilteredAndSorted();
}

void MusicLibraryTable::libraryFinished (ITunesLibrary* library)
{
	if (library == currentLibrary) 
	{
		finishedLoading = true;
        updateTableFilteredAndSorted();
	}
}

//==============================================================================
int MusicLibraryTable::getNumRows()
{
	return filteredNumRows;
}

void MusicLibraryTable::paintRowBackground (Graphics& g, int /*rowNumber*/,
                                            int /*width*/, int /*height*/, bool rowIsSelected)
{
	if (rowIsSelected)
		g.fillAll (defaultColours.findColour (*this, table.hasKeyboardFocus (true) ? selectedBackgroundColourId
                                                                                    : selectedUnfocusedBackgroundColourId));
	else
		g.fillAll (defaultColours.findColour (*this, table.hasKeyboardFocus (true) ? backgroundColourId
                                                                                    : unfocusedBackgroundColourId));
}

void MusicLibraryTable::paintCell (Graphics& g,
								   int rowNumber,
								   int columnId,
								   int width, int height,
								   bool rowIsSelected)
{
    if (table.hasKeyboardFocus (true))
        g.setColour (defaultColours.findColour (*this, rowIsSelected ? selectedTextColourId : textColourId));
    else
        g.setColour (defaultColours.findColour (*this, rowIsSelected ? selectedUnfocusedTextColourId : unfocusedTextColourId));

	g.setFont (font);

    {
        const ScopedLock sl (currentLibrary->getParserLock());
        const ValueTree& rowElement (filteredDataList.getChild (rowNumber));
    
        if (rowElement.isValid())
        {
            String text;
            
            if(columnId == MusicColumns::Length)
                text = secondsToTimeLength (rowElement[MusicColumns::columnNames[columnId]].toString().getIntValue());
            else if(columnId == MusicColumns::Added
                    || columnId == MusicColumns::Modified)
                text = Time (int64 (rowElement[MusicColumns::columnNames[columnId]])).formatted ("%d/%m/%Y - %H:%M");
            else
                text = rowElement[MusicColumns::columnNames[columnId]].toString();
            
            g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }
    }

    if (table.hasKeyboardFocus (true))
        g.setColour (defaultColours.findColour (*this, rowIsSelected ? selectedOutlineColourId : outlineColourId));
    else
        g.setColour (defaultColours.findColour (*this, rowIsSelected ? selectedUnfocusedOutlineColourId : unfocusedOutlineColourId));

	g.fillRect (width - 1, 0, 1, height);
	g.fillRect (0, height - 1, width, 1);
}

void MusicLibraryTable::sortOrderChanged (int newSortColumnId, bool isForwards)
{
    findSelectedRows();
    
	if (newSortColumnId != 0)
	{
        const ScopedLock sl (currentLibrary->getParserLock());
        
		if (newSortColumnId == MusicColumns::Length
			|| newSortColumnId == MusicColumns::BPM
			|| newSortColumnId == MusicColumns::LibID
			|| newSortColumnId == MusicColumns::ID
            || newSortColumnId == MusicColumns::Added
            || newSortColumnId == MusicColumns::Modified)
		{
			ValueTreeComparators::Numerical<double> sorter (MusicColumns::columnNames[newSortColumnId], isForwards);
			filteredDataList.sort (sorter, 0, false);
		}
		else
        {
			ValueTreeComparators::LexicographicWithBackup sorter (MusicColumns::columnNames[newSortColumnId],
                                                                  MusicColumns::columnNames[MusicColumns::LibID],
                                                                  isForwards);
			filteredDataList.sort (sorter, 0, false);
		}

		table.updateContent();
	}

    setSelectedRows();
}

//==========================================================================================
int MusicLibraryTable::getColumnAutoSizeWidth (int columnId)
{
	int widest = 32;

	// find the widest bit of text in this column..
	for (int i = getNumRows(); --i >= 0;)
	{
        {
            const ScopedLock sl (currentLibrary->getParserLock());
            const ValueTree& rowElement (filteredDataList.getChild (i));

            if (rowElement.isValid())
            {
                const String text (rowElement[MusicColumns::columnNames[columnId]].toString());
                widest = jmax (widest, font.getStringWidth (text));
            }
        }
	}

	return widest + 8;
}

//==============================================================================
void MusicLibraryTable::resized()
{
	table.setBounds (getLocalBounds());
}

void MusicLibraryTable::focusOfChildComponentChanged (FocusChangeType /*cause*/)
{
	repaint();
}

var MusicLibraryTable::getDragSourceDescription (const SparseSet<int>& currentlySelectedRows)
{
    var itemsArray;

	if(! currentlySelectedRows.isEmpty())
	{
        for (int i = 0; i < currentlySelectedRows.size(); ++i)
        {
            const ScopedLock sl (currentLibrary->getParserLock());

            // get child from main tree with same LibID
            const ValueTree& tree (filteredDataList.getChild (currentlySelectedRows[i]));

            ReferenceCountedValueTree::Ptr childTree = new ReferenceCountedValueTree (tree);
            itemsArray.append (childTree.getObject());
        }
	}
    
    return itemsArray;
}

//==============================================================================
void MusicLibraryTable::updateTableFilteredAndSorted()
{
    // make sure we still apply our filter
    // this will also re-sort and update the table
    setFilterText (currentFilterText);
}

void MusicLibraryTable::findSelectedRows()
{
    selectedRowsLibIds.clear();
    const SparseSet<int> selectedRowNumbers (table.getSelectedRows());
    
    for (int i = 0; i < selectedRowNumbers.size(); ++i)
    {
        const int oldIndex = selectedRowNumbers[i];
        const int libId = int (filteredDataList.getChild (oldIndex)[MusicColumns::columnNames[MusicColumns::LibID]]);
        selectedRowsLibIds.add (libId);
    }
}

void MusicLibraryTable::setSelectedRows()
{
    SparseSet<int> newSelectedRowNumbers;
    
    for (int i = 0; i < selectedRowsLibIds.size(); ++i)
    {
        const int libId = selectedRowsLibIds.getReference (i);
        const int index = filteredDataList.indexOf (filteredDataList.getChildWithProperty (MusicColumns::columnNames[MusicColumns::LibID],
                                                                                           libId));
        newSelectedRowNumbers.addRange (Range<int> (index, index + 1));
    }
    
    table.setSelectedRows (newSelectedRowNumbers, sendNotification);
}
