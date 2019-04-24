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

//==============================================================================
UnityBuilder::UnityBuilder()
{
}

UnityBuilder::~UnityBuilder()
{
}

bool UnityBuilder::processDirectory (const File& sourceDirectory)
{
    if (sourceDirectory.isDirectory())
    {
        Array<File> files;
        sourceDirectory.findChildFiles (files, File::findFiles + File::ignoreHiddenFiles, true);
        
        String includeString, sourceString;
        
        for (int i = 0; i < files.size(); ++i)
        {
            bool includeFile = true;
            File& currentFile (files.getReference (i));

            // first check files
            if (! filesToIgnore.contains (currentFile))
            {
                // now check for directories
                for (int i = 0; i < filesToIgnore.size(); ++i)
                {
                    File& currentDir (filesToIgnore.getReference (i));

                    if (currentDir.isDirectory()
                        && currentFile.isAChildOf (currentDir))
                    {
                        includeFile = false;
                        break;
                    }
                }
                
                if (includeFile)
                {
                    const String relativePath (currentFile.getRelativePathFrom (sourceDirectory));

                    if (currentFile.hasFileExtension (".h"))
                        includeString << "#include \"" << relativePath << "\"" << newLine;
                    else if (currentFile.hasFileExtension (".cpp"))
                        sourceString << "#include \"" << relativePath << "\"" << newLine;
                }
            }
        }
        
        // now write the output files
        File outputFile (destinationFile.exists() ? destinationFile : sourceDirectory);
        
        if (outputFile.hasWriteAccess())
        {
            File headerFile, cppFile;
            
            if (outputFile.isDirectory())
            {
                const String baseName ("UnityBuild");
                
                headerFile = outputFile.getNonexistentChildFile (baseName, ".h");
                cppFile = outputFile.getNonexistentChildFile (baseName, ".cpp");
            }
            else
            {
                headerFile = outputFile.getNonexistentSibling().withFileExtension (".h");
                cppFile = outputFile.getNonexistentSibling().withFileExtension (".cpp");
            }

            String headerOutput (preInclusionString);
            String sourceOutput (preInclusionString);
            
            headerOutput << includeString << postInclusionString;
            sourceOutput << "#include \"" << headerFile.getFileName() << "\"" << newLine << newLine
                << sourceString << postInclusionString;
            
            headerFile.replaceWithText (headerOutput);
            cppFile.replaceWithText (sourceOutput);
            
            return true;
        }
    }
    
    return false;
}

void UnityBuilder::setDestinationFile (const File& newDestinationFile)
{
    destinationFile = newDestinationFile;
}

void UnityBuilder::setFilesToIgnore (const Array<File>& filesToIgnore_)
{
    filesToIgnore = filesToIgnore_;
}

void UnityBuilder::setPreAndPostString (const String& preInclusionString_,
                                        const String& postInclusionString_)
{
    preInclusionString = preInclusionString_;
    postInclusionString = postInclusionString_;
}
