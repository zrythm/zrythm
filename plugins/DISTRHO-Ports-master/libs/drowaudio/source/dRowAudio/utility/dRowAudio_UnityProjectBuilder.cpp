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
const Identifier UnityProjectBuilder::Ids::jucerProject     ("JUCERPROJECT");
const Identifier UnityProjectBuilder::Ids::exportFormats    ("EXPORTFORMATS");
const Identifier UnityProjectBuilder::Ids::mainGroup        ("MAINGROUP");
const Identifier UnityProjectBuilder::Ids::group            ("GROUP");
const Identifier UnityProjectBuilder::Ids::file             ("FILE");
const Identifier UnityProjectBuilder::Ids::idProp           ("id");
const Identifier UnityProjectBuilder::Ids::nameProp         ("name");
const Identifier UnityProjectBuilder::Ids::compileProp      ("compile");
const Identifier UnityProjectBuilder::Ids::fileProp         ("file");
const Identifier UnityProjectBuilder::Ids::resourceProp     ("resource");
const Identifier UnityProjectBuilder::Ids::targetFolderProp ("targetFolder");

//==============================================================================
UnityProjectBuilder::UnityProjectBuilder (const File& sourceProject)
    : projectFile   (sourceProject),
      numFiles      (1),
      shouldLog     (false),
      unityName     ("UnityBuild")
{
}

UnityProjectBuilder::~UnityProjectBuilder()
{
}

bool UnityProjectBuilder::run()
{
    logOutput ("Starting parse of \"" + projectFile.getFullPathName() + "\"...");
    project = readValueTreeFromFile (projectFile);

    if (! project.hasType (Ids::jucerProject))
    {
        logOutput ("ERROR: Invalid project, exiting");
        return false;
    }
    
    logOutput ("Valid project found...");
    ValueTree mainGroupTree (project.getChildWithName (Ids::mainGroup));
    
    if (! mainGroupTree.isValid())
    {
        logOutput ("ERROR: Empty project, exiting");
        return false;
    }
    
    updateBuildDirectories();
    
    File sourceDir (projectFile.getSiblingFile ("Source"));
    
    logOutput (newLine);
    recurseGroup (mainGroupTree, sourceDir);
    
    const Array<File> cppFiles (buildUnityCpp (sourceDir));
    
    // add unity files
    ValueTree unityGroup (Ids::group);
    unityGroup.setProperty (Ids::idProp, createAlphaNumericUID(), nullptr);
    unityGroup.setProperty (Ids::nameProp, "Unity", nullptr);
    mainGroupTree.addChild (unityGroup, -1, nullptr);
    
    for (int i = 0; i < cppFiles.size(); ++i)
    {
        File cppFile (cppFiles[i]);
        
        if (cppFile.exists())
        {
            ValueTree cppTree (Ids::file);
            cppTree.setProperty (Ids::idProp, createAlphaNumericUID(), nullptr);
            cppTree.setProperty (Ids::nameProp, cppFile.getFileName(), nullptr);
            cppTree.setProperty (Ids::compileProp, true, nullptr);
            cppTree.setProperty (Ids::resourceProp, false, nullptr);
            cppTree.setProperty (Ids::fileProp, cppFile.getRelativePathFrom (projectFile), nullptr);
            unityGroup.addChild (cppTree, -1, nullptr);
        }
    }
    
    // write unity Introjucer project
    unityProjectFile = projectFile.getSiblingFile (projectFile.getFileNameWithoutExtension() + unityName + projectFile.getFileExtension());
    
    if (unityProjectFile.existsAsFile())
        unityProjectFile.deleteFile();
    
    writeValueTreeToFile (project, unityProjectFile);
    
    logOutput ("Completed successfully!");
    
    return true;
}

void UnityProjectBuilder::setNumFilesToSplitBetween (int numFiles_)
{
    numFiles = numFiles_;
}

void UnityProjectBuilder::setLogOutput (bool shouldOutput)
{
    shouldLog = shouldOutput;
}

//==============================================================================
void UnityProjectBuilder::recurseGroup (ValueTree group, const File& sourceDir)
{
    logOutput ("Recursing group \"" + group.getProperty (Ids::nameProp).toString() + "\"...");

    const int numChildren = group.getNumChildren();
    
    for (int i = 0; i < numChildren; ++i)
    {
        ValueTree child (group.getChild (i));
        
        if (child.hasType (Ids::group))
            recurseGroup (child, sourceDir);
        else if (child.hasType (Ids::file))
            parseFile (child, sourceDir);
    }
}

void UnityProjectBuilder::parseFile (ValueTree file, const File& sourceDir)
{
    const String path (file.getProperty (Ids::fileProp).toString());
    const File sourceFile (projectFile.getSiblingFile (path));
    const bool compile = bool (file.getProperty (Ids::compileProp));
    
    if (compile && isValidSourceFile (sourceFile))
    {
        file.setProperty (Ids::compileProp, false, nullptr);
        filesToAdd.add (sourceFile.getRelativePathFrom (sourceDir));
        logOutput ("Adding file \"" + path + "\"...");
    }
}

Array<File> UnityProjectBuilder::buildUnityCpp (const File& destDir)
{
    Array<File> files;
    
    if (filesToAdd.size() == 0)
        return files;

    const int filesPerUnityFile = (int) std::ceil (filesToAdd.size() / (float) numFiles);
    
    for (int i = 0; i < numFiles; ++i)
    {
        const int start = i * filesPerUnityFile;
        const int end = jmin (filesToAdd.size(), start + filesPerUnityFile);

        files.add (buildUnityCpp (destDir, i, Range<int> (start, end)));
    }
    
    return files;
}

File UnityProjectBuilder::buildUnityCpp (const File& destDir, int unityNum, const Range<int> fileRange)
{
    const File cppFile (destDir.getChildFile (unityName + String (unityNum)).withFileExtension (".cpp"));
    logOutput ("Building Unity cpp file \"" + cppFile.getFullPathName() + "\"...");
    
    if (cppFile.existsAsFile())
        cppFile.deleteFile();
    
    FileOutputStream s (cppFile);
    
    for (int i = fileRange.getStart(); i < fileRange.getEnd(); ++i)
        s << "#include \"" << filesToAdd[i] << "\"" << newLine;
    
    return cppFile;
}

void UnityProjectBuilder::updateBuildDirectories()
{
    if (buildDir.isEmpty())
        return;
    
    ValueTree exportsTree (project.getChildWithName (Ids::exportFormats));
    
    if (! exportsTree.isValid())
        return;
    
    const int numExports = exportsTree.getNumChildren();
    
    for (int i = 0; i < numExports; ++i)
    {
        ValueTree exporter (exportsTree.getChild (i));
        
        if (exporter.hasProperty (Ids::targetFolderProp))
        {
            logOutput ("Updating exporter " + exporter.getType().toString());
            const String oldTarget (exporter.getProperty (Ids::targetFolderProp).toString());
            String newTarget (buildDir);
            
            if (oldTarget.containsChar ('/'))
                 newTarget << oldTarget.fromLastOccurrenceOf ("/", true, false);
                
            exporter.setProperty (Ids::targetFolderProp, newTarget, nullptr);
        }
    }
}

void UnityProjectBuilder::logOutput (const String& output)
{
    if (shouldLog)
        std::cout << output << std::endl;
}

void UnityProjectBuilder::setUnityFileName (const String& fileName)
{
    unityName = fileName;
}

void UnityProjectBuilder::setBuildDirectoryName (const String& buildDirName)
{
    buildDir = buildDirName;
}

void UnityProjectBuilder::saveProject (const File& introjucerAppFile)
{
    if (! (introjucerAppFile.exists() || unityProjectFile.existsAsFile()))
        return;
    
    logOutput("Resaving Introjucer project...");
    StringArray args;
    args.add (getExeFromApp (introjucerAppFile).getFullPathName());
    args.add ("--resave");
    args.add (unityProjectFile.getFullPathName());

    ChildProcess introjucerProcess;
    introjucerProcess.start (args);
    logOutput (introjucerProcess.readAllProcessOutput());
}

//==============================================================================
bool UnityProjectBuilder::isValidHeaderFile (const File& file)
{
    return file.hasFileExtension ("h") || file.hasFileExtension ("hpp");
}

bool UnityProjectBuilder::isValidSourceFile (const File& file)
{
    return file.hasFileExtension ("cpp") || file.hasFileExtension ("c");
}

File UnityProjectBuilder::getExeFromApp (const File& app)
{
   #if JUCE_MAC
    return app.getChildFile ("Contents").getChildFile ("MacOS").getChildFile (app.getFileNameWithoutExtension());
   #else
    return app;
   #endif
}

String UnityProjectBuilder::createAlphaNumericUID()
{
    // copied from Introjucer
    String uid;
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    Random r;
    
    uid << chars [r.nextInt (52)]; // make sure the first character is always a letter
    
    for (int i = 9; --i >= 0;)
    {
        r.setSeedRandomly();
        uid << chars [r.nextInt (62)];
    }
    
    return uid;
}
