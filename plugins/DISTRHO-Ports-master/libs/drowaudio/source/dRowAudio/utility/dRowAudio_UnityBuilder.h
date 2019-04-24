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

#ifndef __DROWAUDIO_UNITYBUILDER_H__
#define __DROWAUDIO_UNITYBUILDER_H__


//==============================================================================
/**
    UnityBuilder class.

    This is a helper class used to generate "unity build" files for quick
    compilation of projects. This class will take a source directory, scan it
    for all .h and .cpp files and create two build files which simply include
    their contents. This is very similar to how the JUCE modular system works
    and can be used to include source files in other projects and drastically
    speed up build times.
 
    If you need to set custom defines or pragmas use the setPreAndPostString method.
 */
class UnityBuilder
{
public:
    //==============================================================================
    /** Creates a default UnityBuilder.
     
        Use the processDirectory method to actually perform the file generation.
        If you need additional options call the other set-up methods before process
        processDirectory;
     */
    UnityBuilder();
    
    /** Destructor. */
    ~UnityBuilder();

    /** Processes a directory for all .h and .cpp files and generates a unity header
        and cpp file.
        
        If the destination file has not been set this will generate files in the
        source directory named UnityBuild.h and UnityBuild.cpp.
        Note that this will not delete any exisiting files, if the target files
        already exist non-existent ones will be created with numbers in brackets.
     
        @param sourceDirectory  The source directory to "unify".
     */
    bool processDirectory (const File& sourceDirectory);
    
    /** Sets the location and name of the output unity build files.
        Note that the extension of this file is not taken into account and will
        always generate a header and cpp file if needed.
        
        @param newDestinationFile   The output location for the unity build files.
     */
    void setDestinationFile (const File& newDestinationFile);
    
    /** Sets a number of files to ignore.
        This may be useful of you have some working files that you don't currently
        want included in the unity build.
        Note that these Files can be directories and if so none of their child
        files will be included.
        
        @param filesToIgnore    An arry of files ot ignore.
     */
    void setFilesToIgnore (const Array<File>& filesToIgnore);
    
    /** This enables you to include a block of text before and after the source 
        files. This can be handy if you need extra defines or pragmas.
     
        @param preInclusionString   The text to include before the files.
        @param postInclusionString  The text to include after the files.
     */
    void setPreAndPostString (const String& preInclusionString,
                              const String& postInclusionString);
    
private:
    //==============================================================================
    String preInclusionString, postInclusionString;
    Array<File> filesToIgnore;
    File destinationFile;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnityBuilder);
};


#endif  // __DROWAUDIO_UNITYBUILDER_H__
