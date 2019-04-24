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
// as_callfunc_arm.cpp
//
// These functions handle the actual calling of system functions on the arm platform
//
// Written by Fredrik Ehnbom in June 2009, based on as_callfunc_x86.cpp


#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_ARM

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"

BEGIN_AS_NAMESPACE

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *)
{
	// References are always returned as primitive data
	if( func->returnType.IsReference() || func->returnType.IsObjectHandle() )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = false;
	}
	// Registered types have special flags that determine how they are returned
	else if( func->returnType.IsObject() )
	{
		asDWORD objType = func->returnType.GetObjectType()->flags;
		if( (objType & asOBJ_VALUE) && (objType & asOBJ_APP_CLASS) )
		{
			if( objType & COMPLEX_MASK )
			{
				internal->hostReturnInMemory = true;
				internal->hostReturnSize = 1;
				internal->hostReturnFloat = false;
			}
			else
			{
				internal->hostReturnFloat = false;
				if( func->returnType.GetSizeInMemoryDWords() > 2 )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
				else
				{
					internal->hostReturnInMemory = false;
					internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
				}

#ifdef THISCALL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_THISCALL ||
					internal->callConv == ICC_VIRTUAL_THISCALL )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
#endif
#ifdef CDECL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_CDECL ||
					internal->callConv == ICC_CDECL_OBJLAST ||
					internal->callConv == ICC_CDECL_OBJFIRST )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
#endif
#ifdef STDCALL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_STDCALL )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
#endif
			}
		}
		else if( (objType & asOBJ_VALUE) && (objType & asOBJ_APP_PRIMITIVE) )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = false;
		}
		else if( (objType & asOBJ_VALUE) && (objType & asOBJ_APP_FLOAT) )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = true;
		}
	}
	// Primitive types can easily be determined
	else if( func->returnType.GetSizeInMemoryDWords() > 2 )
	{
		// Shouldn't be possible to get here
		asASSERT(false);

		internal->hostReturnInMemory = true;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = false;
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 2 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 2;
		internal->hostReturnFloat = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttDouble, true));
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 1 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttFloat, true));
	}
	else
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 0;
		internal->hostReturnFloat = false;
	}

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	// Verify if the function takes any objects by value
	asUINT n;
	internal->takesObjByVal = false;
	for( n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].IsObject() && !func->parameterTypes[n].IsObjectHandle() && !func->parameterTypes[n].IsReference() )
		{
			internal->takesObjByVal = true;
			break;
		}
	}

	// Verify if the function has any registered autohandles
	internal->hasAutoHandles = false;
	for( n = 0; n < internal->paramAutoHandles.GetLength(); n++ )
	{
		if( internal->paramAutoHandles[n] )
		{
			internal->hasAutoHandles = true;
			break;
		}
	}

	return 0;
}

extern "C" asQWORD armFunc(const asDWORD *, int, size_t);
extern "C" asQWORD armFuncR0(const asDWORD *, int, size_t, asDWORD r0);
extern "C" asQWORD armFuncR0R1(const asDWORD *, int, size_t, asDWORD r0, asDWORD r1);
extern "C" asQWORD armFuncObjLast(const asDWORD *, int, size_t, asDWORD obj);
extern "C" asQWORD armFuncR0ObjLast(const asDWORD *, int, size_t, asDWORD r0, asDWORD obj);

// TODO: CallSystemFunction should be split in two layers. The top layer
//       implements the parts that are common to all system functions,
//       e.g. the check for generic calling convention, the extraction of the
//       object pointer, the processing of auto handles, and the pop size.
//       Remember that the proper handling of auto handles is implemented in
//       as_callfunc_x64_gcc.cpp as that code can handle both 32bit and 64bit pointers.
//
//       The lower layer implemented in CallNativeSystemFunction will then
//       be responsible for just transforming the parameters to the native
//       calling convention.
//
//       This should be done for all supported platforms.
int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asCScriptFunction *descr = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = descr->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD  retQW             = 0;
	void    *func              = (void*)sysFunc->func;
	int      paramSize         = sysFunc->paramSize;
	asDWORD *args              = context->stackPointer;
	void    *retPointer        = 0;
	void    *obj               = 0;
	asDWORD *vftable;
	int      popSize           = paramSize;

	context->objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());

		if( sysFunc->hostReturnInMemory )
		{
			// The return is made in memory
			callConv++;
		}
	}

	if( callConv >= ICC_THISCALL )
	{
		if( objectPointer )
		{
			obj = objectPointer;
		}
		else
		{
			// The object pointer should be popped from the context stack
			popSize++;

			// Check for null pointer
			obj = (void*)*(size_t*)(args);
			if( obj == 0 )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retPointer )
					engine->CallFree(retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(size_t(obj) + sysFunc->baseOffset);

			// Skip the object pointer
			args++;
		}
	}

	asDWORD paramBuffer[64];
	if( sysFunc->takesObjByVal )
	{
		paramSize = 0;
		int spos = 0;
		int dpos = 1;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
            if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
			{
#ifdef COMPLEX_OBJS_PASSED_BY_REF
				if( descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK )
				{
					paramBuffer[dpos++] = args[spos++];
					paramSize++;
				}
				else
#endif
				{
					// Copy the object's memory to the buffer
					memcpy(&paramBuffer[dpos], *(void**)(args+spos), descr->parameterTypes[n].GetSizeInMemoryBytes());

					// Delete the original memory
					engine->CallFree(*(char**)(args+spos));
					spos++;
					dpos += descr->parameterTypes[n].GetSizeInMemoryDWords();
					paramSize += descr->parameterTypes[n].GetSizeInMemoryDWords();
				}
			}
			else
			{
				// Copy the value directly
				paramBuffer[dpos++] = args[spos++];
				if( descr->parameterTypes[n].GetSizeOnStackDWords() > 1 )
					paramBuffer[dpos++] = args[spos++];
				paramSize += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}
		// Keep a free location at the beginning
		args = &paramBuffer[1];
	}

	context->isCallingSystemFunction = true;
	switch( callConv )
	{
	case ICC_CDECL_RETURNINMEM:     // fall through
	case ICC_STDCALL_RETURNINMEM:
        retQW = armFuncR0(args, paramSize<<2, (asDWORD)func, (asDWORD) retPointer);
        break;
    case ICC_CDECL:     // fall through
    case ICC_STDCALL:
		retQW = armFunc(args, paramSize<<2, (asDWORD)func);
		break;
    case ICC_THISCALL:  // fall through
	case ICC_CDECL_OBJFIRST:
        retQW = armFuncR0(args, paramSize<<2, (asDWORD)func, (asDWORD) obj);
        break;
    case ICC_THISCALL_RETURNINMEM:
        retQW = armFuncR0R1(args, paramSize<<2, (asDWORD)func, (asDWORD) obj, (asDWORD) retPointer);
		break;
    case ICC_CDECL_OBJFIRST_RETURNINMEM:
        retQW = armFuncR0R1(args, paramSize<<2, (asDWORD)func, (asDWORD) retPointer, (asDWORD) obj);
		break;
	case ICC_VIRTUAL_THISCALL:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;
        retQW = armFuncR0(args, paramSize<<2, vftable[asDWORD(func)>>2], (asDWORD) obj);
        break;
    case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;
        retQW = armFuncR0R1(args, (paramSize+1)<<2, vftable[asDWORD(func)>>2], (asDWORD) obj, (asDWORD) retPointer);
		break;
	case ICC_CDECL_OBJLAST:
		retQW = armFuncObjLast(args, paramSize<<2, (asDWORD)func, (asDWORD) obj);
        break;
    case ICC_CDECL_OBJLAST_RETURNINMEM:
		retQW = armFuncR0ObjLast(args, paramSize<<2, (asDWORD)func, (asDWORD) retPointer, (asDWORD) obj);
		break;
	default:
		context->SetInternalException(TXT_INVALID_CALLING_CONVENTION);
	}
	context->isCallingSystemFunction = false;
#ifdef COMPLEX_OBJS_PASSED_BY_REF
	if( sysFunc->takesObjByVal )
	{
		// Need to free the complex objects passed by value
		args = context->stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
		    args++;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsReference() &&
                !descr->parameterTypes[n].IsObjectHandle()
                )
            {
                if (descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK)
                {
			        void *obj = (void*)args[spos++];
                    // TODO: running the destructor here results in the destructor being run twice
                    // as it has already been called in the native function.
                    // Should this be an ifdef or removed completely?
//			        asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
//			        if( beh->destruct )
//				        engine->CallObjectMethod(obj, beh->destruct);

			        engine->CallFree(obj);
                }
                else
                {
                    // The original data was already freed earlier
                    spos += descr->parameterTypes[n].GetSizeInMemoryDWords();
                }
            }
		    else
            {
			    spos += descr->parameterTypes[n].GetSizeOnStackDWords();
            }
        }
	}
#endif

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() )
	{
		if( descr->returnType.IsObjectHandle() )
		{
			context->objectRegister = (void*)(size_t)retQW;

			if( sysFunc->returnAutoHandle && context->objectRegister )
				engine->CallObjectMethod(context->objectRegister, descr->returnType.GetObjectType()->beh.addref);
		}
		else
		{
			if( !sysFunc->hostReturnInMemory )
			{
				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 )
					*(asDWORD*)retPointer = (asDWORD)retQW;
				else
					*(asQWORD*)retPointer = retQW;
			}

			// Store the object in the register
			context->objectRegister = retPointer;
		}
	}
	else
	{
		// Store value in register1 register
		if( sysFunc->hostReturnFloat )
		{
			if( sysFunc->hostReturnSize == 1 )
				*(asDWORD*)&context->register1 = (asDWORD) retQW;
			else
				context->register1 = retQW;
		}
		else if( sysFunc->hostReturnSize == 1 )
			*(asDWORD*)&context->register1 = (asDWORD)retQW;
		else
			context->register1 = retQW;
	}

	if( sysFunc->hasAutoHandles )
	{
		args = context->stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args++;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( sysFunc->paramAutoHandles[n] && args[spos] )
			{
				// Call the release method on the type
				engine->CallObjectMethod((void*)*(size_t*)&args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
				args[spos] = 0;
			}

			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
				spos++;
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}

	return popSize;
}

END_AS_NAMESPACE

#endif // AS_ARM
#endif // AS_MAX_PORTABILITY




