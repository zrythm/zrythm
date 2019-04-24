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
// as_callfunc.cpp
//
// These functions handle the actual calling of system functions
//



#include "as_config.h"
#include "as_callfunc.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

int DetectCallingConvention(bool isMethod, const asSFuncPtr &ptr, int callConv, asSSystemFunctionInterface *internal)
{
	memset(internal, 0, sizeof(asSSystemFunctionInterface));

	internal->func = (size_t)ptr.ptr.f.func;

	// Was a compatible calling convention specified?
	if( internal->func )
	{
		if( ptr.flag == 1 && callConv != asCALL_GENERIC )
			return asWRONG_CALLING_CONV;
		else if( ptr.flag == 2 && (callConv == asCALL_GENERIC || callConv == asCALL_THISCALL) )
			return asWRONG_CALLING_CONV;
		else if( ptr.flag == 3 && callConv != asCALL_THISCALL )
			return asWRONG_CALLING_CONV;
	}

	asDWORD base = callConv;
	if( !isMethod )
	{
		if( base == asCALL_CDECL )
			internal->callConv = ICC_CDECL;
		else if( base == asCALL_STDCALL )
			internal->callConv = ICC_STDCALL;
		else if( base == asCALL_GENERIC )
			internal->callConv = ICC_GENERIC_FUNC;
		else
			return asNOT_SUPPORTED;
	}
	else
	{
#ifndef AS_NO_CLASS_METHODS
		if( base == asCALL_THISCALL )
		{
			internal->callConv = ICC_THISCALL;
#ifdef GNU_STYLE_VIRTUAL_METHOD
			if( (size_t(ptr.ptr.f.func) & 1) )
				internal->callConv = ICC_VIRTUAL_THISCALL;
#endif
			internal->baseOffset = ( int )MULTI_BASE_OFFSET(ptr);

#ifdef HAVE_VIRTUAL_BASE_OFFSET
			// We don't support virtual inheritance
			if( VIRTUAL_BASE_OFFSET(ptr) != 0 )
				return asNOT_SUPPORTED;
#endif
		}
		else
#endif
		if( base == asCALL_CDECL_OBJLAST )
			internal->callConv = ICC_CDECL_OBJLAST;
		else if( base == asCALL_CDECL_OBJFIRST )
			internal->callConv = ICC_CDECL_OBJFIRST;
		else if( base == asCALL_GENERIC )
			internal->callConv = ICC_GENERIC_METHOD;
		else
			return asNOT_SUPPORTED;
	}

	return 0;
}

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunctionGeneric(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine * /*engine*/)
{
	asASSERT(internal->callConv == ICC_GENERIC_METHOD || internal->callConv == ICC_GENERIC_FUNC);

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	return 0;
}

END_AS_NAMESPACE


#ifdef AS_MAX_PORTABILITY

#include "as_texts.h"

BEGIN_AS_NAMESPACE


// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction(asCScriptFunction * /*func*/, asSSystemFunctionInterface * /*internal*/, asCScriptEngine * /*engine*/)
{
	// This should never happen, as when AS_MAX_PORTABILITY is on, all functions 
	// are asCALL_GENERIC, which are prepared by PrepareSystemFunctionGeneric
	asASSERT(false);

	return 0;
}

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asSSystemFunctionInterface *sysFunc = engine->scriptFunctions[id]->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	context->SetInternalException(TXT_INVALID_CALLING_CONVENTION);

	return 0;
}

END_AS_NAMESPACE

#endif // AS_MAX_PORTABILITY
