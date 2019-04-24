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

#if JUCE_SUPPORT_SCRIPTING

#if JUCE_WIN32
 #undef T
 #undef GetObject
#endif

namespace angelscriptNamespace
{
#if JUCETICE_INCLUDE_ANGELSCRIPT_CODE
  #include "../dependancies/angelscript/include/angelscript.h"
  #include "../dependancies/angelscript/source/as_arrayobject.cpp"
  #include "../dependancies/angelscript/source/as_atomic.cpp"
  #include "../dependancies/angelscript/source/as_builder.cpp"
  #include "../dependancies/angelscript/source/as_bytecode.cpp"
  #include "../dependancies/angelscript/source/as_callfunc.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_arm.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_mips.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_ppc_64.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_ppc.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_sh4.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_x64_gcc.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_x86.cpp"
  #include "../dependancies/angelscript/source/as_callfunc_xenon.cpp"
  #include "../dependancies/angelscript/source/as_compiler.cpp"
  #include "../dependancies/angelscript/source/as_configgroup.cpp"
  #include "../dependancies/angelscript/source/as_context.cpp"
  #include "../dependancies/angelscript/source/as_datatype.cpp"
  #include "../dependancies/angelscript/source/as_gc.cpp"
  #include "../dependancies/angelscript/source/as_generic.cpp"
  #include "../dependancies/angelscript/source/as_memory.cpp"
  #include "../dependancies/angelscript/source/as_module.cpp"
  #include "../dependancies/angelscript/source/as_objecttype.cpp"
  #include "../dependancies/angelscript/source/as_outputbuffer.cpp"
  #include "../dependancies/angelscript/source/as_parser.cpp"
  #include "../dependancies/angelscript/source/as_restore.cpp"
  #include "../dependancies/angelscript/source/as_scriptcode.cpp"
  #include "../dependancies/angelscript/source/as_scriptengine.cpp"
  #include "../dependancies/angelscript/source/as_scriptfunction.cpp"
  #include "../dependancies/angelscript/source/as_scriptnode.cpp"
  #include "../dependancies/angelscript/source/as_scriptstruct.cpp"
  #include "../dependancies/angelscript/source/as_string.cpp"
  #include "../dependancies/angelscript/source/as_string_util.cpp"
  #include "../dependancies/angelscript/source/as_thread.cpp"
  #include "../dependancies/angelscript/source/as_tokenizer.cpp"
  #include "../dependancies/angelscript/source/as_typeinfo.cpp"
  #include "../dependancies/angelscript/source/as_variablescope.cpp"
#else
  #include <angelscript.h>
#endif
}

#if JUCE_WIN32
 #define T(stringLiteral)            JUCE_T(stringLiteral)
 #ifdef UNICODE
  #define GetObject  GetObjectW
 #else
  #define GetObject  GetObjectA
#endif // !UNICODE
#endif 

BEGIN_JUCE_NAMESPACE

#include "bindings/jucetice_ScriptableEngineString.h"
#include "bindings/jucetice_ScriptableEngineCore.h"

using namespace angelscriptNamespace;

//==============================================================================
void ScriptableEngineMessageCallback (asSMessageInfo* msg, ScriptableEngine* engine)
{
	String msgType = "Unknown";

	switch (msg->type) {
	    case asMSGTYPE_ERROR:       msgType = "Error"; break;
	    case asMSGTYPE_WARNING:     msgType = "Warning"; break;
	    case asMSGTYPE_INFORMATION: msgType = "Info"; break;
	}

    if (engine)
        engine->printErrorMessage (msgType, msg->message, msg->section, msg->row);
}

void ScriptableEngineLineCallback (asIScriptContext* context, ScriptableEngine* engine)
{
#if 0
	// If the time out is reached we suspend the script
	if (script->timeOut < Time::currentTimeMillis())
		context->Suspend();
#endif
}

//==============================================================================
ScriptableEngine::ScriptableEngine()
    : engine (0),
      status (Invalid)
{
    initialise ();
}

ScriptableEngine::~ScriptableEngine()
{
    shutdown ();
}

void ScriptableEngine::initialise ()
{
	// creates the engine
	asIScriptEngine* currentEngine = asCreateScriptEngine (ANGELSCRIPT_VERSION);
    if (currentEngine != 0)
    {
        // now dispose the current engine  
        shutdown ();
        engine = currentEngine;

        // set the common compiler message handler
        currentEngine->SetMessageCallback (asFUNCTION (ScriptableEngineMessageCallback), currentEngine, asCALL_THISCALL);

        // set engine properties
        currentEngine->SetEngineProperty (asEP_ALLOW_UNSAFE_REFERENCES, 1);

        // register extensions
        Bindings::RegisterObjectTypeString (this);
        Bindings::RegisterObjectTypeCore (this);

        // engine status created
        status = Ready;
    }
}

void ScriptableEngine::shutdown ()
{
	if (engine)
	{
		((asIScriptEngine*) engine)->Release();
		engine = 0;
	}

	status = Invalid;
}

void ScriptableEngine::printErrorMessage (const String& type,
                                          const String& message,
                                          const String& section,
                                          const int lineNumber)
{
    printf ("%s: section %s %d - %s",
            (const char*) type,
            (const char*) section,
            lineNumber,
            (const char*) message);
}

void ScriptableEngine::executeScript (const String& script)
{
    asIScriptEngine* currentEngine = (asIScriptEngine*) engine;
    
    if (currentEngine != 0 && status != Invalid)
    {
        int r, func_id;

        // create the module
        asIScriptModule* module = currentEngine->GetModule (0, asGM_CREATE_IF_NOT_EXISTS);

        r = module->AddScriptSection ("script", (const char*) script, script.length()); jassert (r >= 0);
        r = module->Build(); jassert (r >= 0);
        func_id = module->GetFunctionIdByName ("main"); jassert (func_id >= 0);

        asIScriptContext* context = currentEngine->CreateContext();
        if (context != 0)
        {
	        if (r >= 0)
	        {
                r = context->Prepare (func_id); jassert (r >= 0);
                r = context->Execute (); jassert (r >= 0);
	        }

            context->Release();
            context = 0;

            currentEngine->GarbageCollect (asGC_FULL_CYCLE);
        }
    }
}

bool ScriptableEngine::compileScript (const String& scriptCode)
{
    asIScriptEngine* currentEngine = (asIScriptEngine*) engine;

    if (currentEngine != 0 && status != Invalid)
    {
        int r;

	    // remove carriage returns if any...
	    String scriptCodeProcessed = scriptCode.replaceCharacter('\r','\n');
	    scriptCodeProcessed += T(' '); // hack if the script is empty

        // create the module
        asIScriptModule* module = currentEngine->GetModule (0, asGM_CREATE_IF_NOT_EXISTS);

	    // add a script section
        r = module->AddScriptSection ("main", (const char*) scriptCodeProcessed, scriptCodeProcessed.length());
	    if (r < 0)
		    return false;

	    // build the script
        r = module->Build();
	    if (r < 0)
		    return false;

        status = Compiled;

    	return true;
    }

    return false;
}

void ScriptableEngine::executeFunction (const String& functionName)
{
    asIScriptEngine* currentEngine = (asIScriptEngine*) engine;

    if (currentEngine != 0 && status == Compiled)
    {
        int r, func_id;

        // get the module    
        asIScriptModule* module = currentEngine->GetModule (0, asGM_CREATE_IF_NOT_EXISTS);

    	// get the function to execute
    	func_id = module->GetFunctionIdByName ((const char*) functionName); jassert (func_id >= 0);
    	if (func_id < 0)
    	{
    	    printErrorMessage ("Runtime", String("Cannot find function ") + functionName, "", -1);
    		return;
    	}

	    // creates the context
	    asIScriptContext* context = currentEngine->CreateContext();
	    if (! context)
	    {
		    printErrorMessage ("Runtime", "Cannot create context for execution", "", -1);
		    return;
	    }

        r = context->SetLineCallback (asFUNCTION(ScriptableEngineLineCallback), this, asCALL_CDECL);
        r = context->Prepare (func_id);
        if (r < 0)
        {
	        printErrorMessage ("Runtime", "Cannot prepare function ececution", "", -1);
	        return;
        }

        status = Running;

        bool abortScript = false;
        bool exitLoop = false;

    	while (! abortScript)
        {
    	    try
            {
        		int timeOut = Time::getMillisecondCounter() + 100; // 100
        		r = context->Execute();
            }
    	    catch(...)
    	    {
    	        r = asEXECUTION_EXCEPTION;
            }

	        switch (r)
	        {
	        case asEXECUTION_FINISHED:
		        {
		        exitLoop = true;
		        break;
		        }
	        case asEXECUTION_SUSPENDED:
		        {
		        MessageManager::getInstance()->runDispatchLoopUntil (250);
		        break;
		        }
	        case asEXECUTION_ABORTED:
		        {
		        printErrorMessage ("Exception",
                                   "The script has timed out.",
                                   "", // TODO
                                   context->GetExceptionLineNumber());
		        exitLoop = true;
		        break;
		        }
	        case asEXECUTION_EXCEPTION:
		        {
		        printErrorMessage ("Exception",
                                   context->GetExceptionString(),
                                   "", // TODO
                                   context->GetExceptionLineNumber());
		        exitLoop = true;
		        break;
		        }
	        }

	        if (exitLoop)
		        break;
        }

        context->Release();
        context = 0;

        status = Compiled;
    }

/*
//	Retrieve the return value from the context
	float returnValue = context->GetReturnFloat();
	cout << "The script function returned: " << returnValue << endl;
	returnValue = true;
*/
}


#if 0
//==============================================================================
class ScriptableCompilerStream
{
public:

	ScriptableCompilerStream (ScriptableEngine* const engine_) :
	  engine (engine_)
	{}

	void Write (asSMessageInfo* msg)
	{
		String msgType = "Unknown";
		if (msg->type == asEMsgType::asMSGTYPE_ERROR)       msgType = "Error";
		if (msg->type == asEMsgType::asMSGTYPE_WARNING)     msgType = "Warning";
		if (msg->type == asEMsgType::asMSGTYPE_INFORMATION) msgType = "Info";

		String file (msg->section);
		int line = msg->row;

		switch (msg->type)
		{
		case asEMsgType::asMSGTYPE_ERROR:
			engine->engineModel->reportErrors (msgType, msg->message, file, line);
			break;
		case asEMsgType::asMSGTYPE_WARNING:
		case asEMsgType::asMSGTYPE_INFORMATION:
			if (engine->warningsOn)
				engine->engineModel->reportErrors (msgType, msg->message, file, line);
			break;
		}
	}

private:
	ScriptableEngine* engine;
};


//==============================================================================
class ScriptableByteCodeInStream : public asIBinaryStream
{
public:

	ScriptableByteCodeInStream (ScriptableEngine* compiler_,
							    const File& byteCodeFile_) :
	  compiler (compiler_),
      byteCodeFile (byteCodeFile_)
	{
		in = byteCodeFile.createInputStream();
	}

	~ScriptableByteCodeInStream () {
		delete in;
	}

	void Write (const void *ptr, asUINT size) {}
	void Read (void *ptr, asUINT size) {
		in->read (ptr, size);
	}

private:
	ScriptableEngine* compiler;
	FileInputStream* in;
	File byteCodeFile;
};

class ScriptableByteCodeOutStream : public asIBinaryStream
{
public:

	ScriptableByteCodeOutStream (ScriptableEngine* compiler_,
							     const File& byteCodeFile_) :
	  compiler (compiler_),
      byteCodeFile (byteCodeFile_)
	{
		out = byteCodeFile.createOutputStream();
	}

	~ScriptableByteCodeOutStream () {
		delete out;
	}

	void Read (void *ptr, asUINT size) {}
	void Write (const void *ptr, asUINT size) {
		out->write (ptr, size);
	}

private:
	ScriptableEngine* compiler;
	FileOutputStream* out;
	File byteCodeFile;
};


//==============================================================================
/*
String PrintVariables (asIScriptContext *ctx, int stackLevel)
{
	String error;
	int numVars = ctx->GetVarCount(stackLevel);
	asIScriptEngine *engine = ctx->GetEngine();
	for( int n = 0; n < numVars; n++ )
	{
		int typeId = ctx->GetVarTypeId(n, stackLevel);
		if( typeId == engine->GetTypeIdByDecl(0, "int") )
		{
			error << ctx->GetVarDeclaration(n, 0, stackLevel)
					<< " " << *(int*)ctx->GetVarPointer(n, stackLevel) << "\n";
		}
		else if( typeId == engine->GetTypeIdByDecl(0, "String") )
		{
			error << ctx->GetVarDeclaration(n, 0, stackLevel)
					<< " " << (const char*) ((*(asString**)ctx->GetVarPointer(n, stackLevel))->buffer);
		}
	}
	return error;
}

void ScriptableExceptionCallback (asIScriptContext *context, void *param)
{
	ScriptableEngine* script = (ScriptableEngine*) param;
	asIScriptEngine *engine = context->GetEngine();

	int funcID = context->GetExceptionFunction();

	String error = "Exception: ";
	error
		<< context->GetExceptionString() << "\n"
		<< engine->GetFunctionDeclaration(funcID) << "\n";

	error += PrintVariables (context, -1);
	error += "\n";

	for( int n = 0; n < context->GetCallstackSize(); n++ )
	{
		funcID = context->GetCallstackFunction (n);

		error += engine->GetFunctionDeclaration(funcID);
		error += "\n";
		error += PrintVariables (context, n);
		error += "\n";
	}

	if (script->engineModel)
		script->engineModel->reportErrors ("runtime", error);
}
*/

//==============================================================================
void ScriptableDebuggerOutput (const char* message, const int appendMode)
{
	ScriptableEngine* script = ScriptableEngine::getInstance();
	if (script && script->engineModel)
	{
		script->engineModel->reportOutput (message, appendMode);
	}
}

//==============================================================================
void ScriptableLineCallback (asIScriptContext* context, ScriptableEngine* script)
{
	// If the time out is reached we suspend the script
	if( script->timeOut < Time::currentTimeMillis() )
		context->Suspend();
}

void ScriptableLineCallbackDebug (asIScriptContext* context, ScriptableEngine* script)
{
	if (script->engineModel)
		script->engineModel->debugLineCallback (script, context);

	context->Suspend();
}


//==============================================================================
ScriptableEngine::ScriptableEngine() :
		timeOut (0),
		debugMode (false),
		warningsOn (true),
		preprocessFailed (false),
		compileFailed (false),
		runningScript (false),
		abortScript (false),
		continueSilently (false),
		outStreamPreprocessor (0),
        outStream (0),
		engineModel (0),
		engine (0)
{
	// creates the engine
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		return;
	}

	// set the common compiler message handler
	outStream = new ScriptableCompilerStream (this);
	engine->SetMessageCallback (asMETHOD(ScriptableCompilerStream,Write), outStream, asCALL_THISCALL);

    // set engine properties
    engine->SetEngineProperty (asEP_ALLOW_UNSAFE_REFERENCES, 1);

	// add some predefined resolvePaths
//	resolvePaths.add ( File::getCurrentWorkingDirectory().getFullPathName() );
//	resolvePaths.add ( File::getCurrentApplicationFile().getParentDirectory().getFullPathName() );
}

ScriptableEngine::~ScriptableEngine()
{
	if (engine)
		engine->Release();

	if (outStream)
		delete (ScriptableCompilerStream*) outStream;
}

//==============================================================================
void ScriptableEngine::resetEngine ()
{
	// release the engine
	if (engine)
		engine->Release ();

	if (outStream)
		delete (ScriptableCompilerStream*) outStream;

	// recreates the engine
	engine = asCreateScriptEngine (ANGELSCRIPT_VERSION);
	if( engine == 0 )
		return;

	outStream = new ScriptableCompilerStream (this);
	engine->SetMessageCallback (asMETHOD(ScriptableCompilerStream,Write), outStream, asCALL_THISCALL);

	debugMode = false;
	runningScript = false;
	abortScript = false;
	continueSilently = false;
	preprocessFailed = false;
	compileFailed = false;
}


//==============================================================================
void ScriptableEngine::setDefaultContextStackSize (const int initialSize, const int maxSize)
{
	engine->SetDefaultContextStackSize (initialSize, maxSize);
}

//==============================================================================
void ScriptableEngine::registerGlobalProperty(const String& declaration, void* object)
{
	engine->RegisterGlobalProperty ((const char*)declaration,object);
}

//==============================================================================
bool ScriptableEngine::compileScript (const String& strScript, const String& strSection)
{
    jassert (engine != 0);

	compileFailed = false;

	// do garbage collect
	engine->GarbageCollect (true);

	// remove carriage returns if any...
	String preScript = strScript.replaceCharacter('\r','\n');
	preScript += T(' '); // hack if the script is empty

	// call the pre compiler callback
	preCompileCallback ();

	// add a script section
	int r = engine->AddScriptSection (0, (const char*) strSection, (const char*) strScript, outVector.size(), 0);
	if (r < 0)
	{
		compileFailed = true;
		return false;
	}

	// build the script
	r = engine->Build (0);
	if (r < 0)
	{
		compileFailed = true;
		return false;
	}

	return true;
}

//==============================================================================
asIScriptContext* ScriptableEngine::createContext()
{
	asIScriptContext* context = engine->CreateContext();

	if( context != 0 )
	{
		int r;
		if (debugMode)
			r = context->SetLineCallback (asFUNCTION(ScriptableLineCallbackDebug), this, asCALL_CDECL);
		else
			r = context->SetLineCallback (asFUNCTION(ScriptableLineCallback), this, asCALL_CDECL);

		if (r < 0)
		{
			context->Release();
			context = 0;
		}
	}

	return context;
}

//==============================================================================
bool ScriptableEngine::executeFunction (const String& strFunction)
{
    jassert (engine != 0);

	if (compileFailed)
		return false;

	// get the function to execute
	int functionID = engine->GetFunctionIDByName(0, (const char*) strFunction);
	if (functionID < 0)
	{
	    reportErrors ("runtime", String("Cannot find function ") + strFunction, "", -1);
		return false;
	}

	// creates the context
	asIScriptContext* context = createContext();
	if (context == 0)
	{
		reportErrors ("runtime", "Cannot create context for execution", "", -1);
		return false;
	}

	// prepare the function
	int r = context->Prepare (functionID);
	if (r < 0)
	{
		reportErrors ("runtime", "Cannot prepare function ececution", "", -1);
		return false;
	}

	bool returnValue = false;
	bool exitLoop = false;

	runningScript = true;
	abortScript = false;
	continueSilently = true;

	while (!abortScript)
	{
//	    try
	    {
    		timeOut = Time::getMillisecondCounter() + 100; // 100
    		r = context->Execute();
	    }
//	    catch(...)
//	    {
//	        r = asEXECUTION_EXCEPTION;
//      }

		switch(r)
		{
		case asEXECUTION_FINISHED:
			{
			returnValue = true;
			exitLoop = true;
			break;
			}
		case asEXECUTION_SUSPENDED:
			{
			MessageManager::getInstance()->dispatchPendingMessages ();
			break;
			}
		case asEXECUTION_ABORTED:
			{
			reportErrors ("Exception",
                          "The script has timed out.",
                          resolveOriginalFile (context->GetExceptionLineNumber()),
                          context->GetExceptionLineNumber());
			exitLoop = true;
			break;
			}
		case asEXECUTION_EXCEPTION:
			{
			int funcID = context->GetExceptionFunction();

			engineModel->reportErrors ("Exception",
										resolveFunctionDeclaration (funcID) + " : " +context->GetExceptionString(),
										resolveOriginalFile (context->GetExceptionLineNumber()),
										resolveOriginalLine (context->GetExceptionLineNumber()) );

			exitLoop = true;
			break;
			}
		default:
			exitLoop = true;
			break;
		}

		if (exitLoop)
			break;
	}

/*
//	Retrieve the return value from the context
	float returnValue = context->GetReturnFloat();
	cout << "The script function returned: " << returnValue << endl;
	returnValue = true;
*/

	context->Release();

	runningScript = false;

	return returnValue;
}

bool ScriptableEngine::executeString (const String& strScript)
{
	if (!engine)
		return false;

	int r = engine->ExecuteString (0, (const char*)strScript);
	if( r < 0 )
	{
		return false;
	}
	return true;
}

//==============================================================================
void ScriptableEngine::abortExecution ()
{
	// @XXX - hack for the component bindings
	asComponentManager::getInstance()->releaseComponents();

	abortScript = true;
}

void ScriptableEngine::continueExecution ()
{
	continueSilently = true;
}

//==============================================================================
int ScriptableEngine::resolveOriginalLine (const int lineNumber)
{
	return lineTranslator.ResolveOriginalLine (lineNumber);
}

String ScriptableEngine::resolveOriginalFile (const int lineNumber)
{
	return String ((const char*) lineTranslator.ResolveOriginalFile (lineNumber).c_str());
}

String ScriptableEngine::resolveFunctionDeclaration (const int functionID)
{
	return String (engine->GetFunctionDeclaration (functionID));
}

String ScriptableEngine::serializeVariable (asIScriptContext* context, const int variableNumber)
{
	void* varPointer = context->GetVarPointer (variableNumber);
	int typeID = context->GetVarTypeId (variableNumber);
	String typeDecl = engine->GetTypeDeclaration (typeID);

	if (typeDecl == "bool")
	{
		return String ((int)*(bool*)(varPointer));
	}
	else if (typeDecl == "int8" ||
		typeDecl == "int16" ||
		typeDecl == "int32" ||
		typeDecl == "int")
	{
		return String (*(int*)(varPointer));
	}
	else if (typeDecl == "uint8" ||
		typeDecl == "uint16" ||
		typeDecl == "uint32" ||
		typeDecl == "uint")
	{
		return String ((int)*(unsigned int*)(varPointer));
	}
	else if (typeDecl == "float")
	{
		return String (*(float*)(varPointer));
	}
	else if (typeDecl == "double")
	{
		return String (*(double*)(varPointer));
	}
	else
	{
/*
		char* buffer = 0;
		for (int i=0; i<plugins.size(); i++)
		{
			ScriptableExtensionModel* plug =
				(ScriptableExtensionModel*) plugins.getUnchecked (i);

			buffer = plug->variableCallback (engine, typeID, varPointer);

			if (buffer != 0)
			{
				String varValue = buffer;
				free (buffer);
				return varValue;
			}
		}
*/
	}

	return "???";
}

//==============================================================================
void ScriptableEngine::addResolveFilePath (const File& directoryToLookInto)
{
	jassert (!directoryToLookInto.exists());

	if (directoryToLookInto.existsAsFile())
		resolvePaths.add (directoryToLookInto.getParentDirectory().getFullPathName());
	else
		resolvePaths.add (directoryToLookInto.getFullPathName());
}

void ScriptableEngine::removeResolveFilePath (const File& directoryToLookInto)
{
	jassert (! directoryToLookInto.exists());

	if (directoryToLookInto.existsAsFile())
		resolvePaths.removeString (directoryToLookInto.getParentDirectory().getFullPathName());
	else
		resolvePaths.removeString (directoryToLookInto.getFullPathName());
}

File ScriptableEngine::findResolveFilePaths (const String& fileName)
{
	File fileFound;

	int pathToResolve = resolvePaths.size();
	for (int i=0; i<pathToResolve; i++)
	{
		String filePath = resolvePaths [i];
		filePath << "\\" << fileName;

		File fileToLoad (filePath);
		if (fileToLoad.existsAsFile())
		{
			fileFound = fileToLoad;
			break;
		}
	}

	return fileFound;
}


#endif

END_JUCE_NAMESPACE

#endif // JUCE_SUPPORT_SCRIPTING

