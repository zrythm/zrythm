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
// as_module.cpp
//
// A class that holds a script module
//

#include "as_config.h"
#include "as_module.h"
#include "as_builder.h"
#include "as_context.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

// internal
asCModule::asCModule(const char *name, int id, asCScriptEngine *engine)
{
	this->name     = name;
	this->engine   = engine;
	this->moduleId = id;

	builder = 0;
	isDiscarded = false;
	isBuildWithoutErrors = false;
	contextCount.set(0);
	moduleCount.set(0);
	isGlobalVarInitialized = false;
	initFunction = 0;
}

// internal
asCModule::~asCModule()
{
	InternalReset();

	if( builder ) 
	{
		asDELETE(builder,asCBuilder);
		builder = 0;
	}

	// Remove the module from the engine
	if( engine )
	{
		if( engine->lastModule == this )
			engine->lastModule = 0;

		int index = (moduleId >> 16);
		engine->scriptModules[index] = 0;
	}
}

// interface
asIScriptEngine *asCModule::GetEngine()
{
	return engine;
}

// interface
void asCModule::SetName(const char *name)
{
	this->name = name;
}

// interface
const char *asCModule::GetName()
{
	return name.AddressOf();
}

// interface
int asCModule::AddScriptSection(const char *name, const char *code, size_t codeLength, int lineOffset)
{
	if( IsUsed() )
		return asMODULE_IS_IN_USE;

	if( !builder )
		builder = asNEW(asCBuilder)(engine, this);

	builder->AddCode(name, code, (int)codeLength, lineOffset, (int)builder->scripts.GetLength(), engine->ep.copyScriptSections);

	return asSUCCESS;
}

// interface
int asCModule::Build()
{
	asASSERT( contextCount.get() == 0 );
	
	// Only one thread may build at one time
	int r = engine->RequestBuild();
	if( r < 0 )
		return r;

	engine->PrepareEngine();
	if( engine->configFailed )
	{
		engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, TXT_INVALID_CONFIGURATION);
		engine->BuildCompleted();
		return asINVALID_CONFIGURATION;
	}

 	InternalReset();

	if( !builder )
	{
		engine->BuildCompleted();
		return asSUCCESS;
	}

	// Store the section names
	for( size_t n = 0; n < builder->scripts.GetLength(); n++ )
	{
		asCString *sectionName = asNEW(asCString)(builder->scripts[n]->name);
		scriptSections.PushLast(sectionName);
	}

	// Compile the script
	r = builder->Build();
	asDELETE(builder,asCBuilder);
	builder = 0;
	
	if( r < 0 )
	{
		// Reset module again
		InternalReset();

		isBuildWithoutErrors = false;
		engine->BuildCompleted();
		return r;
	}

	isBuildWithoutErrors = true;

	engine->PrepareEngine();
	engine->BuildCompleted();

	// Initialize global variables
	if( r >= 0 && engine->ep.initGlobalVarsAfterBuild )
		r = ResetGlobalVars();

	return r;
}

// interface
int asCModule::ResetGlobalVars()
{
	if( isGlobalVarInitialized ) 
		CallExit();

	if( !isBuildWithoutErrors )
		return asERROR;

	// TODO: The application really should do this manually through a context
	//       otherwise it cannot properly handle script exceptions that may be
	//       thrown by object initializations.
	return CallInit();
}

// interface
int asCModule::GetFunctionIdByIndex(int index)
{
	if( index < 0 || index >= (int)scriptFunctions.GetLength() )
		return asNO_FUNCTION;

	return scriptFunctions[index]->id;
}

// internal
int asCModule::CallInit()
{
	if( isGlobalVarInitialized ) 
		return asERROR;

	// Each global variable needs to be cleared individually
	for( asUINT n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		if( scriptGlobals[n] )
		{
			memset(scriptGlobals[n]->GetAddressOfValue(), 0, sizeof(asDWORD)*scriptGlobals[n]->type.GetSizeOnStackDWords());
		}
	}

	if( initFunction && initFunction->byteCode.GetLength() == 0 ) 
		return asERROR;

	int id = asFUNC_INIT;
	asIScriptContext *ctx = 0;
	int r = engine->CreateContext(&ctx, true);
	if( r >= 0 && ctx )
	{
		r = ((asCContext*)ctx)->PrepareSpecial(id, this);
		if( r >= 0 )
			r = ctx->Execute();
		ctx->Release();
		ctx = 0;
	}

	// Even if the initialization failed we need to set the 
	// flag that the variables have been initialized, otherwise
	// the module won't free those variables that really were 
	// initialized.
	isGlobalVarInitialized = true;

	if( r != asEXECUTION_FINISHED )
		return asINIT_GLOBAL_VARS_FAILED;

	return asSUCCESS;
}

// internal
void asCModule::CallExit()
{
	if( !isGlobalVarInitialized ) return;

	for( size_t n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		if( scriptGlobals[n]->type.IsObject() )
		{
			void *obj = *(void**)scriptGlobals[n]->GetAddressOfValue();
			if( obj )
			{
				asCObjectType *ot = scriptGlobals[n]->type.GetObjectType();

				if( ot->beh.release )
					engine->CallObjectMethod(obj, ot->beh.release);
				else
				{
					if( ot->beh.destruct )
						engine->CallObjectMethod(obj, ot->beh.destruct);

					engine->CallFree(obj);
				}
			}
		}
	}

	isGlobalVarInitialized = false;
}

// internal
void asCModule::Discard()
{
	isDiscarded = true;
}

// internal
void asCModule::InternalReset()
{
	asASSERT( !IsUsed() );

	CallExit();

	size_t n;

	// TODO: This is incorrect. If there are still live objects, the methods shouldn't be destroyed yet.
	//       It should still be possible to discard the module, but the methods should only be destroyed
	//       when all objects have been destroyed. However, we can't just check if there are live objects
	//       when determining if functions can or cannot be destroyed, because then we could create situations
	//       of memory that is never freed, e.g. a global variable in the module, being referenced by one
	//       of it's own class methods. To fix this, we need a full garbage collector that can resolve 
	//       circular references. 

	// Since we're going to delete the script functions, we must 
	// not allow any straying objects call them afterwards. If this is not done
	// the object types' function id will either point to non-existing functions,
	// or possibly to new functions that re-use the old function id. Needless to
	// say, it could be disastrous if the function is called like this.
	for( n = 0; n < classTypes.GetLength(); n++ )
	{
		if( classTypes[n] == 0 ) continue;

		// Skip interfaces that the module doesn't own, as it's methods won't be deleted
		if( classTypes[n]->IsInterface() && classTypes[n]->GetRefCount() > 1 )
			continue;

		// Just set the function id's to 0
		classTypes[n]->beh.factory   = 0;
		classTypes[n]->beh.construct = 0;
		classTypes[n]->beh.destruct  = 0;
		classTypes[n]->beh.factories.SetLength(0);
		classTypes[n]->beh.constructors.SetLength(0);
		classTypes[n]->beh.operators.SetLength(0);
		classTypes[n]->methods.SetLength(0);
	}

	// First free the functions
	for( n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		if( scriptFunctions[n] == 0 )
			continue;

		// Don't delete interface methods, if the module isn't the only owner of the interface
		if( scriptFunctions[n]->objectType && scriptFunctions[n]->objectType->IsInterface() && scriptFunctions[n]->objectType->GetRefCount() > 1 )
			continue;

		engine->DeleteScriptFunction(scriptFunctions[n]->id);
	}
	scriptFunctions.SetLength(0);
	globalFunctions.SetLength(0);

	if( initFunction )
	{
		engine->DeleteScriptFunction(initFunction->id);
		initFunction = 0;
	}

	// Release bound functions
	for( n = 0; n < bindInformations.GetLength(); n++ )
	{
		int oldFuncID = bindInformations[n].importedFunction;
		if( oldFuncID != -1 )
		{
			asCModule *oldModule = engine->GetModuleFromFuncId(oldFuncID);
			if( oldModule != 0 ) 
			{
				// Release reference to the module
				oldModule->ReleaseModuleRef();
			}
		}
	}
	bindInformations.SetLength(0);

	for( n = 0; n < importedFunctions.GetLength(); n++ )
	{
		asDELETE(importedFunctions[n],asCScriptFunction);
	}
	importedFunctions.SetLength(0);

	// Free global variables
	globalVarPointers.SetLength(0);

	isBuildWithoutErrors = true;
	isDiscarded = false;

	for( n = 0; n < stringConstants.GetLength(); n++ )
	{
		asDELETE(stringConstants[n],asCString);
	}
	stringConstants.SetLength(0);

	for( n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		asDELETE(scriptGlobals[n],asCGlobalProperty);
	}
	scriptGlobals.SetLength(0);

	for( n = 0; n < scriptSections.GetLength(); n++ )
	{
		asDELETE(scriptSections[n],asCString);
	}
	scriptSections.SetLength(0);

	// Free declared types, including classes, typedefs, and enums
	for( n = 0; n < classTypes.GetLength(); n++ )
		classTypes[n]->Release();
	classTypes.SetLength(0);
	for( n = 0; n < enumTypes.GetLength(); n++ )
		enumTypes[n]->Release();
	enumTypes.SetLength(0);
	for( n = 0; n < typeDefs.GetLength(); n++ )
		typeDefs[n]->Release();
	typeDefs.SetLength(0);
}

// interface
int asCModule::GetFunctionIdByName(const char *name)
{
	if( isBuildWithoutErrors == false )
		return asERROR;
	
	// TODO: optimize: Improve linear search
	// Find the function id
	int id = -1;
	for( size_t n = 0; n < globalFunctions.GetLength(); n++ )
	{
		if( globalFunctions[n]->name == name )
		{
			if( id == -1 )
				id = globalFunctions[n]->id;
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

// interface
int asCModule::GetImportedFunctionCount()
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	return (int)importedFunctions.GetLength();
}

// interface
int asCModule::GetImportedFunctionIndexByDecl(const char *decl)
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	asCBuilder bld(engine, this);

	asCScriptFunction func(engine, this);
	bld.ParseFunctionDeclaration(0, decl, &func, false);

	// TODO: optimize: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( asUINT n = 0; n < importedFunctions.GetLength(); ++n )
	{
		if( func.name == importedFunctions[n]->name && 
			func.returnType == importedFunctions[n]->returnType &&
			func.parameterTypes.GetLength() == importedFunctions[n]->parameterTypes.GetLength() )
		{
			bool match = true;
			for( asUINT p = 0; p < func.parameterTypes.GetLength(); ++p )
			{
				if( func.parameterTypes[p] != importedFunctions[n]->parameterTypes[p] )
				{
					match = false;
					break;
				}
			}

			if( match )
			{
				if( id == -1 )
					id = n;
				else
					return asMULTIPLE_FUNCTIONS;
			}
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

// interface
int asCModule::GetFunctionCount()
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	return (int)globalFunctions.GetLength();
}

// interface
int asCModule::GetFunctionIdByDecl(const char *decl)
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	asCBuilder bld(engine, this);

	asCScriptFunction func(engine, this);
	int r = bld.ParseFunctionDeclaration(0, decl, &func, false);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// TODO: optimize: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( size_t n = 0; n < globalFunctions.GetLength(); ++n )
	{
		if( globalFunctions[n]->objectType == 0 && 
			func.name == globalFunctions[n]->name && 
			func.returnType == globalFunctions[n]->returnType &&
			func.parameterTypes.GetLength() == globalFunctions[n]->parameterTypes.GetLength() )
		{
			bool match = true;
			for( size_t p = 0; p < func.parameterTypes.GetLength(); ++p )
			{
				if( func.parameterTypes[p] != globalFunctions[n]->parameterTypes[p] )
				{
					match = false;
					break;
				}
			}

			if( match )
			{
				if( id == -1 )
					id = globalFunctions[n]->id;
				else
					return asMULTIPLE_FUNCTIONS;
			}
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

// interface
int asCModule::GetGlobalVarCount()
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	return (int)scriptGlobals.GetLength();
}

// interface
int asCModule::GetGlobalVarIndexByName(const char *name)
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	// Find the global var id
	int id = -1;
	for( size_t n = 0; n < scriptGlobals.GetLength(); n++ )
	{
		if( scriptGlobals[n]->name == name )
		{
			id = (int)n;
			break;
		}
	}

	if( id == -1 ) return asNO_GLOBAL_VAR;

	return id;
}

// interface
asIScriptFunction *asCModule::GetFunctionDescriptorByIndex(int index)
{
	if( index < 0 || index >= (int)globalFunctions.GetLength() )
		return 0;

	return globalFunctions[index];
}

// interface
asIScriptFunction *asCModule::GetFunctionDescriptorById(int funcId)
{
	return engine->GetFunctionDescriptorById(funcId);
}

// interface
int asCModule::GetGlobalVarIndexByDecl(const char *decl)
{
	if( isBuildWithoutErrors == false )
		return asERROR;

	asCBuilder bld(engine, this);

	asCObjectProperty gvar;
	bld.ParseVariableDeclaration(decl, &gvar);

	// TODO: optimize: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( size_t n = 0; n < scriptGlobals.GetLength(); ++n )
	{
		if( gvar.name == scriptGlobals[n]->name && 
			gvar.type == scriptGlobals[n]->type )
		{
			id = (int)n;
			break;
		}
	}

	if( id == -1 ) return asNO_GLOBAL_VAR;

	return id;
}

// interface
void *asCModule::GetAddressOfGlobalVar(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return 0;

	// TODO: value types shouldn't need dereferencing
	// For object variables it's necessary to dereference the pointer to get the address of the value
	if( scriptGlobals[index]->type.IsObject() && !scriptGlobals[index]->type.IsObjectHandle() )
		return *(void**)(scriptGlobals[index]->GetAddressOfValue());

	return (void*)(scriptGlobals[index]->GetAddressOfValue());
}

// interface
const char *asCModule::GetGlobalVarDeclaration(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return 0;

	asCGlobalProperty *prop = scriptGlobals[index];

	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = prop->type.Format();
	*tempString += " " + prop->name;

	return tempString->AddressOf();
}

// interface
const char *asCModule::GetGlobalVarName(int index)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return 0;

	return scriptGlobals[index]->name.AddressOf();
}

// interface
// TODO: If the typeId ever encodes the const flag, then the isConst parameter should be removed
int asCModule::GetGlobalVarTypeId(int index, bool *isConst)
{
	if( index < 0 || index >= (int)scriptGlobals.GetLength() )
		return asINVALID_ARG;

	if( isConst )
		*isConst = scriptGlobals[index]->type.IsReadOnly();

	return engine->GetTypeIdFromDataType(scriptGlobals[index]->type);
}

// interface
int asCModule::GetObjectTypeCount()
{
	return (int)classTypes.GetLength();
}

// interface 
asIObjectType *asCModule::GetObjectTypeByIndex(asUINT index)
{
	if( index >= classTypes.GetLength() ) 
		return 0;

	return classTypes[index];
}

// interface
int asCModule::GetTypeIdByDecl(const char *decl)
{
	asCDataType dt;
	asCBuilder bld(engine, this);
	int r = bld.ParseDataType(decl, &dt);
	if( r < 0 )
		return asINVALID_TYPE;

	return engine->GetTypeIdFromDataType(dt);
}

// interface
int asCModule::GetEnumCount()
{
	return (int)enumTypes.GetLength();
}

// interface
const char *asCModule::GetEnumByIndex(asUINT index, int *enumTypeId)
{
	if( index >= enumTypes.GetLength() )
		return 0;

	if( enumTypeId )
		*enumTypeId = GetTypeIdByDecl(enumTypes[index]->name.AddressOf());

	return enumTypes[index]->name.AddressOf();
}

// interface
int asCModule::GetEnumValueCount(int enumTypeId)
{
	const asCDataType *dt = engine->GetDataTypeFromTypeId(enumTypeId);
	asCObjectType *t = dt->GetObjectType();
	if( t == 0 || !(t->GetFlags() & asOBJ_ENUM) ) 
		return asINVALID_TYPE;

	return (int)t->enumValues.GetLength();
}

// interface
const char *asCModule::GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue)
{
	const asCDataType *dt = engine->GetDataTypeFromTypeId(enumTypeId);
	asCObjectType *t = dt->GetObjectType();
	if( t == 0 || !(t->GetFlags() & asOBJ_ENUM) ) 
		return 0;

	if( index >= t->enumValues.GetLength() )
		return 0;

	if( outValue )
		*outValue = t->enumValues[index]->value;

	return t->enumValues[index]->name.AddressOf();
}

// interface
int asCModule::GetTypedefCount()
{
	return (int)typeDefs.GetLength();
}

// interface
const char *asCModule::GetTypedefByIndex(asUINT index, int *typeId)
{
	if( index >= typeDefs.GetLength() )
		return 0;

	if( typeId )
		*typeId = GetTypeIdByDecl(typeDefs[index]->name.AddressOf());

	return typeDefs[index]->name.AddressOf();
}


// internal
int asCModule::AddConstantString(const char *str, size_t len)
{
	//  The str may contain null chars, so we cannot use strlen, or strcmp, or strcpy
	asCString *cstr = asNEW(asCString)(str, len);

	// TODO: optimize: Improve linear search
	// Has the string been registered before?
	for( size_t n = 0; n < stringConstants.GetLength(); n++ )
	{
		if( *stringConstants[n] == *cstr )
		{
			asDELETE(cstr,asCString);
			return (int)n;
		}
	}

	// No match was found, add the string
	stringConstants.PushLast(cstr);

	return (int)stringConstants.GetLength() - 1;
}

// internal
const asCString &asCModule::GetConstantString(int id)
{
	return *stringConstants[id];
}

// internal
// TODO: this can probably be removed
int asCModule::GetNextFunctionId()
{
	return engine->GetNextScriptFunctionId();
}

// internal
int asCModule::GetNextImportedFunctionId()
{
	return FUNC_IMPORTED | (asUINT)importedFunctions.GetLength();
}

// internal
int asCModule::AddScriptFunction(int sectionIdx, int id, const char *name, const asCDataType &returnType, asCDataType *params, asETypeModifiers *inOutFlags, int paramCount, bool isInterface, asCObjectType *objType, bool isConstMethod, bool isGlobalFunction)
{
	asASSERT(id >= 0);

	// Store the function information
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine, this);
	func->funcType   = isInterface ? asFUNC_INTERFACE : asFUNC_SCRIPT;
	func->name       = name;
	func->id         = id;
	func->returnType = returnType;
	func->scriptSectionIdx = sectionIdx;
	for( int n = 0; n < paramCount; n++ )
	{
		func->parameterTypes.PushLast(params[n]);
		func->inOutFlags.PushLast(inOutFlags[n]);
	}
	func->objectType = objType;
	func->isReadOnly = isConstMethod;

	scriptFunctions.PushLast(func);
	engine->SetScriptFunction(func);

	// Compute the signature id
	if( objType )
		func->ComputeSignatureId();

	if( isGlobalFunction )
		globalFunctions.PushLast(func);

	return 0;
}

// internal 
int asCModule::AddScriptFunction(asCScriptFunction *func)
{
	scriptFunctions.PushLast(func);
	engine->SetScriptFunction(func);

	return 0;
}




// internal
int asCModule::AddImportedFunction(int id, const char *name, const asCDataType &returnType, asCDataType *params, asETypeModifiers *inOutFlags, int paramCount, int moduleNameStringID)
{
	asASSERT(id >= 0);

	// Store the function information
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine, this);
	func->funcType   = asFUNC_IMPORTED;
	func->name       = name;
	func->id         = id;
	func->returnType = returnType;
	for( int n = 0; n < paramCount; n++ )
	{
		func->parameterTypes.PushLast(params[n]);
		func->inOutFlags.PushLast(inOutFlags[n]);
	}
	func->objectType = 0;

	importedFunctions.PushLast(func);

	sBindInfo info;
	info.importedFunction = -1;
	info.importFrom = moduleNameStringID;
	bindInformations.PushLast(info);

	return 0;
}

// internal
// TODO: This can probably be removed
asCScriptFunction *asCModule::GetScriptFunction(int funcID)
{
	return engine->scriptFunctions[funcID & 0xFFFF];
}

// internal
asCScriptFunction *asCModule::GetImportedFunction(int funcID)
{
	return importedFunctions[funcID & 0xFFFF];
}

// internal
asCScriptFunction *asCModule::GetSpecialFunction(int funcID)
{
	if( funcID & FUNC_IMPORTED )
		return importedFunctions[funcID & 0xFFFF];
	else
	{
		if( (funcID & 0xFFFF) == asFUNC_INIT )
			return initFunction;
		else if( (funcID & 0xFFFF) == asFUNC_STRING )
		{
			asASSERT(false);
		}

		return engine->scriptFunctions[funcID & 0xFFFF];
	}
}

// internal
int asCModule::AddContextRef()
{
	return contextCount.atomicInc();
}

// internal
int asCModule::ReleaseContextRef()
{
	return contextCount.atomicDec();
}

// internal
int asCModule::AddModuleRef()
{
	return moduleCount.atomicInc();
}

// internal
int asCModule::ReleaseModuleRef()
{
	return moduleCount.atomicDec();
}

// internal
bool asCModule::CanDelete()
{
	// Don't delete if not discarded
	if( !isDiscarded ) return false;

	// Are there any contexts still referencing the module?
	if( contextCount.get() ) return false;

	// If there are modules referencing this one we need to check for circular referencing
	if( moduleCount.get() )
	{
		// Check if all the modules are without external reference
		asCArray<asCModule*> modules;
		if( CanDeleteAllReferences(modules) )
		{
			// Unbind all functions. This will break any circular references
			for( size_t n = 0; n < bindInformations.GetLength(); n++ )
			{
				int oldFuncID = bindInformations[n].importedFunction;
				if( oldFuncID != -1 )
				{
					asCModule *oldModule = engine->GetModuleFromFuncId(oldFuncID);
					if( oldModule != 0 ) 
					{
						// Release reference to the module
						oldModule->ReleaseModuleRef();
					}
				}
			}
		}

		// Can't delete the module yet because the  
		// other modules haven't released this one
		return false;
	}

	return true;
}

// internal
bool asCModule::CanDeleteAllReferences(asCArray<asCModule*> &modules)
{
	if( !isDiscarded ) return false;

	if( contextCount.get() ) return false;

	modules.PushLast(this);

	// Check all bound functions for referenced modules
	for( size_t n = 0; n < bindInformations.GetLength(); n++ )
	{
		int funcID = bindInformations[n].importedFunction;
		asCModule *module = engine->GetModuleFromFuncId(funcID);

		// If the module is already in the list don't check it again
		bool inList = false;
		for( size_t m = 0; m < modules.GetLength(); m++ )
		{
			if( modules[m] == module )
			{
				inList = true;
				break;
			}
		}

		if( !inList )
		{
			bool ret = module->CanDeleteAllReferences(modules);
			if( ret == false ) return false;
		}
	}

	// If no module has external references then all can be deleted
	return true;
}

// interface
int asCModule::BindImportedFunction(int index, int sourceID)
{
	// First unbind the old function
	int r = UnbindImportedFunction(index);
	if( r < 0 ) return r;

	// Must verify that the interfaces are equal
	asCModule *srcModule = engine->GetModuleFromFuncId(sourceID);
	if( srcModule == 0 ) return asNO_MODULE;

	asCScriptFunction *dst = GetImportedFunction(index);
	if( dst == 0 ) return asNO_FUNCTION;

	asCScriptFunction *src = srcModule->GetScriptFunction(sourceID);
	if( src == 0 ) return asNO_FUNCTION;

	// Verify return type
	if( dst->returnType != src->returnType )
		return asINVALID_INTERFACE;

	if( dst->parameterTypes.GetLength() != src->parameterTypes.GetLength() )
		return asINVALID_INTERFACE;

	for( size_t n = 0; n < dst->parameterTypes.GetLength(); ++n )
	{
		if( dst->parameterTypes[n] != src->parameterTypes[n] )
			return asINVALID_INTERFACE;
	}

	// Add reference to new module
	srcModule->AddModuleRef();

	bindInformations[index].importedFunction = sourceID;

	return asSUCCESS;
}

// interface
int asCModule::UnbindImportedFunction(int index)
{
	if( index < 0 || index > (int)bindInformations.GetLength() )
		return asINVALID_ARG;

	// Remove reference to old module
	int oldFuncID = bindInformations[index].importedFunction;
	if( oldFuncID != -1 )
	{
		asCModule *oldModule = engine->GetModuleFromFuncId(oldFuncID);
		if( oldModule != 0 ) 
		{
			// Release reference to the module
			oldModule->ReleaseModuleRef();
		}
	}

	bindInformations[index].importedFunction = -1;
	return asSUCCESS;
}

// interface
const char *asCModule::GetImportedFunctionDeclaration(int index)
{
	asCScriptFunction *func = GetImportedFunction(index);
	if( func == 0 ) return 0;

	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = func->GetDeclarationStr();

	return tempString->AddressOf();
}

// interface
const char *asCModule::GetImportedFunctionSourceModule(int index)
{
	if( index >= (int)bindInformations.GetLength() )
		return 0;

	index = bindInformations[index].importFrom;

	return stringConstants[index]->AddressOf();
}

// inteface
int asCModule::BindAllImportedFunctions()
{
	bool notAllFunctionsWereBound = false;

	// Bind imported functions
	int c = GetImportedFunctionCount();
	for( int n = 0; n < c; ++n )
	{
		asCScriptFunction *func = GetImportedFunction(n);
		if( func == 0 ) return asERROR;

		asCString str = func->GetDeclarationStr();

		// Get module name from where the function should be imported
		const char *moduleName = GetImportedFunctionSourceModule(n);
		if( moduleName == 0 ) return asERROR;

		asCModule *srcMod = engine->GetModule(moduleName, false);
		int funcId = -1;
		if( srcMod )
			funcId = srcMod->GetFunctionIdByDecl(str.AddressOf());

		if( funcId < 0 )
			notAllFunctionsWereBound = true;
		else
		{
			if( BindImportedFunction(n, funcId) < 0 )
				notAllFunctionsWereBound = true;
		}
	}

	if( notAllFunctionsWereBound )
		return asCANT_BIND_ALL_FUNCTIONS;

	return asSUCCESS;
}

// interface
int asCModule::UnbindAllImportedFunctions()
{
	int c = GetImportedFunctionCount();
	for( int n = 0; n < c; ++n )
		UnbindImportedFunction(n);

	return asSUCCESS;
}

// internal
bool asCModule::IsUsed()
{
	if( contextCount.get() ) return true;
	if( moduleCount.get() ) return true;

	return false;
}

// internal
asCObjectType *asCModule::GetObjectType(const char *type)
{
	size_t n;

	// TODO: optimize: Improve linear search
	for( n = 0; n < classTypes.GetLength(); n++ )
		if( classTypes[n]->name == type )
			return classTypes[n];

	for( n = 0; n < enumTypes.GetLength(); n++ )
		if( enumTypes[n]->name == type )
			return enumTypes[n];

	for( n = 0; n < typeDefs.GetLength(); n++ )
		if( typeDefs[n]->name == type )
			return typeDefs[n];

	return 0;
}

// internal
asCGlobalProperty *asCModule::AllocateGlobalProperty(const char *name, const asCDataType &dt)
{
	asCGlobalProperty *prop = asNEW(asCGlobalProperty);
	prop->index      = (int)scriptGlobals.GetLength();
	prop->name       = name;
	prop->type       = dt;
	scriptGlobals.PushLast(prop);

	// Allocate the memory for this property
	if( dt.GetSizeOnStackDWords() > 2 )
	{
		prop->SetAddressOfValue(asNEWARRAY(asDWORD, dt.GetSizeOnStackDWords()));
		prop->memoryAllocated = true;
	}

	return prop;
}

// internal
int asCModule::GetGlobalVarIndex(int propIdx)
{
	void *ptr = 0;
	if( propIdx < 0 )
		ptr = engine->globalPropAddresses[-int(propIdx) - 1];
	else
		// TODO: We can probably remove the 0xFFFF
		ptr = scriptGlobals[propIdx & 0xFFFF]->GetAddressOfValue();

	for( int n = 0; n < (signed)globalVarPointers.GetLength(); n++ )
		if( globalVarPointers[n] == ptr )
			return n;

	globalVarPointers.PushLast(ptr);
	return (int)globalVarPointers.GetLength()-1;
}

// internal
void asCModule::ResolveInterfaceIds()
{
	// For each of the interfaces declared in the script find identical interface in the engine.
	// If an identical interface was found then substitute the current id for the identical interface's id, 
	// then remove this interface declaration. If an interface was modified by the declaration, then 
	// retry the detection of identical interface for it since it may now match another.

	// For an interface to be equal to another the name and methods must match. If the interface
	// references another interface, then that must be checked as well, which can lead to circular references.

	// Example:
	//
	// interface A { void f(B@); }
	// interface B { void f(A@); void f(C@); }
	// interface C { void f(A@); }
	//
	// A1 equals A2 if and only if B1 equals B2
	// B1 equals B2 if and only if A1 equals A2 and C1 equals C2
	// C1 equals C2 if and only if A1 equals A2


	unsigned int i;

	// The interface can only be equal to interfaces declared in other modules. 
	// Interfaces registered by the application will conflict with this one if it has the same name.
	// This means that we only need to look for the interfaces in the engine->classTypes, but not in engine->objectTypes.
	asCArray<sObjectTypePair> equals;
	for( i = 0; i < classTypes.GetLength(); i++ )
	{
		asCObjectType *intf1 = classTypes[i];
		if( !intf1->IsInterface() )
			continue;

		// The interface may have been determined to be equal to another already
		bool found = false;
		for( unsigned int e = 0; e < equals.GetLength(); e++ )
		{
			if( equals[e].a == intf1 )
			{
				found = true;
				break;
			}
		}
		
		if( found )
			break;

		for( unsigned int n = 0; n < engine->classTypes.GetLength(); n++ )
		{
			// Don't compare against self
			if( engine->classTypes[n] == intf1 )
				continue;
	
			asCObjectType *intf2 = engine->classTypes[n];

			// Assume the interface are equal, then validate this
			sObjectTypePair pair = {intf1,intf2};
			equals.PushLast(pair);

			if( AreInterfacesEqual(intf1, intf2, equals) )
				break;
			
			// Since they are not equal, remove them from the list again
			equals.PopLast();
		}
	}

	// For each of the interfaces that have been found to be equal we need to 
	// remove the new declaration and instead have the module use the existing one.
	for( i = 0; i < equals.GetLength(); i++ )
	{
		// Substitute the old object type from the module's class types
		unsigned int c;
		for( c = 0; c < classTypes.GetLength(); c++ )
		{
			if( classTypes[c] == equals[i].a )
			{
				classTypes[c] = equals[i].b;
				equals[i].b->AddRef();
				break;
			}
		}

		// Remove the old object type from the engine's class types
		for( c = 0; c < engine->classTypes.GetLength(); c++ )
		{
			if( engine->classTypes[c] == equals[i].a )
			{
				engine->classTypes[c] = engine->classTypes[engine->classTypes.GetLength()-1];
				engine->classTypes.PopLast();
				break;
			}
		}

		// Substitute all uses of this object type
		// Only interfaces in the module is using the type so far
		for( c = 0; c < classTypes.GetLength(); c++ )
		{
			if( classTypes[c]->IsInterface() )
			{
				asCObjectType *intf = classTypes[c];
				for( int m = 0; m < intf->GetMethodCount(); m++ )
				{
					asCScriptFunction *func = engine->GetScriptFunction(intf->methods[m]);
					if( func )
					{
						if( func->returnType.GetObjectType() == equals[i].a )
							func->returnType.SetObjectType(equals[i].b);

						for( int p = 0; p < func->GetParamCount(); p++ )
						{
							if( func->parameterTypes[p].GetObjectType() == equals[i].a )
								func->parameterTypes[p].SetObjectType(equals[i].b);
						}
					}
				}
			}
		}

		// Substitute all interface methods in the module. Delete all methods for the old interface
		for( unsigned int m = 0; m < equals[i].a->methods.GetLength(); m++ )
		{
			for( c = 0; c < scriptFunctions.GetLength(); c++ )
			{
				if( scriptFunctions[c]->id == equals[i].a->methods[m] )
				{
					engine->DeleteScriptFunction(scriptFunctions[c]->id);
					scriptFunctions[c] = engine->GetScriptFunction(equals[i].b->methods[m]);
				}
			}
		}

		// Deallocate the object type
		asDELETE(equals[i].a, asCObjectType);
	}
}

// internal
bool asCModule::AreInterfacesEqual(asCObjectType *a, asCObjectType *b, asCArray<sObjectTypePair> &equals)
{
	// An interface is considered equal to another if the following criterias apply:
	//
	// - The interface names are equal
	// - The number of methods is equal
	// - All the methods are equal
	// - The order of the methods is equal
	// - If a method returns or takes an interface by handle or reference, both interfaces must be equal


	// ------------
	// TODO: Study the possiblity of allowing interfaces where methods are declared in different orders to
	// be considered equal. The compiler and VM can handle this, but it complicates the comparison of interfaces
	// where multiple methods take different interfaces as parameters (or return values). Example:
	// 
	// interface A
	// {
	//    void f(B, C);
	//    void f(B);
	//    void f(C);
	// }
	//
	// If 'void f(B)' in module A is compared against 'void f(C)' in module B, then the code will assume
	// interface B in module A equals interface C in module B. Thus 'void f(B, C)' in module A won't match
	// 'void f(C, B)' in module B.
	// ------------


	// Are both interfaces?
	if( !a->IsInterface() || !b->IsInterface() )
		return false;

	// Are the names equal?
	if( a->name != b->name )
		return false;

	// Are the number of methods equal?
	if( a->methods.GetLength() != b->methods.GetLength() )
		return false;

	// Keep the number of equals in the list so we can restore it later if necessary
	int prevEquals = (int)equals.GetLength();

	// Are the methods equal to each other?
	bool match = true;
	for( unsigned int n = 0; n < a->methods.GetLength(); n++ )
	{
		match = false;

		asCScriptFunction *funcA = (asCScriptFunction*)engine->GetFunctionDescriptorById(a->methods[n]);
		asCScriptFunction *funcB = (asCScriptFunction*)engine->GetFunctionDescriptorById(b->methods[n]);

		// funcB can be null if the module that created the interface has been  
		// discarded but the type has not yet been released by the engine.
		if( funcB == 0 )
			break;

		// The methods must have the same name and the same number of parameters
		if( funcA->name != funcB->name ||
			funcA->parameterTypes.GetLength() != funcB->parameterTypes.GetLength() )
			break;
		
		// The return types must be equal. If the return type is an interface the interfaces must match.
		if( !AreTypesEqual(funcA->returnType, funcB->returnType, equals) )
			break;

		match = true;
		for( unsigned int p = 0; p < funcA->parameterTypes.GetLength(); p++ )
		{
			if( !AreTypesEqual(funcA->parameterTypes[p], funcB->parameterTypes[p], equals) ||
				funcA->inOutFlags[p] != funcB->inOutFlags[p] )
			{
				match = false;
				break;
			}
		}

		if( !match )
			break;
	}

	// For each of the new interfaces that we're assuming to be equal, we need to validate this
	if( match )
	{
		for( unsigned int n = prevEquals; n < equals.GetLength(); n++ )
		{
			if( !AreInterfacesEqual(equals[n].a, equals[n].b, equals) )
			{
				match = false;
				break;
			}
		}
	}

	if( !match )
	{
		// The interfaces doesn't match. 
		// Restore the list of previous equals before we go on, so  
		// the caller can continue comparing with another interface
		equals.SetLength(prevEquals);
	}

	return match;
}

// internal
bool asCModule::AreTypesEqual(const asCDataType &a, const asCDataType &b, asCArray<sObjectTypePair> &equals)
{
	if( !a.IsEqualExceptInterfaceType(b) )
		return false;

	asCObjectType *ai = a.GetObjectType();
	asCObjectType *bi = b.GetObjectType();

	if( ai && ai->IsInterface() )
	{
		// If the interface is in the equals list, then the pair must match the pair in the list
		bool found = false;
		unsigned int e;
		for( e = 0; e < equals.GetLength(); e++ )
		{
			if( equals[e].a == ai )
			{
				found = true;
				break;
			}
		}

		if( found )
		{
			// Do the pairs match?
			if( equals[e].b != bi )
				return false;
		}
		else
		{
			// Assume they are equal from now on
			sObjectTypePair pair = {ai, bi};
			equals.PushLast(pair);
		}
	}

	return true;
}

// interface
int asCModule::SaveByteCode(asIBinaryStream *out)
{
	if( out == 0 ) return asINVALID_ARG;

	// TODO: Shouldn't allow saving if the build wasn't successful

	asCRestore rest(this, out, engine);
	return rest.Save();
}

// interface
int asCModule::LoadByteCode(asIBinaryStream *in)
{
	if( in == 0 ) return asINVALID_ARG;

	// Is the module currently in use?
	if( IsUsed() )
		return asMODULE_IS_IN_USE;

	// Only permit loading bytecode if no other thread is currently compiling
	int r = engine->RequestBuild();
	if( r < 0 )
		return r;

	asCRestore rest(this, in, engine);
	r = rest.Restore();

	engine->BuildCompleted();

	return r;
}

// internal
asCConfigGroup *asCModule::GetConfigGroupByGlobalVarId(int gvarId)
{
	void *gvarPtr = globalVarPointers[gvarId];

	gvarId = 0;
	for( asUINT g = 0; g < engine->globalPropAddresses.GetLength(); g++ )
	{
		if( engine->globalPropAddresses[g] == gvarPtr )
		{
			gvarId = -int(g) - 1;
			break;
		}
	}

	if( gvarId < 0 )
	{
		// Find the config group from the property id
		return engine->FindConfigGroupForGlobalVar(gvarId);
	}

	return 0;
}


END_AS_NAMESPACE

