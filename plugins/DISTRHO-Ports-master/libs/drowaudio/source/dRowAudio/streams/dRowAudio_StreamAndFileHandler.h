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

#ifndef __DROWAUDIO_STREAMANDFILEHANDLER_H__
#define __DROWAUDIO_STREAMANDFILEHANDLER_H__

#include "dRowAudio_MemoryInputSource.h"

//==============================================================================
/**
    Abstract class which just keeps track of what type of source was last assigned.
    Notes this doesn't take any ownership so make sure you delete the streams and call 
    clear once you do to avoid any dangling pointers.
 
    @see AudioFilePlayer
 */
class StreamAndFileHandler
{
public:
    //==============================================================================
    /** An enum to distinguish between the different input types. */
    enum InputType
    {
        file,
        memoryBlock,
        memoryInputStream,
        unknownStream,
        noInput
    };
    
    //==============================================================================
	/** Creates an empty StreamAndFileHandler. */
	StreamAndFileHandler()
        : inputType (noInput),
          inputStream (nullptr)
    {
    }

	/** Destructor. */
	virtual ~StreamAndFileHandler()                     {}
	
    //==============================================================================
    /** Clears all the internal references to any files or streams. */
    void clear()
    {
        inputType = noInput;
        currentFile = File();
        inputStream = nullptr;
    }
    
    /** Returns the type of input that was last used. */
    InputType getInputType() const noexcept             { return inputType; }
    
    /** Sets the source to be any kind of InputStream.
     
        @returns true if the stream loaded correctly
     */
    bool setInputStream (InputStream* inputStream)
    {
        inputType = unknownStream;
        
        if (MemoryInputStream* mis = dynamic_cast<MemoryInputStream*> (inputStream))
            return setMemoryInputStream (mis);
        
        if (FileInputStream* fis = dynamic_cast<FileInputStream*> (inputStream))
        {
            const ScopedPointer<FileInputStream> deleter (fis);
            return setFile (fis->getFile());
        }
        
        return streamChanged (inputStream);
    }
    
    /** Returns a stream to the current source, you can find this out using getInputType().
     
        It is the caller's responsibility to delete this stream unless it has the
        type unknownStream which it can't make a copy of. You could use a
        dynamic_cast to do this yourself if you know the type.
     */
    InputStream* getInputStream()
    {
        switch (inputType)
        {
            case file:
            {
                return new FileInputStream (currentFile);
            }
            case memoryBlock:
            case memoryInputStream:
            {
                MemoryInputStream* memoryStream = dynamic_cast<MemoryInputStream*> (inputStream);
                
                if (memoryStream != nullptr)
                    return new MemoryInputStream (memoryStream->getData(), memoryStream->getDataSize(), false);
                else
                    return nullptr;
            }
            case unknownStream:
            {
                return inputStream;
            }
            default:
            {
                return nullptr;
            }
        }
    }

    /** Returns an InputSource to the current stream if it knows the type of stream.
        For example, if the input is a file this will return a FileInputStream etc.
        It is the callers responsibility to delete this source when finished.
     */
    InputSource* getInputSource()
    {
        switch (inputType)
        {
            case file:
            {
                return new FileInputSource (currentFile);
            }
            case memoryBlock:
            case memoryInputStream:
            {
                MemoryInputStream* memoryStream = dynamic_cast<MemoryInputStream*> (getInputStream());
                
                if (memoryStream != nullptr)
                    return new MemoryInputSource (memoryStream);
                else
                    return nullptr;
            }
            default:
            {
                return nullptr;
            }
        }
    }

    //==============================================================================
	/** Sets the source to a File.
        @returns true if the file loaded correctly
     */
	bool setFile (const File& newFile)
    {
        inputType = file;
        inputStream = nullptr;
        currentFile = newFile;
        
        return fileChanged (currentFile);
    }
    
    /** Sets the source to a MemoryInputStream.
        @returns true if the stream loaded correctly
     */
    bool setMemoryInputStream (MemoryInputStream* newMemoryInputStream)
    {
        inputType = memoryInputStream;
        currentFile = File();
        inputStream = newMemoryInputStream;
        
        return streamChanged (inputStream);
    }
    
    /** Sets the source to a memory block.
        @returns true if the block data loaded correctly
     */
    bool setMemoryBlock (MemoryBlock& inputBlock)
    {
        inputType = memoryBlock;
        currentFile = File();
        inputStream = new MemoryInputStream (inputBlock, false);
        
        return streamChanged (inputStream);
    }
    
	/** Returns the current file if it was set with a one.
        If a stream was used this will return File::nonexistant.
     */
	File getFile() const noexcept                       { return currentFile; }
    
    //==============================================================================
    /** Subclasses must override this to be informed of when a file changes.
     
        @returns true if the file was able to be loaded correctly
     */
    virtual bool fileChanged (const File& file) = 0;

    /** Subclasses must override this to be informed of when a stream changes.
        Note that this class doesn't retain any ownership of the stream so subclasses should
        delete them when no longer needed. This obviously means the getInputStream method is 
        only valid for the duration that this stream is kept alive. Be sure to set this to 
        nullptr if you delete the stream by any means other than in this class.
     
        @returns true if the stream was able to be loaded correctly
     */
    virtual bool streamChanged (InputStream* inputStream) = 0;

private:
    //==============================================================================
    InputType inputType;
	File currentFile;
    InputStream* inputStream;
    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StreamAndFileHandler)
};

#endif // __DROWAUDIO_STREAMANDFILEHANDLER_H__
