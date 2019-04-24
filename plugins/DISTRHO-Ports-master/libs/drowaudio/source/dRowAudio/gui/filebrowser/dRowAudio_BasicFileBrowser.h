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

#ifndef __DROWAUDIO_BASICFILEBROWSER_H__
#define __DROWAUDIO_BASICFILEBROWSER_H__

//==================================================================================
/** A BasicFileBrowser with an optional corner resizer.
    
    This is very similar to a FileBrowserComponent expect it does not have the file
    list box, go up button etc.
 */
class  BasicFileBrowser		:	public Component,
								private FileBrowserListener,
                                private FileFilter
{
public:
    //==============================================================================
    /** Various options for the browser.
     
        A combination of these is passed into the FileBrowserComponent constructor.
     */
    enum FileChooserFlags
    {
        openMode                = 1,    /**< specifies that the component should allow the user to
                                         choose an existing file with the intention of opening it. */
        saveMode                = 2,    /**< specifies that the component should allow the user to specify
                                         the name of a file that will be used to save something. */
        canSelectFiles          = 4,    /**< specifies that the user can select files (can be used in
                                         conjunction with canSelectDirectories). */
        canSelectDirectories    = 8,    /**< specifies that the user can select directories (can be used in
                                         conjuction with canSelectFiles). */
        canSelectMultipleItems  = 16,   /**< specifies that the user can select multiple items. */
        useTreeView             = 32,   /**< specifies that a tree-view should be shown instead of a file list. */
        filenameBoxIsReadOnly   = 64    /**< specifies that the user can't type directly into the filename box. */
    };
	
    //==============================================================================
    /** Creates a BasicFileBrowser.
	 
        @param browserMode              The intended purpose for the browser - see the
        FileChooserMode enum for the various options
        @param initialFileOrDirectory   The file or directory that should be selected when
        the component begins. If this is File(),
        a default directory will be chosen.
        @param fileFilter               an optional filter to use to determine which files
        are shown. If this is 0 then all files are displayed. Note
        that a pointer is kept internally to this object, so
        make sure that it is not deleted before the browser object
        is deleted.
	 */
    BasicFileBrowser (int flags,
                      const File& initialFileOrDirectory,
                      const FileFilter* fileFilter);
	
    /** Destructor. */
    ~BasicFileBrowser();
	
    //==============================================================================
    /** Returns the number of files that the user has got selected.
        If multiple select isn't active, this will only be 0 or 1. To get the complete
        list of files they've chosen, pass an index to getCurrentFile().
     */
    int getNumSelectedFiles() const noexcept;
    
    /** Returns one of the files that the user has chosen.
        If the box has multi-select enabled, the index lets you specify which of the files
        to get - see getNumSelectedFiles() to find out how many files were chosen.
        @see getHighlightedFile
     */
    File getSelectedFile (int index) const noexcept;
    
    /** Deselects any files that are currently selected.
     */
    void deselectAllFiles();
    
    /** Returns true if the currently selected file(s) are usable.
     
        This can be used to decide whether the user can press "ok" for the
        current file. What it does depends on the mode, so for example in an "open"
        mode, this only returns true if a file has been selected and if it exists.
        In a "save" mode, a non-existent file would also be valid.
     */
    bool currentFileIsValid() const;
    
    /** This returns the last item in the view that the user has highlighted.
        This may be different from getCurrentFile(), which returns the value
        that is shown in the filename box, and if there are multiple selections,
        this will only return one of them.
        @see getSelectedFile
     */
    File getHighlightedFile() const noexcept;
    
    //==============================================================================
    /** Returns the directory whose contents are currently being shown in the listbox. */
    const File& getRoot() const;
    
    /** Changes the directory that's being shown in the listbox. */
    void setRoot (const File& newRootDirectory);
    
    /** Refreshes the directory that's currently being listed. */
    void refresh();
    
    /** Changes the filter that's being used to sift the files. */
    void setFileFilter (const FileFilter* newFileFilter);
    
    /** Returns a verb to describe what should happen when the file is accepted.
     
        E.g. if browsing in "load file" mode, this will be "Open", if in "save file"
        mode, it'll be "Save", etc.
     */
    virtual String getActionVerb() const;
    
    /** Returns true if the saveMode flag was set when this component was created.
     */
    bool isSaveMode() const noexcept;
    
    //==============================================================================
    /** Adds a listener to be told when the user selects and clicks on files.
        @see removeListener
     */
    void addListener (FileBrowserListener* listener);
    
    /** Removes a listener.
        @see addListener
     */
    void removeListener (FileBrowserListener* listener);
        
	//==============================================================================
	/** Enables the column resizer.
     */
	void setResizeEnable(bool enableResize)
    {	
		showResizer = enableResize;
	}
    
    /** Returns true if the resizer is enabled.
     */
	bool getResizeEnabled()					{	return showResizer;			}
	
    /** Returns the width for the longest item in the list.
     */
	int getLongestWidth();
    
    //==============================================================================
    /** @internal */
    void resized();
    /** @internal */
    bool keyPressed (const KeyPress& key);
    /** @internal */
    void mouseDoubleClick (const MouseEvent &e);
    /** @internal */
    void selectionChanged();
    /** @internal */
    void fileClicked (const File& f, const MouseEvent& e);
    /** @internal */
    void fileDoubleClicked (const File& f);
    /** @internal */
    void browserRootChanged (const File&);
    /** @internal */
    bool isFileSuitable (const File&) const;
    /** @internal */
    bool isDirectorySuitable (const File&) const;

    /** @internal */
    DirectoryContentsDisplayComponent* getDisplayComponent() const noexcept;
    
private:
    //==============================================================================
    ScopedPointer <DirectoryContentsList> fileList;
    const FileFilter* fileFilter;
    
    int flags;
    File currentRoot;
    Array<File> chosenFiles;
    ListenerList <FileBrowserListener> listeners;
    
    ScopedPointer<DirectoryContentsDisplayComponent> fileListComponent;
    
    TimeSliceThread thread;
    
    void sendListenerChangeMessage();
    bool isFileOrDirSuitable (const File& f) const;
	
	bool showResizer;
	ScopedPointer<ResizableCornerComponent> resizer;
	ComponentBoundsConstrainer resizeLimits;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicFileBrowser);
};

#endif //__DROWAUDIO_BASICFILEBROWSER_H__