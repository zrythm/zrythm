/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_SCRIPTABLEENGINE_HEADER__
#define __JUCETICE_SCRIPTABLEENGINE_HEADER__

#if JUCE_SUPPORT_SCRIPTING

//==============================================================================
/** Manages a script to access functionality of juce

    @see angelscript
*/
class ScriptableEngine
{
public:

    //==============================================================================
    enum EngineStatus
    {
        Invalid   = 0,
        Ready     = 1,
        Compiled  = 2,
        Running   = 3,
        Paused    = 4
    };

    //==============================================================================
    /** Creates a script engine. */
    ScriptableEngine();

    /** Destructor. */
    virtual ~ScriptableEngine();

    //==============================================================================
    /** Executes a current script
        
        
    */
    virtual void executeScript (const String& script);

    /**
    */
    virtual bool compileScript (const String& scriptCode);

    /**
    */
    virtual void executeFunction (const String& functionName);

    /** Prints an error reported by the engine
    
        When the engine encounter an error compiling or at runtime, it will trigger
        this function. Obviously you can override this in order to allow your
        specific engine to explicitly ignore errors, or to redirect error reporting
        to a GUI component, a message box or whatever your application needs.
     */
    virtual void printErrorMessage (const String& type,
                                    const String& message,
                                    const String& section,
                                    const int lineNumber);

    //==============================================================================
    /** Return the current engine */
    void* getCurrentEngine () const                            { return engine; }    

protected:

    //==============================================================================
    /** Initialise the engine
    
        Must be called before trying to do anything to the engine itself, and if
        this returns false, then it will be impossible to use the engine itself.
     */
    virtual void initialise ();

    /** Shutdown the engine
    
        Should be called automatically when the object goes out of scope, but can be
        called if you need to manually free any engine and scripts allocated.
     */
    virtual void shutdown ();

    //==============================================================================
    void* engine;
    EngineStatus status;
};

#if 0

#include "../juce_core/text/juce_XmlDocument.h"
#include "../juce_core/threads/juce_Thread.h"
#include "../juce_core/io/files/juce_DirectoryIterator.h"
#include "../juce_appframework/events/juce_ChangeBroadcaster.h"
#include "../juce_appframework/audio/midi/juce_MidiKeyboardState.h"
#include "../juce_appframework/audio/processors/juce_AudioProcessor.h"
#include "containers/jucetice_LockFreeFifo.h"


class ScriptableEngine;
class asIScriptEngine;
class asIScriptContext;


//==============================================================================
/** Manages a script to access functionality of juce

    @see angelscript
*/
class ScriptableEngine
{
public:

    //==============================================================================
    /** Creates a script engine. */
    ScriptableEngine();

    /** Destructor. */
    virtual ~ScriptableEngine();

    //==============================================================================
	void setDefaultContextStackSize (const int initialSize, const int maxSize);

    //==============================================================================
	/** Registers a global property to be used in the script

		This function is actually used in the main application to publish app managed
		objects and variables to the script sections.

		@param declaration				this is the declaration of the variable,
										usually something like "Object obj"
		@param object					this is a void pointer to the object the
										application is publishing
	*/
	void registerGlobalProperty (const String& declaration, void* object);

    //==============================================================================
	/** Add a script section to the engine

        You can add any number of script sections you need, but u have to
		name them with different identifier. This also compile
		the script that u added

		@param strScript				this is the script code as string
		@param strSection				this is the name of the section you are compiling
										since more session are allowed, you have to make different
										names for different sections
    */
	bool compileScript (const String& strScript,
                        const String& strSection = "main"
//						,const String& strModule = 0,
						);

    //==============================================================================
    /** This check for a function defined in the script
		and execute it in the context we have created

		Actually this support only one context at time

		@param strFunction				this is the declaration of the function you
										would like to execute
    */
	bool executeFunction (const String& strFunction
//						  ,const String& strModule = 0,
                         );

    /** This execute a string of code

		@param strScript				this is the script you want to execute
    */
	bool executeString (const String& strScript);

    //==============================================================================
    /** This abort the current running context

		Actually this is only useful in debug mode
    */
	void abortExecution ();

    /** This let continue execution if in debug mode

		Actually this is only useful in debug mode
    */
	void continueExecution ();

	//==============================================================================
	/** This lets you add directory to the resolve path when looking for
		files to include or a plugin library to load
	*/
	void addResolveFilePath (const File& directoryToLookInto);

	/** This lets you remove a previously added directory to the resolve path
	*/
	void removeResolveFilePath (const File& directoryToLookInto);

	/** This lets you try to locate a file in the resolve paths, useful for
		trying to locate add directory to the resolve path when looking for
		files to include or a plugin library to load
	*/
	File findResolveFilePaths (const String& fileName);


	//==============================================================================
	/** Passing a function identifier it returns the declaration
	*/
	String resolveFunctionDeclaration (const int functionID);

    //==============================================================================
	void setEngineModel (ScriptableEngineModel* modelToNotify)	{ engineModel = modelToNotify; }

    //==============================================================================
	/**@internal*/
	asIScriptEngine* getEngine() const		{ return engine; }
	/**@internal*/
	void resetEngine ();
	/**@internal*/
	asIScriptContext* createContext();
	/**@internal*/
	bool executeContext (asIScriptContext* context);
	/**@internal*/
	String serializeVariable (asIScriptContext* ctx, const int i);


protected:

	//==============================================================================
	/** Use this to do some jobs before script sections compilation */
	virtual void preCompileCallback () {}

	/** Used in debugging mode to do some jobs every line of the script */
	virtual void debugLineCallback (asIScriptContext* context) {}

    //==============================================================================
    /** Use this to set your specific report when the compiler
	    gets messed up. Here you could write to a Log file or
		use the AlertWindow to report errors to user.
    */
	virtual void reportErrors (const String& errorType,
                               const String& errorString,
							   const String& fileName,
							   const int lineNumber) {}

    //==============================================================================
    /** Use this to output lines from plugins in a IDE output section.
		This is useful when u want the user defined plugins to be able to
		interact with the ide directly, outputting strings in the text
		editor debugger.
    */
	virtual void reportOutput (const String& message, const int appendMode) {}


	StringArray resolvePaths;

	int timeOut;

	bool debugMode;
	bool warningsOn;
	bool preprocessFailed;
	bool compileFailed;
	bool runningScript;
	bool abortScript;
	bool continueSilently; // @TODO - change this to step to next breakpoint

	asIScriptEngine *engine;
	void* outStream;
};

#endif

#endif // JUCE_SUPPORT_SCRIPTING

#endif // __JUCETICE_SCRIPTABLEENGINE_HEADER__
