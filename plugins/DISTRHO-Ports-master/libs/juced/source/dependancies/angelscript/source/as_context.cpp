/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_context.cpp
//
// This class handles the execution of the byte code
//

#include <math.h> // fmodf()

#include "as_config.h"
#include "as_context.h"
#include "as_scriptengine.h"
#include "as_tokendef.h"
#include "as_bytecodedef.h"
#include "as_texts.h"
#include "as_callfunc.h"
#include "as_module.h"
#include "as_generic.h"
#include "as_debug.h" // mkdir()
#include "as_bytecode.h"
#include "as_scriptstruct.h"

#ifdef _MSC_VER
#pragma warning(disable:4702) // unreachable code
#endif

BEGIN_AS_NAMESPACE

// We need at least 2 DWORDs reserved for exception handling
// We need at least 1 DWORD reserved for calling system functions
const int RESERVE_STACK = 2*PTR_SIZE;

// For each script function call we push 6 DWORDs on the call stack
const int CALLSTACK_FRAME_SIZE = 6;


#ifdef AS_DEBUG
// Instruction statistics
int instrCount[256];

int instrCount2[256][256];
int lastBC;

class asCDebugStats
{
public:
	asCDebugStats()
	{
		memset(instrCount, 0, sizeof(instrCount));
	}

	~asCDebugStats()
	{
/*
		// This code writes out some statistics for the VM. 
		// It's useful for determining what needs to be optimized.

		_mkdir("AS_DEBUG");
		FILE *f = fopen("AS_DEBUG/total.txt", "at");
		if( f )
		{
			// Output instruction statistics
			fprintf(f, "\nTotal count\n");
			int n;
			for( n = 0; n < BC_MAXBYTECODE; n++ )
			{
				if( bcName[n].name && instrCount[n] > 0 )
					fprintf(f, "%-10.10s : %.0f\n", bcName[n].name, instrCount[n]);
			}

			fprintf(f, "\nNever executed\n");
			for( n = 0; n < BC_MAXBYTECODE; n++ )
			{
				if( bcName[n].name && instrCount[n] == 0 )
					fprintf(f, "%-10.10s\n", bcName[n].name);
			}

			fclose(f);
		}
*/
	}

	double instrCount[256];
} stats;
#endif

AS_API asIScriptContext *asGetActiveContext()
{
	asASSERT(threadManager);
	asCThreadLocalData *tld = threadManager->GetLocalData();
	if( tld->activeContexts.GetLength() == 0 )
		return 0;
	return tld->activeContexts[tld->activeContexts.GetLength()-1];
}

void asPushActiveContext(asIScriptContext *ctx)
{
	asASSERT(threadManager);
	asCThreadLocalData *tld = threadManager->GetLocalData();
	tld->activeContexts.PushLast(ctx);
}

void asPopActiveContext(asIScriptContext *ctx)
{
	asASSERT(threadManager);
	asCThreadLocalData *tld = threadManager->GetLocalData();

	asASSERT(tld->activeContexts.GetLength() > 0);
	asASSERT(tld->activeContexts[tld->activeContexts.GetLength()-1] == ctx);
	UNUSED_VAR(ctx);

	tld->activeContexts.PopLast();
}

asCContext::asCContext(asCScriptEngine *engine, bool holdRef)
{
#ifdef AS_DEBUG
	memset(instrCount, 0, sizeof(instrCount));

	memset(instrCount2, 0, sizeof(instrCount2));

	lastBC = 255;
#endif

	holdEngineRef = holdRef;
	if( holdRef )
		engine->AddRef();
	this->engine = engine;

	status = asEXECUTION_UNINITIALIZED;
	stackBlockSize = 0;
	refCount.set(1);
	module = 0;
	inExceptionHandler = false;
	isStackMemoryNotAllocated = false;

	stringFunction = 0;
	currentFunction = 0;
	objectRegister = 0;
	initialFunction = 0;

	lineCallback = false;
	exceptionCallback = false;

	doProcessSuspend = false;
	doSuspend = false;

	userData = 0;
}

asCContext::~asCContext()
{
	DetachEngine();
}

int asCContext::AddRef()
{
	return refCount.atomicInc();
}

int asCContext::Release()
{
	int r = refCount.atomicDec();

	if( r == 0 )
	{
		asDELETE(this,asCContext);
		return 0;
	}

	return r;
}

void asCContext::DetachEngine()
{
	if( engine == 0 ) return;

	// Abort any execution
	Abort();

	// Free all resources
	Unprepare();

	// Clear engine pointer
	if( holdEngineRef )
		engine->Release();
	engine = 0;
}

asIScriptEngine *asCContext::GetEngine()
{
	return engine;
}

void *asCContext::SetUserData(void *data)
{
	void *oldData = userData;
	userData = data;
	return oldData;
}

void *asCContext::GetUserData()
{
	return userData;
}

int asCContext::Prepare(int funcID)
{
	if( status == asEXECUTION_ACTIVE || status == asEXECUTION_SUSPENDED )
		return asCONTEXT_ACTIVE;

	// Clean the stack if not done before
	if( status != asEXECUTION_UNINITIALIZED )
		CleanStack();

	// Release the returned object (if any)
	CleanReturnObject();

	if( funcID == -1 )
	{
		// Use the previously prepared function
		if( initialFunction == 0 )
			return asNO_FUNCTION;

		currentFunction = initialFunction;
	}
	else if( initialFunction && initialFunction->id == funcID )
	{
		currentFunction = initialFunction;
	}
	else
	{
		// Check engine pointer
		asASSERT( engine );

		initialFunction = engine->GetScriptFunction(funcID);
		currentFunction = initialFunction;
		if( currentFunction == 0 )
			return asNO_FUNCTION;

		// Remove reference to previous module. Add reference to new module
		if( module ) module->ReleaseContextRef();
		module = initialFunction->module;
		if( module )
			module->AddContextRef();

		// Determine the minimum stack size needed
		// TODO: optimize: GetSpaceNeededForArguments() should be precomputed
		int stackSize = currentFunction->GetSpaceNeededForArguments() + currentFunction->stackNeeded + RESERVE_STACK;

		stackSize = stackSize > engine->initialContextStackSize ? stackSize : engine->initialContextStackSize;

		if( stackSize > stackBlockSize )
		{
			for( asUINT n = 0; n < stackBlocks.GetLength(); n++ )
				if( stackBlocks[n] )
				{
					asDELETEARRAY(stackBlocks[n]);
				}
			stackBlocks.SetLength(0);

			stackBlockSize = stackSize;

			asDWORD *stack = asNEWARRAY(asDWORD,stackBlockSize);
			stackBlocks.PushLast(stack);
		}

		// Reserve space for the arguments and return value
		returnValueSize = currentFunction->GetSpaceNeededForReturnValue();

		// TODO: optimize: GetSpaceNeededForArguments() should be precomputed
		argumentsSize = currentFunction->GetSpaceNeededForArguments() + (currentFunction->objectType ? PTR_SIZE : 0);
	}

	if( currentFunction->funcType == asFUNC_SCRIPT )
		byteCode = currentFunction->byteCode.AddressOf();
	else
		byteCode = 0;

	// Reset state
	exceptionLine = -1;
	exceptionFunction = 0;
	isCallingSystemFunction = false;
	doAbort = false;
	doSuspend = false;
	doProcessSuspend = lineCallback;
	externalSuspendRequest = false;
	status = asEXECUTION_PREPARED;

	asASSERT(objectRegister == 0);
	objectRegister = 0;

	// Reserve space for the arguments and return value
	stackFramePointer = stackBlocks[0] + stackBlockSize - argumentsSize;
	stackPointer = stackFramePointer;
	stackIndex = 0;

	// Set arguments to 0
	memset(stackPointer, 0, 4*argumentsSize);

	// Set all object variables to 0
	if( currentFunction->funcType == asFUNC_SCRIPT )
	{
		for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
		{
			int pos = currentFunction->objVariablePos[n];
			*(size_t*)&stackFramePointer[-pos] = 0;
		}
	}

	return asSUCCESS;
}

// Free all resources
int asCContext::Unprepare()
{
	if( status == asEXECUTION_ACTIVE || status == asEXECUTION_SUSPENDED )
		return asCONTEXT_ACTIVE;

	// Only clean the stack if the context was prepared but not executed
	if( status != asEXECUTION_UNINITIALIZED )
		CleanStack();

	// Release the returned object (if any)
	CleanReturnObject();

	// Clear function pointers
	initialFunction = 0;
	currentFunction = 0;
	exceptionFunction = 0;
	byteCode = 0;

	// Release the module
	if( module ) module->ReleaseContextRef();
	module = 0;

	// Reset status
	status = asEXECUTION_UNINITIALIZED;

	// Deallocate the stack blocks
	for( asUINT n = 0; n < stackBlocks.GetLength(); n++ )
	{
		if( stackBlocks[n] )
		{
			asDELETEARRAY(stackBlocks[n]);
		}
	}
	stackBlocks.SetLength(0);
	stackFramePointer = 0;
	stackPointer = 0;
	stackIndex = 0;

	// Deallocate string function
	if( stringFunction )
	{
		asDELETE(stringFunction, asCScriptFunction);
		stringFunction = 0;
	}
	
	return 0;
}

int asCContext::SetExecuteStringFunction(asCScriptFunction *func)
{
	// TODO: multithread: Make thread safe

	// TODO: Verify that the context isn't running

	if( stringFunction )
	{
		asDELETE(stringFunction,asCScriptFunction);
	}

	stringFunction = func;

	return 0;
}

int asCContext::PrepareSpecial(int funcID, asCModule *mod)
{
	// Check engine pointer
	if( engine == 0 ) return asERROR;

	if( status == asEXECUTION_ACTIVE || status == asEXECUTION_SUSPENDED )
		return asCONTEXT_ACTIVE;

	exceptionLine = -1;
	exceptionFunction = 0;

	isCallingSystemFunction = false;

	if( module ) module->ReleaseContextRef();

	module = mod;
	module->AddContextRef();

	if( (funcID & 0xFFFF) == asFUNC_STRING )
		initialFunction = stringFunction;
	else
		initialFunction = module->GetSpecialFunction(funcID & 0xFFFF);

	currentFunction = initialFunction;
	if( currentFunction == 0 )
		return asERROR;

	byteCode = currentFunction->byteCode.AddressOf();

	doAbort = false;
	doSuspend = false;
	doProcessSuspend = lineCallback;
	externalSuspendRequest = false;
	status = asEXECUTION_PREPARED;

	// Determine the minimum stack size needed
	int stackSize = currentFunction->stackNeeded + RESERVE_STACK;

	stackSize = stackSize > engine->initialContextStackSize ? stackSize : engine->initialContextStackSize;

	if( stackSize != stackBlockSize )
	{
		for( asUINT n = 0; n < stackBlocks.GetLength(); n++ )
			if( stackBlocks[n] )
			{
				asDELETEARRAY(stackBlocks[n]);
			}
		stackBlocks.SetLength(0);

		stackBlockSize = stackSize;

		asDWORD *stack = asNEWARRAY(asDWORD,stackBlockSize);
		stackBlocks.PushLast(stack);
	}

	// Reserve space for the arguments and return value
	returnValueSize = currentFunction->GetSpaceNeededForReturnValue();
	argumentsSize = currentFunction->GetSpaceNeededForArguments();

	stackFramePointer = stackBlocks[0] + stackBlockSize - argumentsSize;
	stackPointer = stackFramePointer;
	stackIndex = 0;

	// Set arguments to 0
	memset(stackPointer, 0, 4*argumentsSize);

	// Set all object variables to 0
	for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
	{
		int pos = currentFunction->objVariablePos[n];
		*(size_t*)&stackFramePointer[-pos] = 0;
	}

	return asSUCCESS;
}

asBYTE asCContext::GetReturnByte()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() || dt->IsReference() ) return 0;

	return *(asBYTE*)&register1;
}

asWORD asCContext::GetReturnWord()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() || dt->IsReference() ) return 0;

	return *(asWORD*)&register1;
}

asDWORD asCContext::GetReturnDWord()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() || dt->IsReference() ) return 0;

	return *(asDWORD*)&register1;
}

asQWORD asCContext::GetReturnQWord()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() || dt->IsReference() ) return 0;

	return register1;
}

float asCContext::GetReturnFloat()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() || dt->IsReference() ) return 0;

	return *(float*)&register1;
}

double asCContext::GetReturnDouble()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsObject() || dt->IsReference() ) return 0;

	return *(double*)&register1;
}

void *asCContext::GetReturnAddress()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( dt->IsReference() )
		return *(void**)&register1;
	else if( dt->IsObject() )
		return objectRegister;

	return 0;
}

void *asCContext::GetReturnObject()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	if( !dt->IsObject() ) return 0;

	if( dt->IsReference() )
		return *(void**)(size_t)register1;
	else
		return objectRegister;
}

#ifdef AS_DEPRECATED
// deprecated since 2008-11-11, 2.15.0
void *asCContext::GetReturnPointer()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	// An object is stored in the objectRegister
	if( !dt->IsReference() && dt->IsObject() )
		return &objectRegister;

	// Primitives and references are stored in register1
	return &register1;
}
#endif

void *asCContext::GetAddressOfReturnValue()
{
	if( status != asEXECUTION_FINISHED ) return 0;

	asCDataType *dt = &initialFunction->returnType;

	// An object is stored in the objectRegister
	if( !dt->IsReference() && dt->IsObject() )
	{
		// Need to dereference objects 
		if( !dt->IsObjectHandle() )
			return *(void**)&objectRegister;
		return &objectRegister;
	}

	// Primitives and references are stored in register1
	return &register1;
}

int asCContext::SetObject(void *obj)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( !initialFunction->objectType )
	{
		status = asEXECUTION_ERROR;
		return asERROR;
	}

	*(size_t*)&stackFramePointer[0] = (size_t)obj;

	return 0;
}

int asCContext::SetArgByte(asUINT arg, asBYTE value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeInMemoryBytes() != 1 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(asBYTE*)&stackFramePointer[offset] = value;

	return 0;
}

int asCContext::SetArgWord(asUINT arg, asWORD value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeInMemoryBytes() != 2 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(asWORD*)&stackFramePointer[offset] = value;

	return 0;
}

int asCContext::SetArgDWord(asUINT arg, asDWORD value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeInMemoryBytes() != 4 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(asDWORD*)&stackFramePointer[offset] = value;

	return 0;
}

int asCContext::SetArgQWord(asUINT arg, asQWORD value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 2 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(asQWORD*)(&stackFramePointer[offset]) = value;

	return 0;
}

int asCContext::SetArgFloat(asUINT arg, float value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 1 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(float*)(&stackFramePointer[offset]) = value;

	return 0;
}

int asCContext::SetArgDouble(asUINT arg, double value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( dt->IsObject() || dt->IsReference() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	if( dt->GetSizeOnStackDWords() != 2 )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(double*)(&stackFramePointer[offset]) = value;

	return 0;
}

int asCContext::SetArgAddress(asUINT arg, void *value)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( !dt->IsReference() && !dt->IsObjectHandle() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;

	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(size_t*)(&stackFramePointer[offset]) = (size_t)value;

	return 0;
}

int asCContext::SetArgObject(asUINT arg, void *obj)
{
	if( status != asEXECUTION_PREPARED )
		return asCONTEXT_NOT_PREPARED;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_ARG;
	}

	// Verify the type of the argument
	asCDataType *dt = &initialFunction->parameterTypes[arg];
	if( !dt->IsObject() )
	{
		status = asEXECUTION_ERROR;
		return asINVALID_TYPE;
	}

	// If the object should be sent by value we must make a copy of it
	if( !dt->IsReference() )
	{
		if( dt->IsObjectHandle() )
		{
			// Increase the reference counter
			asSTypeBehaviour *beh = &dt->GetObjectType()->beh;
			if( obj && beh->addref )
				engine->CallObjectMethod(obj, beh->addref);
		}
		else 
		{
			obj = engine->CreateScriptObjectCopy(obj, engine->GetTypeIdFromDataType(*dt));
		}
	}

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	// Set the value
	*(size_t*)(&stackFramePointer[offset]) = (size_t)obj;

	return 0;
}


// TODO: We should deprecate this, and implement the SetArgValue(int arg, void *value, bool takeOwnership) instead.
void *asCContext::GetArgPointer(asUINT arg)
{
	if( status != asEXECUTION_PREPARED )
		return 0;

	if( arg >= (unsigned)initialFunction->parameterTypes.GetLength() )
		return 0;

	// Determine the position of the argument
	int offset = 0;
	if( initialFunction->objectType )
		offset += PTR_SIZE;
	for( asUINT n = 0; n < arg; n++ )
		offset += initialFunction->parameterTypes[n].GetSizeOnStackDWords();

	return &stackFramePointer[offset];
}


int asCContext::Abort()
{
	// TODO: multithread: Make thread safe

	if( engine == 0 ) return asERROR;

	if( status == asEXECUTION_SUSPENDED )
		status = asEXECUTION_ABORTED;

	doSuspend = true;
	doProcessSuspend = true;
	externalSuspendRequest = true;
	doAbort = true;

	return 0;
}

int asCContext::Suspend()
{
	// TODO: multithread: Make thread safe

	if( engine == 0 ) return asERROR;

	doSuspend = true;
	doProcessSuspend = true;
	externalSuspendRequest = true;

	return 0;
}

int asCContext::Execute()
{
	// Check engine pointer
	if( engine == 0 ) return asERROR;

	if( status != asEXECUTION_SUSPENDED && status != asEXECUTION_PREPARED )
		return asERROR;

	status = asEXECUTION_SUSPENDED;

	asPushActiveContext((asIScriptContext *)this);

	if( byteCode == 0 )
	{
		if( currentFunction->funcType == asFUNC_VIRTUAL ||
			currentFunction->funcType == asFUNC_INTERFACE )
		{
			// The currentFunction is a virtual method

			// Determine the true function from the object
			asCScriptObject *obj = *(asCScriptObject**)(size_t*)stackFramePointer;
			if( obj == 0 )
			{
				SetInternalException(TXT_NULL_POINTER_ACCESS);
			}
			else
			{
				asCObjectType *objType = obj->objType;
				asCScriptFunction *realFunc = 0;

				if( currentFunction->funcType == asFUNC_VIRTUAL )
				{
					if( objType->virtualFunctionTable.GetLength() > (asUINT)currentFunction->vfTableIdx )
					{
						realFunc = objType->virtualFunctionTable[currentFunction->vfTableIdx];
					}
				}
				else
				{
					// Search the object type for a function that matches the interface function
					for( asUINT n = 0; n < objType->methods.GetLength(); n++ )
					{
						asCScriptFunction *f2 = engine->scriptFunctions[objType->methods[n]];
						if( f2->signatureId == currentFunction->signatureId )
						{
							if( f2->funcType == asFUNC_VIRTUAL )
								realFunc = objType->virtualFunctionTable[f2->vfTableIdx];
							else
								realFunc = f2;
							break;
						}
					}
				}

				if( realFunc )
				{
					if( realFunc->signatureId != currentFunction->signatureId )
					{
						SetInternalException(TXT_NULL_POINTER_ACCESS);
					}
					else
					{
						currentFunction = realFunc;
						byteCode = currentFunction->byteCode.AddressOf();

						if( module ) module->ReleaseContextRef();
						module = currentFunction->module;
						if( module )
							module->AddContextRef();

						// Set the local objects to 0
						for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
						{
							int pos = currentFunction->objVariablePos[n];
							*(size_t*)&stackFramePointer[-pos] = 0;
						}
					}
				}
			}
		}
		else if( currentFunction->funcType == asFUNC_SYSTEM )
		{
			// The current function is an application registered function

			// Call the function directly
			CallSystemFunction(currentFunction->id, this, 0);
			
			// Was the call successful?
			if( status == asEXECUTION_SUSPENDED )
			{
				status = asEXECUTION_FINISHED;
			}
		}
		else
		{
			// This shouldn't happen
			asASSERT(false);
		}
	}

	while( !doSuspend && status == asEXECUTION_SUSPENDED )
	{
		status = asEXECUTION_ACTIVE;
		while( status == asEXECUTION_ACTIVE )
			ExecuteNext();
	}

	doSuspend = false;
	doProcessSuspend = lineCallback;

	asPopActiveContext((asIScriptContext *)this);


#ifdef AS_DEBUG
/*
	// Output instruction statistics
	// This is useful for determining what needs to be optimized.

	_mkdir("AS_DEBUG");
	FILE *f = fopen("AS_DEBUG/stats.txt", "at");
	fprintf(f, "\n");
	asQWORD total = 0;
	int n;
	for( n = 0; n < 256; n++ )
	{
		if( bcName[n].name && instrCount[n] )
			fprintf(f, "%-10.10s : %d\n", bcName[n].name, instrCount[n]);

		total += instrCount[n];
	}

	fprintf(f, "\ntotal      : %I64d\n", total);

	fprintf(f, "\n");
	for( n = 0; n < 256; n++ )
	{
		if( bcName[n].name )
		{
			for( int m = 0; m < 256; m++ )
			{
				if( instrCount2[n][m] )
					fprintf(f, "%-10.10s, %-10.10s : %d\n", bcName[n].name, bcName[m].name, instrCount2[n][m]);
			}
		}
	}
	fclose(f);
*/
#endif

	if( doAbort )
	{
		doAbort = false;

		status = asEXECUTION_ABORTED;
		return asEXECUTION_ABORTED;
	}

	if( status == asEXECUTION_SUSPENDED )
		return asEXECUTION_SUSPENDED;

	if( status == asEXECUTION_FINISHED )
	{
		objectType = initialFunction->returnType.GetObjectType();
		return asEXECUTION_FINISHED;
	}

	if( status == asEXECUTION_EXCEPTION )
		return asEXECUTION_EXCEPTION;

	return asERROR;
}

void asCContext::PushCallState()
{
	callStack.SetLength(callStack.GetLength() + CALLSTACK_FRAME_SIZE);

	size_t *s = callStack.AddressOf() + callStack.GetLength() - CALLSTACK_FRAME_SIZE;

	s[0] = (size_t)stackFramePointer;
	s[1] = (size_t)currentFunction;
	s[2] = (size_t)byteCode;
	s[3] = (size_t)stackPointer;
	s[4] = stackIndex;
	s[5] = (size_t)module;
}

void asCContext::PopCallState()
{
	size_t *s = callStack.AddressOf() + callStack.GetLength() - CALLSTACK_FRAME_SIZE;

	stackFramePointer = (asDWORD*)s[0];
	currentFunction   = (asCScriptFunction*)s[1];
	byteCode          = (asDWORD*)s[2];
	stackPointer      = (asDWORD*)s[3];
	stackIndex        = (int)s[4];
	module            = (asCModule*)s[5];

	callStack.SetLength(callStack.GetLength() - CALLSTACK_FRAME_SIZE);
}

int asCContext::GetCallstackSize()
{
	return (int)callStack.GetLength() / CALLSTACK_FRAME_SIZE;
}

int asCContext::GetCallstackFunction(int index)
{
	if( index < 0 || index >= GetCallstackSize() ) return asINVALID_ARG;

	size_t *s = callStack.AddressOf() + index*CALLSTACK_FRAME_SIZE;
	asCScriptFunction *func = (asCScriptFunction*)s[1];

	return func->id;
}

int asCContext::GetCallstackLineNumber(int index, int *column)
{
	if( index < 0 || index >= GetCallstackSize() ) return asINVALID_ARG;

	size_t *s = callStack.AddressOf() + index*CALLSTACK_FRAME_SIZE;
	asCScriptFunction *func = (asCScriptFunction*)s[1];
	asDWORD *bytePos = (asDWORD*)s[2];

	asDWORD line = func->GetLineNumber(int(bytePos - func->byteCode.AddressOf()));
	if( column ) *column = (line >> 20);

	return (line & 0xFFFFF);
}

void asCContext::CallScriptFunction(asCModule *mod, asCScriptFunction *func)
{
	// Push the framepointer, functionid and programCounter on the stack
	PushCallState();

	currentFunction = func;
	module = func->module ? func->module : mod;
	byteCode = currentFunction->byteCode.AddressOf();

	// Verify if there is enough room in the stack block. Allocate new block if not
	asDWORD *oldStackPointer = stackPointer;
	while( stackPointer - (func->stackNeeded + RESERVE_STACK) < stackBlocks[stackIndex] )
	{
		// The size of each stack block is determined by the following formula:
		// size = stackBlockSize << index

		// Make sure we don't allocate more space than allowed
		if( engine->ep.maximumContextStackSize )
		{
			// This test will only stop growth once it has already crossed the limit
			if( stackBlockSize * ((1 << (stackIndex+1)) - 1) > engine->ep.maximumContextStackSize )
			{
				isStackMemoryNotAllocated = true;

				// Set the stackFramePointer, even though the stackPointer wasn't updated
				stackFramePointer = stackPointer;

				// TODO: Make sure the exception handler doesn't try to free objects that have not been initialized
				SetInternalException(TXT_STACK_OVERFLOW);
				return;
			}
		}

		stackIndex++;
		if( (int)stackBlocks.GetLength() == stackIndex )
		{
			asDWORD *stack = asNEWARRAY(asDWORD,(stackBlockSize << stackIndex));
			stackBlocks.PushLast(stack);
		}

		stackPointer = stackBlocks[stackIndex] + (stackBlockSize<<stackIndex) - func->GetSpaceNeededForArguments();
	}

	if( stackPointer != oldStackPointer )
	{
		// Copy the function arguments to the new stack space
		memcpy(stackPointer, oldStackPointer, sizeof(asDWORD)*func->GetSpaceNeededForArguments());
	}

	// Update framepointer and programCounter
	stackFramePointer = stackPointer;

	// Set all object variables to 0
	for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
	{
		int pos = currentFunction->objVariablePos[n];
		*(size_t*)&stackFramePointer[-pos] = 0;
	}
}

void asCContext::CallInterfaceMethod(asCModule *mod, asCScriptFunction *func)
{
	// Resolve the interface method using the current script type
	asCScriptObject *obj = *(asCScriptObject**)(size_t*)stackPointer;
	if( obj == 0 )
	{
		SetInternalException(TXT_NULL_POINTER_ACCESS);
		return;
	}

	asCObjectType *objType = obj->objType;

	// TODO: optimize: The object type should have a list of only those methods that 
	//                 implement interface methods. This list should be ordered by
	//                 the signatureId so that a binary search can be made, instead
	//                 of a linear search.

	// Search the object type for a function that matches the interface function
	asCScriptFunction *realFunc = 0;
	if( func->funcType == asFUNC_INTERFACE )
	{
		for( asUINT n = 0; n < objType->methods.GetLength(); n++ )
		{
			asCScriptFunction *f2 = engine->scriptFunctions[objType->methods[n]];
			if( f2->signatureId == func->signatureId )
			{
				if( f2->funcType == asFUNC_VIRTUAL )
					realFunc = objType->virtualFunctionTable[f2->vfTableIdx];
				else
					realFunc = f2;
				break;
			}
		}

		if( realFunc == 0 )
		{
			SetInternalException(TXT_NULL_POINTER_ACCESS);
			return;
		}
	}
	else /* if( func->funcType == asFUNC_VIRTUAL ) */
	{
		realFunc = objType->virtualFunctionTable[func->vfTableIdx];
	}

	// Then call the true script function
	CallScriptFunction(mod, realFunc);
}

#define DWORDARG(x)  (asDWORD(*(x+1)))
#define INTARG(x)    (int(*(x+1)))
#define QWORDARG(x)  (*(asQWORD*)(x+1))
#define FLOATARG(x)  (*(float*)(x+1))
#define PTRARG(x)    (asPTRWORD(*(x+1)))

#define WORDARG0(x)   (*(((asWORD*)x)+1))
#define WORDARG1(x)   (*(((asWORD*)x)+2))

#define SWORDARG0(x) (*(((short*)x)+1))
#define SWORDARG1(x) (*(((short*)x)+2))
#define SWORDARG2(x) (*(((short*)x)+3))


void asCContext::ExecuteNext()
{
	asDWORD *l_bc = byteCode;
	asDWORD *l_sp = stackPointer;
	asDWORD *l_fp = stackFramePointer;

	for(;;)
	{

#ifdef AS_DEBUG
	++stats.instrCount[*(asBYTE*)l_bc];

	++instrCount[*(asBYTE*)l_bc];

	++instrCount2[lastBC][*(asBYTE*)l_bc];
	lastBC = *(asBYTE*)l_bc;

	// Used to verify that the size of the instructions are correct
	asDWORD *old = l_bc;
#endif


	// Remember to keep the cases in order and without
	// gaps, because that will make the switch faster.
	// It will be faster since only one lookup will be
	// made to find the correct jump destination. If not
	// in order, the switch will make two lookups.
	switch( *(asBYTE*)l_bc )
	{
//--------------
// memory access functions

	// Decrease the stack pointer with n dwords (stack grows downward)
	case BC_POP:
		l_sp += WORDARG0(l_bc);
		l_bc++;
		break;

	// Increase the stack pointer with n dwords
	case BC_PUSH:
		l_sp -= WORDARG0(l_bc);
		l_bc++;
		break;

	// Push a dword value on the stack
	case BC_PshC4:
		--l_sp;
		*l_sp = DWORDARG(l_bc);
		l_bc += 2;
		break;

	// Push the dword value of a variable on the stack
	case BC_PshV4:
		--l_sp;
		*l_sp = *(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	// Push the address of a variable on the stack
	case BC_PSF:
		l_sp -= PTR_SIZE;
		*(asPTRWORD*)l_sp = (asPTRWORD)size_t(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	// Swap the top 2 dwords on the stack
	case BC_SWAP4:
		{
			asDWORD d = (asDWORD)*l_sp;
			*l_sp = *(l_sp+1);
			*(asDWORD*)(l_sp+1) = d;
			l_bc++;
		}
		break;

	// Do a boolean not operation, modifying the value of the variable
	case BC_NOT:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is equal to 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on the pointer.

			volatile asBYTE *ptr = (asBYTE*)(l_fp - SWORDARG0(l_bc));
			asBYTE val = (ptr[0] == 0) ? VALUE_OF_BOOLEAN_TRUE : 0;
			ptr[0] = val; // The result is stored in the lower byte
			ptr[1] = 0;   // Make sure the rest of the DWORD is 0
			ptr[2] = 0;
			ptr[3] = 0;
		}
#else
		*(l_fp - SWORDARG0(l_bc)) = (*(l_fp - SWORDARG0(l_bc)) == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
#endif
		l_bc++;
		break;

	// Push the dword value of a global variable on the stack
	case BC_PshG4:
		--l_sp;
		*l_sp = *(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)];
		l_bc++;
		break;

	// Load the address of a globar variable in the register, then  
	// copy the value of the global variable into a local variable
	case BC_LdGRdR4:
		*(void**)&register1 = module->globalVarPointers[WORDARG1(l_bc)];
		*(l_fp - SWORDARG0(l_bc)) = **(asDWORD**)&register1;
		l_bc += 2;
		break;

//----------------
// path control instructions

	// Begin execution of a script function
	case BC_CALL:
		{
			int i = INTARG(l_bc);
			l_bc += 2;

			asASSERT( i >= 0 );
			asASSERT( (i & FUNC_IMPORTED) == 0 );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			CallScriptFunction(module, module->GetScriptFunction(i));

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != asEXECUTION_ACTIVE )
				return;
		}
		break;

	// Return to the caller, and remove the arguments from the stack
	case BC_RET:
		{
			if( callStack.GetLength() == 0 )
			{
				status = asEXECUTION_FINISHED;
				return;
			}

			asWORD w = WORDARG0(l_bc);

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Read the old framepointer, functionid, and programCounter from the stack
			PopCallState();

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// Pop arguments from stack
			l_sp += w;
		}
		break;

	// Jump to a relative position
	case BC_JMP:
		l_bc += 2 + INTARG(l_bc);
		break;

//----------------
// Conditional jumps

	// Jump to a relative position if the value in the register is 0
	case BC_JZ:
		if( *(int*)&register1 == 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;

	// Jump to a relative position if the value in the register is not 0
	case BC_JNZ:
		if( *(int*)&register1 != 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;

	// Jump to a relative position if the value in the register is negative
	case BC_JS:
		if( *(int*)&register1 < 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;

	// Jump to a relative position if the value in the register it not negative
	case BC_JNS:
		if( *(int*)&register1 >= 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;

	// Jump to a relative position if the value in the register is greater than 0
	case BC_JP:
		if( *(int*)&register1 > 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;

	// Jump to a relative position if the value in the register is not greater than 0
	case BC_JNP:
		if( *(int*)&register1 <= 0 )
			l_bc += INTARG(l_bc) + 2;
		else
			l_bc += 2;
		break;
//--------------------
// test instructions

	// If the value in the register is 0, then set the register to 1, else to 0
	case BC_TZ:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is equal to 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on register1.

			volatile int    *regPtr  = (int*)&register1;
			volatile asBYTE *regBptr = (asBYTE*)&register1;
			asBYTE val = (regPtr[0] == 0) ? VALUE_OF_BOOLEAN_TRUE : 0;
			regBptr[0] = val; // The result is stored in the lower byte
			regBptr[1] = 0;   // Make sure the rest of the register is 0
			regBptr[2] = 0;
			regBptr[3] = 0;
			regBptr[4] = 0;
			regBptr[5] = 0;
			regBptr[6] = 0;
			regBptr[7] = 0;
		}
#else
		*(int*)&register1 = (*(int*)&register1 == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
#endif
		l_bc++;
		break;

	// If the value in the register is not 0, then set the register to 1, else to 0
	case BC_TNZ:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is not equal to 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on register1.

			volatile int    *regPtr  = (int*)&register1;
			volatile asBYTE *regBptr = (asBYTE*)&register1;
			asBYTE val = (regPtr[0] == 0) ? 0 : VALUE_OF_BOOLEAN_TRUE;
			regBptr[0] = val; // The result is stored in the lower byte
			regBptr[1] = 0;   // Make sure the rest of the register is 0
			regBptr[2] = 0;
			regBptr[3] = 0;
			regBptr[4] = 0;
			regBptr[5] = 0;
			regBptr[6] = 0;
			regBptr[7] = 0;
		}
#else
		*(int*)&register1 = (*(int*)&register1 == 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
#endif
		l_bc++;
		break;

	// If the value in the register is negative, then set the register to 1, else to 0
	case BC_TS:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is less than 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on register1.

			volatile int    *regPtr  = (int*)&register1;
			volatile asBYTE *regBptr = (asBYTE*)&register1;
			asBYTE val = (regPtr[0] < 0) ? VALUE_OF_BOOLEAN_TRUE : 0;
			regBptr[0] = val; // The result is stored in the lower byte
			regBptr[1] = 0;   // Make sure the rest of the register is 0
			regBptr[2] = 0;
			regBptr[3] = 0;
			regBptr[4] = 0;
			regBptr[5] = 0;
			regBptr[6] = 0;
			regBptr[7] = 0;
		}
#else
		*(int*)&register1 = (*(int*)&register1 < 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
#endif
		l_bc++;
		break;

	// If the value in the register is not negative, then set the register to 1, else to 0
	case BC_TNS:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is not less than 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on register1.

			volatile int    *regPtr  = (int*)&register1;
			volatile asBYTE *regBptr = (asBYTE*)&register1;
			asBYTE val = (regPtr[0] >= 0) ? VALUE_OF_BOOLEAN_TRUE : 0;
			regBptr[0] = val; // The result is stored in the lower byte
			regBptr[1] = 0;   // Make sure the rest of the register is 0
			regBptr[2] = 0;
			regBptr[3] = 0;
			regBptr[4] = 0;
			regBptr[5] = 0;
			regBptr[6] = 0;
			regBptr[7] = 0;
		}
#else
		*(int*)&register1 = (*(int*)&register1 < 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
#endif
		l_bc++;
		break;

	// If the value in the register is greater than 0, then set the register to 1, else to 0
	case BC_TP:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is greater than 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on register1.

			volatile int    *regPtr  = (int*)&register1;
			volatile asBYTE *regBptr = (asBYTE*)&register1;
			asBYTE val = (regPtr[0] > 0) ? VALUE_OF_BOOLEAN_TRUE : 0;
			regBptr[0] = val; // The result is stored in the lower byte
			regBptr[1] = 0;   // Make sure the rest of the register is 0
			regBptr[2] = 0;
			regBptr[3] = 0;
			regBptr[4] = 0;
			regBptr[5] = 0;
			regBptr[6] = 0;
			regBptr[7] = 0;
		}
#else
		*(int*)&register1 = (*(int*)&register1 > 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
#endif
		l_bc++;
		break;

	// If the value in the register is not greater than 0, then set the register to 1, else to 0
	case BC_TNP:
#if AS_SIZEOF_BOOL == 1
		{
			// Set the value to true if it is not greater than 0

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on register1.

			volatile int    *regPtr  = (int*)&register1;
			volatile asBYTE *regBptr = (asBYTE*)&register1;
			asBYTE val = (regPtr[0] <= 0) ? VALUE_OF_BOOLEAN_TRUE : 0;
			regBptr[0] = val; // The result is stored in the lower byte
			regBptr[1] = 0;   // Make sure the rest of the register is 0
			regBptr[2] = 0;
			regBptr[3] = 0;
			regBptr[4] = 0;
			regBptr[5] = 0;
			regBptr[6] = 0;
			regBptr[7] = 0;
		}
#else
		*(int*)&register1 = (*(int*)&register1 > 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
#endif
		l_bc++;
		break;

//--------------------
// negate value

	// Negate the integer value in the variable
	case BC_NEGi:
		*(l_fp - SWORDARG0(l_bc)) = asDWORD(-int(*(l_fp - SWORDARG0(l_bc))));
		l_bc++;
		break;

	// Negate the float value in the variable
	case BC_NEGf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = -*(float*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	// Negate the double value in the variable
	case BC_NEGd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = -*(double*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

//-------------------------
// Increment value pointed to by address in register

	// Increment the short value pointed to by the register
	case BC_INCi16:
		(**(short**)&register1)++;
		l_bc++;
		break;

	// Increment the byte value pointed to by the register
	case BC_INCi8:
		(**(char**)&register1)++;
		l_bc++;
		break;

	// Decrement the short value pointed to by the register
	case BC_DECi16:
		(**(short**)&register1)--;
		l_bc++;
		break;

	// Decrement the byte value pointed to by the register
	case BC_DECi8:
		(**(char**)&register1)--;
		l_bc++;
		break;

	// Increment the integer value pointed to by the register
	case BC_INCi:
		++(**(int**)&register1);
		l_bc++;
		break;

	// Decrement the integer value pointed to by the register
	case BC_DECi:
		--(**(int**)&register1);
		l_bc++;
		break;

	// Increment the float value pointed to by the register
	case BC_INCf:
		++(**(float**)&register1);
		l_bc++;
		break;

	// Decrement the float value pointed to by the register
	case BC_DECf:
		--(**(float**)&register1);
		l_bc++;
		break;

	// Increment the double value pointed to by the register
	case BC_INCd:
		++(**(double**)&register1);
		l_bc++;
		break;

	// Decrement the double value pointed to by the register
	case BC_DECd:
		--(**(double**)&register1);
		l_bc++;
		break;

	// Increment the local integer variable
	case BC_IncVi:
		(*(int*)(l_fp - SWORDARG0(l_bc)))++;
		l_bc++;
		break;

	// Decrement the local integer variable
	case BC_DecVi:
		(*(int*)(l_fp - SWORDARG0(l_bc)))--;
		l_bc++;
		break;

//--------------------
// bits instructions

	// Do a bitwise not on the value in the variable
	case BC_BNOT:
		*(l_fp - SWORDARG0(l_bc)) = ~*(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	// Do a bitwise and of two variables and store the result in a third variable
	case BC_BAND:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) & *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	// Do a bitwise or of two variables and store the result in a third variable
	case BC_BOR:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) | *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	// Do a bitwise xor of two variables and store the result in a third variable
	case BC_BXOR:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) ^ *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	// Do a logical shift left of two variables and store the result in a third variable
	case BC_BSLL:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) << *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	// Do a logical shift right of two variables and store the result in a third variable
	case BC_BSRL:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc)) >> *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	// Do an arithmetic shift right of two variables and store the result in a third variable
	case BC_BSRA:
		*(l_fp - SWORDARG0(l_bc)) = int(*(l_fp - SWORDARG1(l_bc))) >> *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_COPY:
		{
			void *d = (void*)*(size_t*)l_sp; l_sp += PTR_SIZE;
			void *s = (void*)*(size_t*)l_sp;
			if( s == 0 || d == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
			memcpy(d, s, WORDARG0(l_bc)*4);

			// replace the pointer on the stack with the lvalue
			*(size_t**)l_sp = (size_t*)d;
		}
		l_bc++;
		break;

	case BC_SET8:
		l_sp -= 2;
		*(asQWORD*)l_sp = QWORDARG(l_bc);
		l_bc += 3;
		break;

	case BC_RDS8:
#ifndef AS_64BIT_PTR
		*(asQWORD*)(l_sp-1) = *(asQWORD*)*(size_t*)l_sp;
		--l_sp;
#else
		*(asQWORD*)l_sp = *(asQWORD*)*(size_t*)l_sp;
#endif
		l_bc++;
		break;

	case BC_SWAP8:
		{
			asQWORD q = *(asQWORD*)l_sp;
			*(asQWORD*)l_sp = *(asQWORD*)(l_sp+2);
			*(asQWORD*)(l_sp+2) = q;
			l_bc++;
		}
		break;

	//----------------------------
	// Comparisons
	case BC_CMPd:
		{
			double dbl = *(double*)(l_fp - SWORDARG0(l_bc)) - *(double*)(l_fp - SWORDARG1(l_bc));
			if( dbl == 0 )     *(int*)&register1 =  0;
			else if( dbl < 0 ) *(int*)&register1 = -1;
			else               *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPu:
		{
			asDWORD d = *(asDWORD*)(l_fp - SWORDARG0(l_bc));
			asDWORD d2 = *(asDWORD*)(l_fp - SWORDARG1(l_bc));
			if( d == d2 )     *(int*)&register1 =  0;
			else if( d < d2 ) *(int*)&register1 = -1;
			else              *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPf:
		{
			float f = *(float*)(l_fp - SWORDARG0(l_bc)) - *(float*)(l_fp - SWORDARG1(l_bc));
			if( f == 0 )     *(int*)&register1 =  0;
			else if( f < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPi:
		{
			int i = *(int*)(l_fp - SWORDARG0(l_bc)) - *(int*)(l_fp - SWORDARG1(l_bc));
			if( i == 0 )     *(int*)&register1 =  0;
			else if( i < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	//----------------------------
	// Comparisons with constant value
	case BC_CMPIi:
		{
			int i = *(int*)(l_fp - SWORDARG0(l_bc)) - INTARG(l_bc);
			if( i == 0 )     *(int*)&register1 =  0;
			else if( i < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPIf:
		{
			float f = *(float*)(l_fp - SWORDARG0(l_bc)) - FLOATARG(l_bc);
			if( f == 0 )     *(int*)&register1 =  0;
			else if( f < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPIu:
		{
			asDWORD d1 = *(asDWORD*)(l_fp - SWORDARG0(l_bc));
			asDWORD d2 = DWORDARG(l_bc);
			if( d1 == d2 )     *(int*)&register1 =  0;
			else if( d1 < d2 ) *(int*)&register1 = -1;
			else               *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_JMPP:
		l_bc += 1 + (*(int*)(l_fp - SWORDARG0(l_bc)))*2;
		break;

	case BC_PopRPtr:
		*(asPTRWORD*)&register1 = *(asPTRWORD*)l_sp;
		l_sp += PTR_SIZE;
		l_bc++;
		break;

	case BC_PshRPtr:
		l_sp -= PTR_SIZE;
		*(asPTRWORD*)l_sp = *(asPTRWORD*)&register1;
		l_bc++;
		break;

	case BC_STR:
		{
			// Get the string id from the argument
			asWORD w = WORDARG0(l_bc);
			// Push the string pointer on the stack
			const asCString &b = module->GetConstantString(w);
			l_sp -= PTR_SIZE;
			*(asPTRWORD*)l_sp = (asPTRWORD)(size_t)b.AddressOf();
			// Push the string length on the stack
			--l_sp;
			*l_sp = (asDWORD)b.GetLength();
			l_bc++;
		}
		break;

	case BC_CALLSYS:
		{
			// Get function ID from the argument
			int i = INTARG(l_bc);

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			l_sp += CallSystemFunction(i, this, 0);

			// Update the program position after the call so that line number is correct
			l_bc += 2;

			// Should the execution be suspended?
			if( doSuspend )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				status = asEXECUTION_SUSPENDED;
				return;
			}
			// An exception might have been raised
			if( status != asEXECUTION_ACTIVE )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				return;
			}
		}
		break;

	case BC_CALLBND:
		{
			// Get the function ID from the stack
			int i = INTARG(l_bc);
			l_bc += 2;

			asASSERT( i >= 0 );
			asASSERT( i & FUNC_IMPORTED );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			int funcID = module->bindInformations[i&0xFFFF].importedFunction;
			if( funcID == -1 )
			{
				SetInternalException(TXT_UNBOUND_FUNCTION);
				return;
			}
			else
			{
				asCScriptFunction *func = engine->GetScriptFunction(funcID);
				asCModule *callModule = func->module;

				CallScriptFunction(callModule, func);
			}

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != asEXECUTION_ACTIVE )
				return;
		}
		break;

	case BC_SUSPEND:
		if( doProcessSuspend )
		{
			if( lineCallback )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				CallLineCallback();
			}
			l_bc++;
			if( doSuspend )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				status = asEXECUTION_SUSPENDED;
				return;
			}
		}
		else
			l_bc++;
		break;

	case BC_ALLOC:
		{
			asCObjectType *objType = (asCObjectType*)(size_t)PTRARG(l_bc);
			int func = INTARG(l_bc+PTR_SIZE);

			if( objType->flags & asOBJ_SCRIPT_OBJECT )
			{
				// Pre-allocate the memory
				asDWORD *mem = (asDWORD*)engine->CallAlloc(objType);

				// Pre-initialize the memory by calling the constructor for asCScriptObject
				ScriptObject_Construct(objType, (asCScriptObject*)mem);

				// Call the constructor to initalize the memory
				asCScriptFunction *f = engine->scriptFunctions[func];

				asDWORD **a = (asDWORD**)*(size_t*)(l_sp + f->GetSpaceNeededForArguments());
				if( a ) *a = mem;

				// Push the object pointer on the stack
				l_sp -= PTR_SIZE;
				*(size_t*)l_sp = (size_t)mem;

				l_bc += 2+PTR_SIZE;

				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				CallScriptFunction(module, f);

				// Extract the values from the context again
				l_bc = byteCode;
				l_sp = stackPointer;
				l_fp = stackFramePointer;

				// If status isn't active anymore then we must stop
				if( status != asEXECUTION_ACTIVE )
					return;
			}
			else
			{
				// Pre-allocate the memory
				asDWORD *mem = (asDWORD*)engine->CallAlloc(objType);

				if( func )
				{
					// Need to move the values back to the context
					byteCode = l_bc;
					stackPointer = l_sp;
					stackFramePointer = l_fp;

					l_sp += CallSystemFunction(func, this, mem);
				}

				// Pop the variable address from the stack
				asDWORD **a = (asDWORD**)*(size_t*)l_sp;
				l_sp += PTR_SIZE;
				if( a ) *a = mem;

				l_bc += 2+PTR_SIZE;

				// Should the execution be suspended?
				if( doSuspend )
				{
					byteCode = l_bc;
					stackPointer = l_sp;
					stackFramePointer = l_fp;

					status = asEXECUTION_SUSPENDED;
					return;
				}
				// An exception might have been raised
				if( status != asEXECUTION_ACTIVE )
				{
					byteCode = l_bc;
					stackPointer = l_sp;
					stackFramePointer = l_fp;

					engine->CallFree(mem);
					*a = 0;

					return;
				}
			}
		}
		break;

	case BC_FREE:
		{
			asDWORD **a = (asDWORD**)*(size_t*)l_sp;
			l_sp += PTR_SIZE;
			if( a && *a )
			{
				asCObjectType *objType = (asCObjectType*)(size_t)PTRARG(l_bc);
				asSTypeBehaviour *beh = &objType->beh;

				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				if( beh->release )
				{
					engine->CallObjectMethod(*a, beh->release);

					// The release method will free the memory
				}
				else
				{
					if( beh->destruct )
					{
						// Call the destructor
						engine->CallObjectMethod(*a, beh->destruct);
					}

					engine->CallFree(*a);
				}
				*a = 0;
			}
		}
		l_bc += 1+PTR_SIZE;
		break;

	case BC_LOADOBJ:
		{
			// Move the object pointer from the object variable into the object register
			void **a = (void**)(l_fp - SWORDARG0(l_bc));
			objectType = 0;
			objectRegister = *a;
			*a = 0;
		}
		l_bc++;
		break;

	case BC_STOREOBJ:
		// Move the object pointer from the object register to the object variable
		*(size_t*)(l_fp - SWORDARG0(l_bc)) = size_t(objectRegister);
		objectRegister = 0;
		l_bc++;
		break;

	case BC_GETOBJ:
		{
			size_t *a = (size_t*)(l_sp + WORDARG0(l_bc));
			asDWORD offset = *(asDWORD*)a;
			size_t *v = (size_t*)(l_fp - offset);
			*a = *v;
			*v = 0;
		}
		l_bc++;
		break;

	case BC_REFCPY:
		{
			asCObjectType *objType = (asCObjectType*)(size_t)PTRARG(l_bc);
			asSTypeBehaviour *beh = &objType->beh;
			void **d = (void**)*(size_t*)l_sp;
			l_sp += PTR_SIZE;
			void *s = (void*)*(size_t*)l_sp;

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			if( *d != 0 )
				engine->CallObjectMethod(*d, beh->release);
			if( s != 0 )
				engine->CallObjectMethod(s, beh->addref);
			*d = s;
		}
		l_bc += 1+PTR_SIZE;
		break;

	case BC_CHKREF:
		{
			// Verify if the pointer on the stack is null
			// This is used when validating a pointer that an operator will work on
			size_t a = *(size_t*)l_sp;
			if( a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	case BC_GETOBJREF:
		{
			size_t *a = (size_t*)(l_sp + WORDARG0(l_bc));
			*(size_t**)a = *(size_t**)(l_fp - *a);
		}
		l_bc++;
		break;

	case BC_GETREF:
		{
			size_t *a = (size_t*)(l_sp + WORDARG0(l_bc));
			*(size_t**)a = (size_t*)(l_fp - (int)*a);
		}
		l_bc++;
		break;

	case BC_SWAP48:
		{
			asDWORD d = *(asDWORD*)l_sp;
			asQWORD q = *(asQWORD*)(l_sp+1);
			*(asQWORD*)l_sp = q;
			*(asDWORD*)(l_sp+2) = d;
			l_bc++;
		}
		break;

	case BC_SWAP84:
		{
			asQWORD q = *(asQWORD*)l_sp;
			asDWORD d = *(asDWORD*)(l_sp+2);
			*(asDWORD*)l_sp = d;
			*(asQWORD*)(l_sp+1) = q;
			l_bc++;
		}
		break;

	case BC_OBJTYPE:
		l_sp -= PTR_SIZE;
		*(asPTRWORD*)l_sp = PTRARG(l_bc);
		l_bc += 1+PTR_SIZE;
		break;

	case BC_TYPEID:
		{
			--l_sp;
			asDWORD typeId = DWORDARG(l_bc);
			*l_sp = typeId;
			l_bc += 2;
		}
		break;

	case BC_SetV4:
		*(l_fp - SWORDARG0(l_bc)) = DWORDARG(l_bc);
		l_bc += 2;
		break;

	case BC_SetV8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = QWORDARG(l_bc);
		l_bc += 3;
		break;

	case BC_ADDSi:
		*(size_t*)l_sp = size_t(asPTRWORD(*(size_t*)l_sp) + INTARG(l_bc));
		l_bc += 2;
		break;

	case BC_CpyVtoV4:
		*(l_fp - SWORDARG0(l_bc)) = *(l_fp - SWORDARG1(l_bc));
		l_bc += 2;
		break;

	case BC_CpyVtoV8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc));
		l_bc += 2;
		break;

	case BC_CpyVtoR4:
		*(asDWORD*)&register1 = *(asDWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_CpyVtoR8:
		*(asQWORD*)&register1 = *(asQWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_CpyVtoG4:
		*(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)] = *(asDWORD*)(l_fp - SWORDARG1(l_bc));
		l_bc += 2;
		break;

	case BC_CpyRtoV4:
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asDWORD*)&register1;
		l_bc++;
		break;

	case BC_CpyRtoV8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = register1;
		l_bc++;
		break;

	case BC_CpyGtoV4:
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = *(asDWORD*)module->globalVarPointers[WORDARG1(l_bc)];
		l_bc += 2;
		break;

	case BC_WRTV1:
		// The pointer in the register points to a byte, and *(l_fp - offset) too
		**(asBYTE**)&register1 = *(asBYTE*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_WRTV2:
		// The pointer in the register points to a word, and *(l_fp - offset) too
		**(asWORD**)&register1 = *(asWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_WRTV4:
		**(asDWORD**)&register1 = *(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_WRTV8:
		**(asQWORD**)&register1 = *(asQWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_RDR1:
		{
			// The pointer in the register points to a byte, and *(l_fp - offset) will also point to a byte
			asBYTE *bPtr = (asBYTE*)(l_fp - SWORDARG0(l_bc));
			bPtr[0] = **(asBYTE**)&register1; // read the byte
			bPtr[1] = 0;                      // 0 the rest of the DWORD
			bPtr[2] = 0;
			bPtr[3] = 0;
		}
		l_bc++;
		break;

	case BC_RDR2:
		{
			// The pointer in the register points to a word, and *(l_fp - offset) will also point to a word
			asWORD *wPtr = (asWORD*)(l_fp - SWORDARG0(l_bc));
			wPtr[0] = **(asWORD**)&register1; // read the word
			wPtr[1] = 0;                      // 0 the rest of the DWORD
		}
		l_bc++;
		break;

	case BC_RDR4:
		*(asDWORD*)(l_fp - SWORDARG0(l_bc)) = **(asDWORD**)&register1;
		l_bc++;
		break;

	case BC_RDR8:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = **(asQWORD**)&register1;
		l_bc++;
		break;

	case BC_LDG:
		*(asDWORD**)&register1 = (asDWORD*)module->globalVarPointers[WORDARG0(l_bc)];
		l_bc++;
		break;

	case BC_LDV:
		*(asDWORD**)&register1 = (l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_PGA:
		l_sp -= PTR_SIZE;
		*(asPTRWORD*)l_sp = (asPTRWORD)(size_t)module->globalVarPointers[WORDARG0(l_bc)];
		l_bc++;
		break;

	case BC_RDS4:
#ifndef AS_64BIT_PTR
		*l_sp = *(asDWORD*)*(size_t*)l_sp;
#else
		{
			asDWORD d = *(asDWORD*)*(size_t*)l_sp;
			l_sp++;
			*l_sp = d;
		}
#endif
		l_bc++;
		break;

	case BC_VAR:
		l_sp -= PTR_SIZE;
		*(size_t*)l_sp = (size_t)SWORDARG0(l_bc);
		l_bc++;
		break;

	//----------------------------
	// Type conversions
	case BC_iTOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(int*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_fTOi:
		*(l_fp - SWORDARG0(l_bc)) = int(*(float*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_uTOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_fTOu:
		// We must cast to int first, because on some compilers the cast of a negative float value to uint result in 0
		*(l_fp - SWORDARG0(l_bc)) = asUINT(int(*(float*)(l_fp - SWORDARG0(l_bc))));
		l_bc++;
		break;

	case BC_sbTOi:
		// *(l_fp - offset) points to a char, and will point to an int afterwards
		*(l_fp - SWORDARG0(l_bc)) = *(signed char*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_swTOi:
		// *(l_fp - offset) points to a short, and will point to an int afterwards
		*(l_fp - SWORDARG0(l_bc)) = *(short*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_ubTOi:
		// (l_fp - offset) points to a byte, and will point to an int afterwards
		*(l_fp - SWORDARG0(l_bc)) = *(asBYTE*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_uwTOi:
		// *(l_fp - offset) points to a word, and will point to an int afterwards
		*(l_fp - SWORDARG0(l_bc)) = *(asWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_dTOi:
		*(l_fp - SWORDARG0(l_bc)) = int(*(double*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_dTOu:
		// We must cast to int first, because on some compilers the cast of a negative float value to uint result in 0
		*(l_fp - SWORDARG0(l_bc)) = asUINT(int(*(double*)(l_fp - SWORDARG1(l_bc))));
		l_bc += 2;
		break;

	case BC_dTOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(double*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_iTOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(int*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_uTOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(asUINT*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_fTOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(float*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	//------------------------------
	// Math operations
	case BC_ADDi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) + *(int*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_SUBi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) - *(int*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) * *(int*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVi:
		{
			int divider = *(int*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) / divider;
		}
		l_bc += 2;
		break;

	case BC_MODi:
		{
			int divider = *(int*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) % divider;
		}
		l_bc += 2;
		break;

	case BC_ADDf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) + *(float*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_SUBf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) - *(float*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) * *(float*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVf:
		{
			float divider = *(float*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) / divider;
		}
		l_bc += 2;
		break;

	case BC_MODf:
		{
			float divider = *(float*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(float*)(l_fp - SWORDARG0(l_bc)) = fmodf(*(float*)(l_fp - SWORDARG1(l_bc)), divider);
		}
		l_bc += 2;
		break;

	case BC_ADDd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) + *(double*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_SUBd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) - *(double*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) * *(double*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVd:
		{
			double divider = *(double*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}

			*(double*)(l_fp - SWORDARG0(l_bc)) = *(double*)(l_fp - SWORDARG1(l_bc)) / divider;
			l_bc += 2;
		}
		break;

	case BC_MODd:
		{
			double divider = *(double*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}

			*(double*)(l_fp - SWORDARG0(l_bc)) = fmod(*(double*)(l_fp - SWORDARG1(l_bc)), divider);
			l_bc += 2;
		}
		break;

	//------------------------------
	// Math operations with constant value
	case BC_ADDIi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) + INTARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_SUBIi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) - INTARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_MULIi:
		*(int*)(l_fp - SWORDARG0(l_bc)) = *(int*)(l_fp - SWORDARG1(l_bc)) * INTARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_ADDIf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) + FLOATARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_SUBIf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) - FLOATARG(l_bc+1);
		l_bc += 3;
		break;

	case BC_MULIf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = *(float*)(l_fp - SWORDARG1(l_bc)) * FLOATARG(l_bc+1);
		l_bc += 3;
		break;

	//-----------------------------------
	case BC_SetG4:
		*(asDWORD*)module->globalVarPointers[WORDARG0(l_bc)] = DWORDARG(l_bc);
		l_bc += 2;
		break;

	case BC_ChkRefS:
		{
			// Verify if the pointer on the stack refers to a non-null value
			// This is used to validate a reference to a handle
			asDWORD *a = (asDWORD*)*(size_t*)l_sp;
			if( *a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	case BC_ChkNullV:
		{
			// Verify if variable (on the stack) is not null
			asDWORD *a = *(asDWORD**)(l_fp - SWORDARG0(l_bc));
			if( a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	case BC_CALLINTF:
		{
			int i = INTARG(l_bc);
			l_bc += 2;

			asASSERT( i >= 0 );
			asASSERT( (i & FUNC_IMPORTED) == 0 );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			CallInterfaceMethod(module, module->GetScriptFunction(i));

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != asEXECUTION_ACTIVE )
				return;
		}
		break;

	case BC_iTOb:
		{
			// *(l_fp - offset) points to an int, and will point to a byte afterwards

			// We need to use volatile here to tell the compiler not to rearrange
			// read and write operations during optimizations.
			volatile asDWORD val  = *(l_fp - SWORDARG0(l_bc));
			volatile asBYTE *bPtr = (asBYTE*)(l_fp - SWORDARG0(l_bc));
			bPtr[0] = (asBYTE)val; // write the byte
			bPtr[1] = 0;           // 0 the rest of the DWORD
			bPtr[2] = 0;
			bPtr[3] = 0;
		}
		l_bc++;
		break;

	case BC_iTOw:
		{
			// *(l_fp - offset) points to an int, and will point to word afterwards

			// We need to use volatile here to tell the compiler not to rearrange
			// read and write operations during optimizations.
			volatile asDWORD val  = *(l_fp - SWORDARG0(l_bc));
			volatile asWORD *wPtr = (asWORD*)(l_fp - SWORDARG0(l_bc));
			wPtr[0] = (asWORD)val; // write the word
			wPtr[1] = 0;           // 0 the rest of the DWORD
		}
		l_bc++;
		break;

	case BC_SetV1:
		// The byte is already stored correctly in the argument
		*(l_fp - SWORDARG0(l_bc)) = DWORDARG(l_bc);
		l_bc += 2;
		break;

	case BC_SetV2:
		// The word is already stored correctly in the argument
		*(l_fp - SWORDARG0(l_bc)) = DWORDARG(l_bc);
		l_bc += 2;
		break;

	case BC_Cast:
		// Cast the handle at the top of the stack to the type in the argument
		{
			asDWORD **a = (asDWORD**)*(size_t*)l_sp;
			if( a && *a )
			{
				asDWORD typeId = DWORDARG(l_bc);

				asCScriptObject *obj = (asCScriptObject *)* a;
				asCObjectType *objType = obj->objType;
				asCObjectType *to = engine->GetObjectTypeFromTypeId(typeId);
				if( objType->Implements(to) || objType->DerivesFrom(to) )
				{
					objectType = 0;
					objectRegister = obj;
					obj->AddRef();
				}
			}
			l_sp += PTR_SIZE;
		}
		l_bc += 2;
		break;

	case BC_i64TOi:
		*(l_fp - SWORDARG0(l_bc)) = int(*(asINT64*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_uTOi64:
		*(asINT64*)(l_fp - SWORDARG0(l_bc)) = asINT64(*(asUINT*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_iTOi64:
		*(asINT64*)(l_fp - SWORDARG0(l_bc)) = asINT64(*(int*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_fTOi64:
		*(asINT64*)(l_fp - SWORDARG0(l_bc)) = asINT64(*(float*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_dTOi64:
		*(asINT64*)(l_fp - SWORDARG0(l_bc)) = asINT64(*(double*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_fTOu64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = asQWORD(asINT64(*(float*)(l_fp - SWORDARG1(l_bc))));
		l_bc += 2;
		break;

	case BC_dTOu64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = asQWORD(asINT64(*(double*)(l_fp - SWORDARG0(l_bc))));
		l_bc++;
		break;

	case BC_i64TOf:
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(asINT64*)(l_fp - SWORDARG1(l_bc)));
		l_bc += 2;
		break;

	case BC_u64TOf:
#if _MSC_VER <= 1200 // MSVC6 
		{
			// MSVC6 doesn't permit UINT64 to double
			asINT64 v = *(asINT64*)(l_fp - SWORDARG1(l_bc));
			if( v < 0 )
				*(float*)(l_fp - SWORDARG0(l_bc)) = 18446744073709551615.0f+float(v);
			else
				*(float*)(l_fp - SWORDARG0(l_bc)) = float(v);
		}
#else
		*(float*)(l_fp - SWORDARG0(l_bc)) = float(*(asQWORD*)(l_fp - SWORDARG1(l_bc)));
#endif
		l_bc += 2;
		break;

	case BC_i64TOd:
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(asINT64*)(l_fp - SWORDARG0(l_bc)));
		l_bc++;
		break;

	case BC_u64TOd:
#if _MSC_VER <= 1200 // MSVC6 
		{
			// MSVC6 doesn't permit UINT64 to double
			asINT64 v = *(asINT64*)(l_fp - SWORDARG0(l_bc));
			if( v < 0 )
				*(double*)(l_fp - SWORDARG0(l_bc)) = 18446744073709551615.0+double(v);
			else
				*(double*)(l_fp - SWORDARG0(l_bc)) = double(v);
		}
#else
		*(double*)(l_fp - SWORDARG0(l_bc)) = double(*(asQWORD*)(l_fp - SWORDARG0(l_bc)));
#endif
		l_bc++;
		break;

	case BC_NEGi64:
		*(asINT64*)(l_fp - SWORDARG0(l_bc)) = -*(asINT64*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_INCi64:
		++(**(asQWORD**)&register1);
		l_bc++;
		break;

	case BC_DECi64:
		--(**(asQWORD**)&register1);
		l_bc++;
		break;

	case BC_BNOT64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = ~*(asQWORD*)(l_fp - SWORDARG0(l_bc));
		l_bc++;
		break;

	case BC_ADDi64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) + *(asQWORD*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_SUBi64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) - *(asQWORD*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_MULi64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) * *(asQWORD*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_DIVi64:
		{
			asQWORD divider = *(asQWORD*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) / divider;
		}
		l_bc += 2;
		break;

	case BC_MODi64:
		{
			asQWORD divider = *(asQWORD*)(l_fp - SWORDARG2(l_bc));
			if( divider == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) % divider;
		}
		l_bc += 2;
		break;

	case BC_BAND64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) & *(asQWORD*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BOR64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) | *(asQWORD*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BXOR64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) ^ *(asQWORD*)(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BSLL64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) << *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BSRL64:
		*(asQWORD*)(l_fp - SWORDARG0(l_bc)) = *(asQWORD*)(l_fp - SWORDARG1(l_bc)) >> *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_BSRA64:
		*(asINT64*)(l_fp - SWORDARG0(l_bc)) = *(asINT64*)(l_fp - SWORDARG1(l_bc)) >> *(l_fp - SWORDARG2(l_bc));
		l_bc += 2;
		break;

	case BC_CMPi64:
		{
			asINT64 i = *(asINT64*)(l_fp - SWORDARG0(l_bc)) - *(asINT64*)(l_fp - SWORDARG1(l_bc));
			if( i == 0 )     *(int*)&register1 =  0;
			else if( i < 0 ) *(int*)&register1 = -1;
			else             *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_CMPu64:
		{
			asQWORD d = *(asQWORD*)(l_fp - SWORDARG0(l_bc));
			asQWORD d2 = *(asQWORD*)(l_fp - SWORDARG1(l_bc));
			if( d == d2 )     *(int*)&register1 =  0;
			else if( d < d2 ) *(int*)&register1 = -1;
			else              *(int*)&register1 =  1;
			l_bc += 2;
		}
		break;

	case BC_ChkNullS:
		{
			// Verify if the pointer on the stack is null
			// This is used for example when validating handles passed as function arguments
			size_t a = *(size_t*)(l_sp + WORDARG0(l_bc));
			if( a == 0 )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
		}
		l_bc++;
		break;

	case BC_ClrHi:
#if AS_SIZEOF_BOOL == 1
		{
			// Clear the upper bytes, so that trash data don't interfere with boolean operations

			// We need to use volatile here to tell the compiler it cannot
			// change the order of read and write operations on the pointer.

			volatile asBYTE *ptr = (asBYTE*)&register1;
			ptr[1] = 0;   // The boolean value is stored in the lower byte, so we clear the rest
			ptr[2] = 0;
			ptr[3] = 0;
		}
#else
		// We don't have anything to do here
#endif
		l_bc++;
		break;

	// Don't let the optimizer optimize for size,
	// since it requires extra conditions and jumps
	case 175: l_bc = (asDWORD*)175; break;
	case 176: l_bc = (asDWORD*)176; break;
	case 177: l_bc = (asDWORD*)177; break;
	case 178: l_bc = (asDWORD*)178; break;
	case 179: l_bc = (asDWORD*)179; break;
	case 180: l_bc = (asDWORD*)180; break;
	case 181: l_bc = (asDWORD*)181; break;
	case 182: l_bc = (asDWORD*)182; break;
	case 183: l_bc = (asDWORD*)183; break;
	case 184: l_bc = (asDWORD*)184; break;
	case 185: l_bc = (asDWORD*)185; break;
	case 186: l_bc = (asDWORD*)186; break;
	case 187: l_bc = (asDWORD*)187; break;
	case 188: l_bc = (asDWORD*)188; break;
	case 189: l_bc = (asDWORD*)189; break;
	case 190: l_bc = (asDWORD*)190; break;
	case 191: l_bc = (asDWORD*)191; break;
	case 192: l_bc = (asDWORD*)192; break;
	case 193: l_bc = (asDWORD*)193; break;
	case 194: l_bc = (asDWORD*)194; break;
	case 195: l_bc = (asDWORD*)195; break;
	case 196: l_bc = (asDWORD*)196; break;
	case 197: l_bc = (asDWORD*)197; break;
	case 198: l_bc = (asDWORD*)198; break;
	case 199: l_bc = (asDWORD*)199; break;
	case 200: l_bc = (asDWORD*)200; break;
	case 201: l_bc = (asDWORD*)201; break;
	case 202: l_bc = (asDWORD*)202; break;
	case 203: l_bc = (asDWORD*)203; break;
	case 204: l_bc = (asDWORD*)204; break;
	case 205: l_bc = (asDWORD*)205; break;
	case 206: l_bc = (asDWORD*)206; break;
	case 207: l_bc = (asDWORD*)207; break;
	case 208: l_bc = (asDWORD*)208; break;
	case 209: l_bc = (asDWORD*)209; break;
	case 210: l_bc = (asDWORD*)210; break;
	case 211: l_bc = (asDWORD*)211; break;
	case 212: l_bc = (asDWORD*)212; break;
	case 213: l_bc = (asDWORD*)213; break;
	case 214: l_bc = (asDWORD*)214; break;
	case 215: l_bc = (asDWORD*)215; break;
	case 216: l_bc = (asDWORD*)216; break;
	case 217: l_bc = (asDWORD*)217; break;
	case 218: l_bc = (asDWORD*)218; break;
	case 219: l_bc = (asDWORD*)219; break;
	case 220: l_bc = (asDWORD*)220; break;
	case 221: l_bc = (asDWORD*)221; break;
	case 222: l_bc = (asDWORD*)222; break;
	case 223: l_bc = (asDWORD*)223; break;
	case 224: l_bc = (asDWORD*)224; break;
	case 225: l_bc = (asDWORD*)225; break;
	case 226: l_bc = (asDWORD*)226; break;
	case 227: l_bc = (asDWORD*)227; break;
	case 228: l_bc = (asDWORD*)228; break;
	case 229: l_bc = (asDWORD*)229; break;
	case 230: l_bc = (asDWORD*)230; break;
	case 231: l_bc = (asDWORD*)231; break;
	case 232: l_bc = (asDWORD*)232; break;
	case 233: l_bc = (asDWORD*)233; break;
	case 234: l_bc = (asDWORD*)234; break;
	case 235: l_bc = (asDWORD*)235; break;
	case 236: l_bc = (asDWORD*)236; break;
	case 237: l_bc = (asDWORD*)237; break;
	case 238: l_bc = (asDWORD*)238; break;
	case 239: l_bc = (asDWORD*)239; break;
	case 240: l_bc = (asDWORD*)240; break;
	case 241: l_bc = (asDWORD*)241; break;
	case 242: l_bc = (asDWORD*)242; break;
	case 243: l_bc = (asDWORD*)243; break;
	case 244: l_bc = (asDWORD*)244; break;
	case 245: l_bc = (asDWORD*)245; break;
	case 246: l_bc = (asDWORD*)246; break;
	case 247: l_bc = (asDWORD*)247; break;
	case 248: l_bc = (asDWORD*)248; break;
	case 249: l_bc = (asDWORD*)249; break;
	case 250: l_bc = (asDWORD*)250; break;
	case 251: l_bc = (asDWORD*)251; break;
	case 252: l_bc = (asDWORD*)252; break;
	case 253: l_bc = (asDWORD*)253; break;
	case 254: l_bc = (asDWORD*)254; break;
	case 255: l_bc = (asDWORD*)255; break;

#ifdef AS_DEBUG
	default:
		asASSERT(false);
#endif
/*
	default:
		// This Microsoft specific code allows the
		// compiler to optimize the switch case as
		// it will know that the code will never
		// reach this point
		__assume(0);
*/	}

#ifdef AS_DEBUG
		asDWORD instr = *(asBYTE*)old;
		if( instr != BC_JMP && instr != BC_JMPP && (instr < BC_JZ || instr > BC_JNP) &&
			instr != BC_CALL && instr != BC_CALLBND && instr != BC_CALLINTF && instr != BC_RET && instr != BC_ALLOC )
		{
			asASSERT( (l_bc - old) == asCByteCode::SizeOfType(bcTypes[instr]) );
		}
#endif
	}

	SetInternalException(TXT_UNRECOGNIZED_BYTE_CODE);
}

int asCContext::SetException(const char *descr)
{
	// Only allow this if we're executing a CALL byte code
	if( !isCallingSystemFunction ) return asERROR;

	SetInternalException(descr);

	return 0;
}

void asCContext::SetInternalException(const char *descr)
{
	if( inExceptionHandler )
	{
		asASSERT(false); // Shouldn't happen
		return; // but if it does, at least this will not crash the application
	}

	status = asEXECUTION_EXCEPTION;

	exceptionString   = descr;
	exceptionFunction = currentFunction->id;
	exceptionLine     = currentFunction->GetLineNumber(int(byteCode - currentFunction->byteCode.AddressOf()));
	exceptionColumn   = exceptionLine >> 20;
	exceptionLine    &= 0xFFFFF;

	if( exceptionCallback )
		CallExceptionCallback();
}

void asCContext::CleanReturnObject()
{
	if( objectRegister == 0 ) return;

	asASSERT( objectType != 0 );

	if( objectType )
	{
		// Call the destructor on the object
		asSTypeBehaviour *beh = &objectType->beh;
		if( beh->release )
		{
			engine->CallObjectMethod(objectRegister, beh->release);
			objectRegister = 0;

			// The release method is responsible for freeing the memory
		}
		else
		{
			if( beh->destruct )
				engine->CallObjectMethod(objectRegister, beh->destruct);

			// Free the memory
			engine->CallFree(objectRegister);
			objectRegister = 0;
		}
	}
}

void asCContext::CleanStack()
{
	inExceptionHandler = true;

	// Run the clean up code for each of the functions called
	CleanStackFrame();

	while( callStack.GetLength() > 0 )
	{
		PopCallState();

		CleanStackFrame();
	}
	inExceptionHandler = false;
}

void asCContext::CleanStackFrame()
{
	// Clean object variables
	if( !isStackMemoryNotAllocated )
	{
		for( asUINT n = 0; n < currentFunction->objVariablePos.GetLength(); n++ )
		{
			int pos = currentFunction->objVariablePos[n];
			if( *(size_t*)&stackFramePointer[-pos] )
			{
				// Call the object's destructor
				asSTypeBehaviour *beh = &currentFunction->objVariableTypes[n]->beh;
				if( beh->release )
				{
					engine->CallObjectMethod((void*)*(size_t*)&stackFramePointer[-pos], beh->release);
					*(size_t*)&stackFramePointer[-pos] = 0;
				}
				else
				{
					if( beh->destruct )
						engine->CallObjectMethod((void*)*(size_t*)&stackFramePointer[-pos], beh->destruct);

					// Free the memory
					engine->CallFree((void*)*(size_t*)&stackFramePointer[-pos]);
					*(size_t*)&stackFramePointer[-pos] = 0;
				}
			}
		}
	}
	else
		isStackMemoryNotAllocated = false;

	// Functions that do not own the object and parameters shouldn't do any clean up
	if( currentFunction->dontCleanUpOnException )
		return;

	// Clean object and parameters
	int offset = 0;
	if( currentFunction->objectType )
	{
		offset += PTR_SIZE;

		// If the object is a script declared object, then we must release it
		// as the compiler adds a reference at the entry of the function
		asSTypeBehaviour *beh = &currentFunction->objectType->beh;
		if( beh->release && *(size_t*)&stackFramePointer[0] != 0 )
		{
			engine->CallObjectMethod((void*)*(size_t*)&stackFramePointer[0], beh->release);
			*(size_t*)&stackFramePointer[0] = 0;
		}
	}
	for( asUINT n = 0; n < currentFunction->parameterTypes.GetLength(); n++ )
	{
		if( currentFunction->parameterTypes[n].IsObject() && !currentFunction->parameterTypes[n].IsReference() )
		{
			if( *(size_t*)&stackFramePointer[offset] )
			{
				// Call the object's destructor
				asSTypeBehaviour *beh = currentFunction->parameterTypes[n].GetBehaviour();
				if( beh->release )
				{
					engine->CallObjectMethod((void*)*(size_t*)&stackFramePointer[offset], beh->release);
					*(size_t*)&stackFramePointer[offset] = 0;
				}
				else
				{
					if( beh->destruct )
						engine->CallObjectMethod((void*)*(size_t*)&stackFramePointer[offset], beh->destruct);

					// Free the memory
					engine->CallFree((void*)*(size_t*)&stackFramePointer[offset]);
					*(size_t*)&stackFramePointer[offset] = 0;
				}
			}
		}

		offset += currentFunction->parameterTypes[n].GetSizeOnStackDWords();
	}
}

int asCContext::GetExceptionLineNumber(int *column)
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	if( column ) *column = exceptionColumn;

	return exceptionLine;
}

int asCContext::GetExceptionFunction()
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	return exceptionFunction;
}

int asCContext::GetCurrentFunction()
{
	if( status == asEXECUTION_SUSPENDED || status == asEXECUTION_ACTIVE )
		return currentFunction->id;

	return -1;
}

int asCContext::GetCurrentLineNumber(int *column)
{
	if( status == asEXECUTION_SUSPENDED || status == asEXECUTION_ACTIVE )
	{
		asDWORD line = currentFunction->GetLineNumber(int(byteCode - currentFunction->byteCode.AddressOf()));
		if( column ) *column = line >> 20;

		return line & 0xFFFFF;
	}

	return -1;
}

const char *asCContext::GetExceptionString()
{
	if( GetState() != asEXECUTION_EXCEPTION ) return 0;

	return exceptionString.AddressOf();
}

asEContextState asCContext::GetState()
{
	return status;
}

int asCContext::SetLineCallback(asSFuncPtr callback, void *obj, int callConv)
{
	lineCallback = true;
	doProcessSuspend = true;
	lineCallbackObj = obj;
	bool isObj = false;
	if( (unsigned)callConv == asCALL_GENERIC )
	{
		lineCallback = false;
		doProcessSuspend = doSuspend;
		return asNOT_SUPPORTED;
	}
	if( (unsigned)callConv >= asCALL_THISCALL )
	{
		isObj = true;
		if( obj == 0 )
		{
			lineCallback = false;
			doProcessSuspend = doSuspend;
			return asINVALID_ARG;
		}
	}

	int r = DetectCallingConvention(isObj, callback, callConv, &lineCallbackFunc);
	if( r < 0 ) lineCallback = false;

	doProcessSuspend = doSuspend || lineCallback;

	return r;
}

void asCContext::CallLineCallback()
{
	if( lineCallbackFunc.callConv < ICC_THISCALL )
		engine->CallGlobalFunction(this, lineCallbackObj, &lineCallbackFunc, 0);
	else
		engine->CallObjectMethod(lineCallbackObj, this, &lineCallbackFunc, 0);
}

int asCContext::SetExceptionCallback(asSFuncPtr callback, void *obj, int callConv)
{
	exceptionCallback = true;
	exceptionCallbackObj = obj;
	bool isObj = false;
	if( (unsigned)callConv == asCALL_GENERIC )
		return asNOT_SUPPORTED;
	if( (unsigned)callConv >= asCALL_THISCALL )
	{
		isObj = true;
		if( obj == 0 )
		{
			exceptionCallback = false;
			return asINVALID_ARG;
		}
	}
	int r = DetectCallingConvention(isObj, callback, callConv, &exceptionCallbackFunc);
	if( r < 0 ) exceptionCallback = false;
	return r;
}

void asCContext::CallExceptionCallback()
{
	if( exceptionCallbackFunc.callConv < ICC_THISCALL )
		engine->CallGlobalFunction(this, exceptionCallbackObj, &exceptionCallbackFunc, 0);
	else
		engine->CallObjectMethod(exceptionCallbackObj, this, &exceptionCallbackFunc, 0);
}

void asCContext::ClearLineCallback()
{
	lineCallback = false;
	doProcessSuspend = doSuspend;
}

void asCContext::ClearExceptionCallback()
{
	exceptionCallback = false;
}

int asCContext::CallGeneric(int id, void *objectPointer)
{
	asCScriptFunction *sysFunction = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = sysFunction->sysFuncIntf;
	void (*func)(asIScriptGeneric*) = (void (*)(asIScriptGeneric*))sysFunc->func;
	int popSize = sysFunc->paramSize;
	asDWORD *args = stackPointer;

	// Verify the object pointer if it is a class method
	void *currentObject = 0;
	if( sysFunc->callConv == ICC_GENERIC_METHOD )
	{
		if( objectPointer )
		{
			currentObject = objectPointer;

			// Don't increase the reference of this pointer
			// since it will not have been constructed yet
		}
		else
		{
			// The object pointer should be popped from the context stack
			popSize += PTR_SIZE;

			// Check for null pointer
			currentObject = (void*)*(size_t*)(args);
			if( currentObject == 0 )
			{
				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return 0;
			}

			// Add the base offset for multiple inheritance
			currentObject = (void*)(size_t(currentObject) + sysFunc->baseOffset);

			// Skip object pointer
			args += PTR_SIZE;
		}
	}

	asCGeneric gen(engine, sysFunction, currentObject, args);

	isCallingSystemFunction = true;
	func(&gen);
	isCallingSystemFunction = false;

	register1 = gen.returnVal;
	objectRegister = gen.objectRegister;
	objectType = sysFunction->returnType.GetObjectType();

	// Clean up function parameters
	int offset = 0;
	for( asUINT n = 0; n < sysFunction->parameterTypes.GetLength(); n++ )
	{
		if( sysFunction->parameterTypes[n].IsObject() && !sysFunction->parameterTypes[n].IsReference() )
		{
			void *obj = *(void**)&args[offset];
			if( obj )
			{
				// Release the object
				asSTypeBehaviour *beh = &sysFunction->parameterTypes[n].GetObjectType()->beh;
				if( beh->release )
					engine->CallObjectMethod(obj, beh->release);
				else
				{
					// Call the destructor then free the memory
					if( beh->destruct )
						engine->CallObjectMethod(obj, beh->destruct);

					engine->CallFree(obj);
				}
			}
		}
		offset += sysFunction->parameterTypes[n].GetSizeOnStackDWords();
	}

	// Return how much should be popped from the stack
	return popSize;
}

int asCContext::GetVarCount(int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return asINVALID_ARG;

	asCScriptFunction *func;
	if( stackLevel == -1 )
		func = currentFunction;
	else
	{
		size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
		func = (asCScriptFunction*)s[1];
	}

	if( func == 0 )
		return asERROR;

	return (int)func->variables.GetLength();
}

const char *asCContext::GetVarName(int varIndex, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	if( stackLevel == -1 )
		func = currentFunction;
	else
	{
		size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
		func = (asCScriptFunction*)s[1];
	}

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;

	return func->variables[varIndex]->name.AddressOf();
}

const char *asCContext::GetVarDeclaration(int varIndex, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	if( stackLevel == -1 )
		func = currentFunction;
	else
	{
		size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
		func = (asCScriptFunction*)s[1];
	}

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;

	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = func->variables[varIndex]->type.Format();
	*tempString += " " + func->variables[varIndex]->name;

	return tempString->AddressOf();
}

int asCContext::GetVarTypeId(int varIndex, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return asINVALID_ARG;

	asCScriptFunction *func;
	if( stackLevel == -1 )
		func = currentFunction;
	else
	{
		size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
		func = (asCScriptFunction*)s[1];
	}

	if( func == 0 )
		return asINVALID_ARG;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return asINVALID_ARG;

	return engine->GetTypeIdFromDataType(func->variables[varIndex]->type);
}

#ifdef AS_DEPRECATED
// deprecated since 2008-11-11, 2.15.0
void *asCContext::GetVarPointer(int varIndex, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	asDWORD *sf;
	if( stackLevel == -1 )
	{
		func = currentFunction;
		sf = stackFramePointer;
	}
	else
	{
		size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
		func = (asCScriptFunction*)s[1];
		sf = (asDWORD*)s[0];
	}

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;

	return sf - func->variables[varIndex]->stackOffset;
}
#endif

void *asCContext::GetAddressOfVar(int varIndex, int stackLevel)
{
	if( stackLevel < -1 || stackLevel >= GetCallstackSize() ) return 0;

	asCScriptFunction *func;
	asDWORD *sf;
	if( stackLevel == -1 )
	{
		func = currentFunction;
		sf = stackFramePointer;
	}
	else
	{
		size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
		func = (asCScriptFunction*)s[1];
		sf = (asDWORD*)s[0];
	}

	if( func == 0 )
		return 0;

	if( varIndex < 0 || varIndex >= (signed)func->variables.GetLength() )
		return 0;

	// For object variables it's necessary to dereference the pointer to get the address of the value
	if( func->variables[varIndex]->type.IsObject() && !func->variables[varIndex]->type.IsObjectHandle() )
		return *(void**)(sf - func->variables[varIndex]->stackOffset);

	return sf - func->variables[varIndex]->stackOffset;
}

// returns the typeId of the 'this' object at the given call stack level (-1 for current)
// returns 0 if the function call at the given stack level is not a method
int asCContext::GetThisTypeId(int stackLevel)
{
       if( stackLevel < -1 || stackLevel >= GetCallstackSize() )
               return 0;

       asCScriptFunction *func = 0;
       if( stackLevel == -1 )
       {
               func = currentFunction;
       }
       else
       {
               size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
               func = (asCScriptFunction*)s[1];
       }

       if( func == 0 )
               return 0;

       if( func->objectType == 0 )
               return 0; // not in a method

       // create a datatype
       asCDataType dt = asCDataType::CreateObject( func->objectType, false);

       // return a typeId from the data type
       return engine->GetTypeIdFromDataType( dt );
}

// returns the 'this' object pointer at the given call stack level (-1 for current)
// returns 0 if the function call at the given stack level is not a method
void *asCContext::GetThisPointer(int stackLevel)
{
       if( stackLevel < -1 || stackLevel >= GetCallstackSize() )
               return 0;

       asCScriptFunction *func;
       asDWORD *sf;
       if( stackLevel == -1 )
       {
               func = currentFunction;
               sf = stackFramePointer;
       }
       else
       {
               size_t *s = callStack.AddressOf() + stackLevel*CALLSTACK_FRAME_SIZE;
               func = (asCScriptFunction*)s[1];
               sf = (asDWORD*)s[0];
       }

       if( func == 0 )
               return 0;

       if( func->objectType == 0 )
               return 0; // not in a method

       void *thisPointer = (void*)*(size_t*)(sf);
       if( thisPointer == 0 )
       {
               return 0;
       }

       // NOTE: this returns the pointer to the 'this' while the GetVarPointer functions return
       // a pointer to a pointer. I can't imagine someone would want to change the 'this'
       return thisPointer;
}

END_AS_NAMESPACE



