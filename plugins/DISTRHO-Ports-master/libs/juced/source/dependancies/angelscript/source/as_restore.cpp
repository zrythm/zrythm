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
// as_restore.cpp
//
// Functions for saving and restoring module bytecode
// asCRestore was originally written by Dennis Bollyn, dennis@gyrbo.be

#include "as_config.h"
#include "as_restore.h"
#include "as_bytecodedef.h"
#include "as_bytecode.h"
#include "as_arrayobject.h"

BEGIN_AS_NAMESPACE

#define WRITE_NUM(N) stream->Write(&(N), sizeof(N))
#define READ_NUM(N) stream->Read(&(N), sizeof(N))

asCRestore::asCRestore(asCModule* _module, asIBinaryStream* _stream, asCScriptEngine* _engine)
 : module(_module), stream(_stream), engine(_engine)
{
}

int asCRestore::Save() 
{
	unsigned long i, count;

	// Store everything in the same order that the builder parses scripts
	
	// Store type declarations first
	count = (asUINT)module->classTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		// Store only the name of the class/interface types
		WriteObjectTypeDeclaration(module->classTypes[i], false);
	}

	// Now store all interface methods
	for( i = 0; i < count; i++ )
	{
		if( module->classTypes[i]->IsInterface() )
			WriteObjectTypeDeclaration(module->classTypes[i], true);
	}

	// Then store the class methods, properties, and behaviours
	for( i = 0; i < count; ++i )
	{
		if( !module->classTypes[i]->IsInterface() )
			WriteObjectTypeDeclaration(module->classTypes[i], true);
	}

	// Store enums
	count = (asUINT)module->enumTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		WriteObjectTypeDeclaration(module->enumTypes[i], false);
		WriteObjectTypeDeclaration(module->enumTypes[i], true);
	}

	// Store typedefs
	count = (asUINT)module->typeDefs.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		WriteObjectTypeDeclaration(module->typeDefs[i], false);
		WriteObjectTypeDeclaration(module->typeDefs[i], true);
	}

	// scriptGlobals[]
	count = (asUINT)module->scriptGlobals.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteGlobalProperty(module->scriptGlobals[i]);

	// globalVarPointers[]
	WriteGlobalVarPointers();

	// scriptFunctions[]
	count = 0;
	for( i = 0; i < module->scriptFunctions.GetLength(); i++ )
		if( module->scriptFunctions[i]->objectType == 0 )
			count++;
	WRITE_NUM(count);
	for( i = 0; i < module->scriptFunctions.GetLength(); ++i )
		if( module->scriptFunctions[i]->objectType == 0 )
			WriteFunction(module->scriptFunctions[i]);

	// globalFunctions[]
	count = (int)module->globalFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; i++ )
	{
		WriteFunction(module->globalFunctions[i]);
	}

	// initFunction
	count = module->initFunction ? 1 : 0;
	WRITE_NUM(count);
	if( module->initFunction )
		WriteFunction(module->initFunction);

	// stringConstants[]
	count = (asUINT)module->stringConstants.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteString(module->stringConstants[i]);

	// importedFunctions[] and bindInformations[]
	count = (asUINT)module->importedFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteFunction(module->importedFunctions[i]);
		WRITE_NUM(module->bindInformations[i].importFrom);
	}

	// usedTypes[]
	count = (asUINT)usedTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(usedTypes[i]);
	}

	// usedTypeIds[]
	WriteUsedTypeIds();

	// usedFunctions[]
	WriteUsedFunctions();

	// TODO: Store script section names

	return asSUCCESS;
}

int asCRestore::Restore() 
{
	// Before starting the load, make sure that 
	// any existing resources have been freed
	module->InternalReset();

	unsigned long i, count;

	asCScriptFunction* func;
	asCString *cstr;

	// structTypes[]
	// First restore the structure names, then the properties
	READ_NUM(count);
	module->classTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = asNEW(asCObjectType)(engine);
		ReadObjectTypeDeclaration(ot, false);
		engine->classTypes.PushLast(ot);
		module->classTypes.PushLast(ot);
		ot->AddRef();
	}

	// Read interface methods
	for( i = 0; i < module->classTypes.GetLength(); i++ )
	{
		if( module->classTypes[i]->IsInterface() )
			ReadObjectTypeDeclaration(module->classTypes[i], true);
	}

	module->ResolveInterfaceIds();

	// Read class methods, properties, and behaviours
	for( i = 0; i < module->classTypes.GetLength(); ++i )
	{
		if( !module->classTypes[i]->IsInterface() )
			ReadObjectTypeDeclaration(module->classTypes[i], true);
	}

	// Read enums
	READ_NUM(count);
	module->enumTypes.Allocate(count, 0);
	for( i = 0; i < count; i++ )
	{
		asCObjectType *ot = asNEW(asCObjectType)(engine);
		ReadObjectTypeDeclaration(ot, false);
		engine->classTypes.PushLast(ot);
		module->enumTypes.PushLast(ot);
		ot->AddRef();
		ReadObjectTypeDeclaration(ot, true);
	}

	// Read typedefs
	READ_NUM(count);
	module->typeDefs.Allocate(count, 0);
	for( i = 0; i < count; i++ )
	{
		asCObjectType *ot = asNEW(asCObjectType)(engine);
		ReadObjectTypeDeclaration(ot, false);
		engine->classTypes.PushLast(ot);
		module->typeDefs.PushLast(ot);
		ot->AddRef();
		ReadObjectTypeDeclaration(ot, true);
	}

	// scriptGlobals[]
	READ_NUM(count);
	module->scriptGlobals.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		ReadGlobalProperty();
	}

	// globalVarPointers[]
	ReadGlobalVarPointers();

	// scriptFunctions[]
	READ_NUM(count);
	for( i = 0; i < count; ++i ) 
	{
		func = ReadFunction();
	}

	// globalFunctions[]
	READ_NUM(count);
	for( i = 0; i < count; ++i )
	{
		func = ReadFunction(false, false);
		module->globalFunctions.PushLast(func);
	}

	// initFunction
	READ_NUM(count);
	if( count )
	{
		module->initFunction = ReadFunction(false, true);
	}

	// stringConstants[]
	READ_NUM(count);
	module->stringConstants.Allocate(count, 0);
	for(i=0;i<count;++i) 
	{
		cstr = asNEW(asCString)();
		ReadString(cstr);
		module->stringConstants.PushLast(cstr);
	}
	
	// importedFunctions[] and bindInformations[]
	READ_NUM(count);
	module->importedFunctions.Allocate(count, 0);
	module->bindInformations.SetLength(count);
	for(i=0;i<count;++i)
	{
		func = ReadFunction(false, false);
		module->importedFunctions.PushLast(func);

		READ_NUM(module->bindInformations[i].importFrom);
		module->bindInformations[i].importedFunction = -1;
	}
	
	// usedTypes[]
	READ_NUM(count);
	usedTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = ReadObjectType();
		usedTypes.PushLast(ot);
	}

	// usedTypeIds[]
	ReadUsedTypeIds();

	// usedFunctions[]
	ReadUsedFunctions();

	if( module->initFunction ) 
		TranslateFunction(module->initFunction);
	for( i = 0; i < module->scriptFunctions.GetLength(); i++ )
		TranslateFunction(module->scriptFunctions[i]);

	// Fake building
	module->isBuildWithoutErrors = true;

	// Init system functions properly
	engine->PrepareEngine();

	// Add references for all functions
	if( module->initFunction )
		module->initFunction->AddReferences();
	for( i = 0; i < module->scriptFunctions.GetLength(); i++ )
	{
		module->scriptFunctions[i]->AddReferences();
	}

	module->CallInit();

	return 0;
}

void asCRestore::WriteString(asCString* str) 
{
	asUINT len = (asUINT)str->GetLength();
	WRITE_NUM(len);
	stream->Write(str->AddressOf(), (asUINT)len);
}

void asCRestore::WriteUsedFunctions()
{
	asUINT count = (asUINT)usedFunctions.GetLength();
	WRITE_NUM(count);

	for( asUINT n = 0; n < usedFunctions.GetLength(); n++ )
	{
		char c;

		// Write enough data to be able to uniquely identify the function upon load

		// Is the function from the module or the application?
		c = usedFunctions[n]->module ? 'm' : 'a';
		WRITE_NUM(c);

		WriteFunctionSignature(usedFunctions[n]);
	}
}

void asCRestore::ReadUsedFunctions()
{
	asUINT count;
	READ_NUM(count);
	usedFunctions.SetLength(count);

	for( asUINT n = 0; n < usedFunctions.GetLength(); n++ )
	{
		char c;

		// Read the data to be able to uniquely identify the function

		// Is the function from the module or the application?
		READ_NUM(c);

		asCScriptFunction func(engine, c == 'm' ? module : 0);
		ReadFunctionSignature(&func);

		// Find the correct function
		if( c == 'm' )
		{
			for( asUINT i = 0; i < module->scriptFunctions.GetLength(); i++ )
			{
				asCScriptFunction *f = module->scriptFunctions[i];
				if( !func.IsSignatureEqual(f) ||
					func.objectType != f->objectType ||
					func.funcType != f->funcType )
					continue;

				usedFunctions[n] = f;
				break;
			}
		}
		else
		{
			for( asUINT i = 0; i < engine->scriptFunctions.GetLength(); i++ )
			{
				asCScriptFunction *f = engine->scriptFunctions[i];
				if( f == 0 ||
					!func.IsSignatureEqual(f) ||
					func.objectType != f->objectType )
					continue;

				usedFunctions[n] = f;
				break;
			}
		}
	}
}

void asCRestore::WriteFunctionSignature(asCScriptFunction *func)
{
	asUINT i, count;

	WriteString(&func->name);
	WriteDataType(&func->returnType);
	count = (asUINT)func->parameterTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteDataType(&func->parameterTypes[i]);

	count = (asUINT)func->inOutFlags.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
		WRITE_NUM(func->inOutFlags[i]);

	WRITE_NUM(func->funcType);

	WriteObjectType(func->objectType);

	WRITE_NUM(func->isReadOnly);
}

void asCRestore::ReadFunctionSignature(asCScriptFunction *func)
{
	int i, count;
	asCDataType dt;
	int num;

	ReadString(&func->name);
	ReadDataType(&func->returnType);
	READ_NUM(count);
	func->parameterTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		ReadDataType(&dt);
		func->parameterTypes.PushLast(dt);
	}

	READ_NUM(count);
	func->inOutFlags.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		READ_NUM(num);
		func->inOutFlags.PushLast(static_cast<asETypeModifiers>(num));
	}

	READ_NUM(func->funcType);

	func->objectType = ReadObjectType();

	READ_NUM(func->isReadOnly);
}

void asCRestore::WriteFunction(asCScriptFunction* func) 
{
	char c;

	// If there is no function, then store a null char
	if( func == 0 )
	{
		c = '\0';
		WRITE_NUM(c);
		return;
	}

	// First check if the function has been saved already
	for( asUINT f = 0; f < savedFunctions.GetLength(); f++ )
	{
		if( savedFunctions[f] == func )
		{
			c = 'r';
			WRITE_NUM(c);
			WRITE_NUM(f);
			return;
		}
	}

	// Keep a reference to the function in the list
	savedFunctions.PushLast(func);

	c = 'f';
	WRITE_NUM(c);

	asUINT i, count;

	WriteFunctionSignature(func);

	count = (asUINT)func->byteCode.GetLength();
	WRITE_NUM(count);
	WriteByteCode(func->byteCode.AddressOf(), count);

	count = (asUINT)func->objVariablePos.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(func->objVariableTypes[i]);
		WRITE_NUM(func->objVariablePos[i]);
	}

	WRITE_NUM(func->stackNeeded);

	asUINT length = (asUINT)func->lineNumbers.GetLength();
	WRITE_NUM(length);
	for( i = 0; i < length; ++i )
		WRITE_NUM(func->lineNumbers[i]);

	WRITE_NUM(func->vfTableIdx);

	// TODO: Write variables

	// TODO: Store script section index
}

asCScriptFunction *asCRestore::ReadFunction(bool addToModule, bool addToEngine) 
{
	char c;
	READ_NUM(c);

	if( c == '\0' )
	{
		// There is no function, so return a null pointer
		return 0;
	}

	if( c == 'r' )
	{
		// This is a reference to a previously saved function
		int index;
		READ_NUM(index);

		return savedFunctions[index];
	}

	// Load the new function
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine,module);
	savedFunctions.PushLast(func);

	int i, count;
	asCDataType dt;
	int num;

	ReadFunctionSignature(func);

	func->id = engine->GetNextScriptFunctionId();
	

	READ_NUM(count);
	func->byteCode.Allocate(count, 0);
	ReadByteCode(func->byteCode.AddressOf(), count);
	func->byteCode.SetLength(count);

	READ_NUM(count);
	func->objVariablePos.Allocate(count, 0);
	func->objVariableTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		func->objVariableTypes.PushLast(ReadObjectType());
		READ_NUM(num);
		func->objVariablePos.PushLast(num);
	}

	READ_NUM(func->stackNeeded);

	int length;
	READ_NUM(length);
	func->lineNumbers.SetLength(length);
	for( i = 0; i < length; ++i )
		READ_NUM(func->lineNumbers[i]);

	READ_NUM(func->vfTableIdx);

	if( addToModule )
		module->scriptFunctions.PushLast(func);
	if( addToEngine )
		engine->SetScriptFunction(func);
	if( func->objectType )
		func->ComputeSignatureId();

	return func;
}

void asCRestore::WriteGlobalProperty(asCGlobalProperty* prop) 
{
	WriteString(&prop->name);
	WriteDataType(&prop->type);
	WRITE_NUM(prop->index);
}

void asCRestore::WriteObjectProperty(asCObjectProperty* prop) 
{
	WriteString(&prop->name);
	WriteDataType(&prop->type);
	WRITE_NUM(prop->byteOffset);
}

void asCRestore::WriteDataType(const asCDataType *dt) 
{
	if( dt->IsTemplate() )
	{
		bool b = true;
		WRITE_NUM(b);

		b = dt->IsObjectHandle();
		WRITE_NUM(b);
		b = dt->IsReadOnly();
		WRITE_NUM(b);
		b = dt->IsHandleToConst();
		WRITE_NUM(b);
		b = dt->IsReference();
		WRITE_NUM(b);

		asCDataType sub = dt->GetSubType();
		WriteDataType(&sub);
	}
	else
	{
		bool b = false;
		WRITE_NUM(b);

		int t = dt->GetTokenType();
		WRITE_NUM(t);
		WriteObjectType(dt->GetObjectType());
		b = dt->IsObjectHandle();
		WRITE_NUM(b);
		b = dt->IsReadOnly();
		WRITE_NUM(b);
		b = dt->IsHandleToConst();
		WRITE_NUM(b);
		b = dt->IsReference();
		WRITE_NUM(b);
	}
}

void asCRestore::WriteObjectType(asCObjectType* ot) 
{
	char ch;

	// Only write the object type name
	if( ot )
	{
		// Check for template instances/specializations
		if( ot->templateSubType.GetTokenType() != ttUnrecognizedToken &&
			ot != engine->defaultArrayObjectType )
		{
			ch = 'a';
			WRITE_NUM(ch);

			if( ot->templateSubType.IsObject() )
			{
				ch = 's';
				WRITE_NUM(ch);
				WriteObjectType(ot->templateSubType.GetObjectType());

				if( ot->templateSubType.IsObjectHandle() )
					ch = 'h';
				else
					ch = 'o';
				WRITE_NUM(ch);
			}
			else
			{
				ch = 't';
				WRITE_NUM(ch);
				eTokenType t = ot->templateSubType.GetTokenType();
				WRITE_NUM(t);
			}
		}
		else
		{
			ch = 'o';
			WRITE_NUM(ch);
			WriteString(&ot->name);
		}
	}
	else
	{
		ch = '\0';
		WRITE_NUM(ch);
		// Write a null string
		asDWORD null = 0;
		WRITE_NUM(null);
	}
}

void asCRestore::WriteObjectTypeDeclaration(asCObjectType *ot, bool writeProperties)
{
	if( !writeProperties )
	{
		// name
		WriteString(&ot->name);
		// size
		int size = ot->size;
		WRITE_NUM(size);
		// flags
		asDWORD flags = ot->flags;
		WRITE_NUM(flags);
	}
	else
	{	
		if( ot->flags & asOBJ_ENUM )
		{
			// enumValues[]
			int size = (int)ot->enumValues.GetLength();
			WRITE_NUM(size);

			for( int n = 0; n < size; n++ )
			{
				WriteString(&ot->enumValues[n]->name);
				WRITE_NUM(ot->enumValues[n]->value);
			}
		}
		else if( ot->flags & asOBJ_TYPEDEF )
		{
			eTokenType t = ot->templateSubType.GetTokenType();
			WRITE_NUM(t);
		}
		else
		{
			WriteObjectType(ot->derivedFrom);

			// interfaces[]
			int size = (asUINT)ot->interfaces.GetLength();
			WRITE_NUM(size);
			asUINT n;
			for( n = 0; n < ot->interfaces.GetLength(); n++ )
			{
				WriteObjectType(ot->interfaces[n]);
			}

			// properties[]
			size = (asUINT)ot->properties.GetLength();
			WRITE_NUM(size);
			for( n = 0; n < ot->properties.GetLength(); n++ )
			{
				WriteObjectProperty(ot->properties[n]);
			}

			// behaviours
			if( !ot->IsInterface() && ot->flags != asOBJ_TYPEDEF && ot->flags != asOBJ_ENUM )
			{
				WriteFunction(engine->scriptFunctions[ot->beh.construct]);
				WriteFunction(engine->scriptFunctions[ot->beh.destruct]);
				WriteFunction(engine->scriptFunctions[ot->beh.factory]);
				size = (int)ot->beh.constructors.GetLength() - 1;
				WRITE_NUM(size);
				for( n = 1; n < ot->beh.constructors.GetLength(); n++ )
				{
					WriteFunction(engine->scriptFunctions[ot->beh.constructors[n]]);
					WriteFunction(engine->scriptFunctions[ot->beh.factories[n]]);
				}
			}

			// methods[]
			size = (int)ot->methods.GetLength();
			WRITE_NUM(size);
			for( n = 0; n < ot->methods.GetLength(); n++ )
			{
				WriteFunction(engine->scriptFunctions[ot->methods[n]]);
			}

			// virtualFunctionTable[]
			size = (int)ot->virtualFunctionTable.GetLength();
			WRITE_NUM(size);
			for( n = 0; n < (asUINT)size; n++ )
			{
				WriteFunction(ot->virtualFunctionTable[n]);
			}
		}
	}
}

void asCRestore::ReadObjectTypeDeclaration(asCObjectType *ot, bool readProperties)
{
	if( !readProperties )
	{
		// name
		ReadString(&ot->name);
		// size
		int size;
		READ_NUM(size);
		ot->size = size;
		// flags
		asDWORD flags;
		READ_NUM(flags);
		ot->flags = flags;

		// Use the default script class behaviours
		ot->beh = engine->scriptTypeBehaviours.beh;
	}
	else
	{	
		if( ot->flags & asOBJ_ENUM )
		{
			int count;
			READ_NUM(count);
			ot->enumValues.Allocate(count, 0);
			for( int n = 0; n < count; n++ )
			{
				asSEnumValue *e = asNEW(asSEnumValue);
				ReadString(&e->name);
				READ_NUM(e->value);
				ot->enumValues.PushLast(e);
			}
		}
		else if( ot->flags & asOBJ_TYPEDEF )
		{
			eTokenType t;
			READ_NUM(t);
			ot->templateSubType = asCDataType::CreatePrimitive(t, false);
		}
		else
		{
			ot->derivedFrom = ReadObjectType();
			if( ot->derivedFrom )
				ot->derivedFrom->AddRef();

			// interfaces[]
			int size;
			READ_NUM(size);
			ot->interfaces.Allocate(size,0);
			int n;
			for( n = 0; n < size; n++ )
			{
				asCObjectType *intf = ReadObjectType();
				ot->interfaces.PushLast(intf);
			}

			// properties[]
			READ_NUM(size);
			ot->properties.Allocate(size,0);
			for( n = 0; n < size; n++ )
			{
				asCObjectProperty *prop = asNEW(asCObjectProperty);
				ReadObjectProperty(prop);
				ot->properties.PushLast(prop);
			}

			// behaviours
			if( !ot->IsInterface() && ot->flags != asOBJ_TYPEDEF && ot->flags != asOBJ_ENUM )
			{
				asCScriptFunction *func = ReadFunction();
				ot->beh.construct = func->id;
				ot->beh.constructors[0] = func->id;

				func = ReadFunction();
				if( func )
					ot->beh.destruct = func->id;

				func = ReadFunction();
				ot->beh.factory = func->id;
				ot->beh.factories[0] = func->id;

				READ_NUM(size);
				for( n = 0; n < size; n++ )
				{
					asCScriptFunction *func = ReadFunction();
					ot->beh.constructors.PushLast(func->id);

					func = ReadFunction();
					ot->beh.factories.PushLast(func->id);
				}
			}

			// methods[]
			READ_NUM(size);
			for( n = 0; n < size; n++ ) 
			{
				asCScriptFunction *func = ReadFunction();
				ot->methods.PushLast(func->id);
			}

			// virtualFunctionTable[]
			READ_NUM(size);
			for( n = 0; n < size; n++ )
			{
				asCScriptFunction *func = ReadFunction();
				ot->virtualFunctionTable.PushLast(func);
			}
		}
	}
}

void asCRestore::ReadString(asCString* str) 
{
	asUINT len;
	READ_NUM(len);
	str->SetLength(len);
	stream->Read(str->AddressOf(), len);
}

void asCRestore::ReadGlobalProperty() 
{
	asCString name;
	asCDataType type;
	int index;

	ReadString(&name);
	ReadDataType(&type);
	READ_NUM(index);

	module->AllocateGlobalProperty(name.AddressOf(), type);
}

void asCRestore::ReadObjectProperty(asCObjectProperty* prop) 
{
	ReadString(&prop->name);
	ReadDataType(&prop->type);
	READ_NUM(prop->byteOffset);
}

void asCRestore::ReadDataType(asCDataType *dt) 
{
	bool b;
	READ_NUM(b);
	if( b ) 
	{
		bool isObjectHandle;
		READ_NUM(isObjectHandle);
		bool isReadOnly;
		READ_NUM(isReadOnly);
		bool isHandleToConst;
		READ_NUM(isHandleToConst);
		bool isReference;
		READ_NUM(isReference);

		asCDataType sub;
		ReadDataType(&sub);

		*dt = sub;
		dt->MakeArray(engine);
		if( isObjectHandle )
		{
			dt->MakeReadOnly(isHandleToConst);
			dt->MakeHandle(true);
		}
		dt->MakeReadOnly(isReadOnly);
		dt->MakeReference(isReference);
	}
	else
	{
		eTokenType tokenType;
		READ_NUM(tokenType);
		asCObjectType *objType = ReadObjectType();
		bool isObjectHandle;
		READ_NUM(isObjectHandle);
		bool isReadOnly;
		READ_NUM(isReadOnly);
		bool isHandleToConst;
		READ_NUM(isHandleToConst);
		bool isReference;
		READ_NUM(isReference);

		if( tokenType == ttIdentifier )
			*dt = asCDataType::CreateObject(objType, false);
		else
			*dt = asCDataType::CreatePrimitive(tokenType, false);
		if( isObjectHandle )
		{
			dt->MakeReadOnly(isHandleToConst);
			dt->MakeHandle(true);
		}
		dt->MakeReadOnly(isReadOnly);
		dt->MakeReference(isReference);
	}
}

asCObjectType* asCRestore::ReadObjectType() 
{
	asCObjectType *ot;
	char ch;
	READ_NUM(ch);
	if( ch == 'a' )
	{
		READ_NUM(ch);
		if( ch == 's' )
		{
			ot = ReadObjectType();
			asCDataType dt = asCDataType::CreateObject(ot, false);

			READ_NUM(ch);
			if( ch == 'h' )
				dt.MakeHandle(true);

			dt.MakeArray(engine);
			ot = dt.GetObjectType();
			
			asASSERT(ot);
		}
		else
		{
			eTokenType tokenType;
			READ_NUM(tokenType);
			asCDataType dt = asCDataType::CreatePrimitive(tokenType, false);
			dt.MakeArray(engine);
			ot = dt.GetObjectType();
			
			asASSERT(ot);
		}
	}
	else
	{
		// Read the object type name
		asCString typeName;
		ReadString(&typeName);

		if( typeName.GetLength() && typeName != "_builtin_object_" )
		{
			// Find the object type
			ot = module->GetObjectType(typeName.AddressOf());
			if( !ot )
				ot = engine->GetObjectType(typeName.AddressOf());
			
			asASSERT(ot);
		}
		else if( typeName == "_builtin_object_" )
		{
			ot = &engine->scriptTypeBehaviours;
		}
		else
			ot = 0;
	}

	return ot;
}


void asCRestore::WriteByteCode(asDWORD *bc, int length)
{
	while( length )
	{
		asDWORD c = *(asBYTE*)bc;
		WRITE_NUM(*bc);
		bc += 1;
		if( c == BC_ALLOC )
		{
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				tmp[n] = *bc++;

			// Translate the object type 
			asCObjectType *ot = *(asCObjectType**)tmp;
			*(int*)tmp = FindObjectTypeIdx(ot);

			// Translate the constructor func id, if it is a script class
			if( ot->flags & asOBJ_SCRIPT_OBJECT )
				*(int*)&tmp[PTR_SIZE] = FindFunctionIndex(engine->scriptFunctions[*(int*)&tmp[PTR_SIZE]]);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == BC_FREE   ||
			     c == BC_REFCPY || 
				 c == BC_OBJTYPE )
		{
			// Translate object type pointers into indices
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindObjectTypeIdx(*(asCObjectType**)tmp);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == BC_TYPEID )
		{
			// Translate type ids into indices
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindTypeIdIdx(*(int*)tmp);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else if( c == BC_CALL ||
			     c == BC_CALLINTF || 
				 c == BC_CALLSYS )
		{
			// Translate the function id
			asDWORD tmp[MAX_DATA_SIZE];
			int n;
			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				tmp[n] = *bc++;

			*(int*)tmp = FindFunctionIndex(engine->scriptFunctions[*(int*)tmp]);

			for( n = 0; n < asCByteCode::SizeOfType(bcTypes[c])-1; n++ )
				WRITE_NUM(tmp[n]);
		}
		else
		{
			// Store the bc as is
			for( int n = 1; n < asCByteCode::SizeOfType(bcTypes[c]); n++ )
				WRITE_NUM(*bc++);
		}

		length -= asCByteCode::SizeOfType(bcTypes[c]);
	}
}


int asCRestore::FindFunctionIndex(asCScriptFunction *func)
{
	asUINT n;
	for( n = 0; n < usedFunctions.GetLength(); n++ )
	{
		if( usedFunctions[n] == func )
			return n;
	}

	usedFunctions.PushLast(func);
	return (int)usedFunctions.GetLength() - 1;
}

asCScriptFunction *asCRestore::FindFunction(int idx)
{
	return usedFunctions[idx];
}

void asCRestore::WriteUsedTypeIds()
{
	asUINT count = (asUINT)usedTypeIds.GetLength();
	WRITE_NUM(count);
	for( asUINT n = 0; n < count; n++ )
		WriteDataType(engine->GetDataTypeFromTypeId(usedTypeIds[n]));
}

void asCRestore::ReadUsedTypeIds()
{
	asUINT n;
	asUINT count;
	READ_NUM(count);
	usedTypeIds.SetLength(count);
	for( n = 0; n < count; n++ )
	{
		asCDataType dt;
		ReadDataType(&dt);
		usedTypeIds[n] = engine->GetTypeIdFromDataType(dt);
	}
}

void asCRestore::TranslateFunction(asCScriptFunction *func)
{
	asDWORD *bc = func->byteCode.AddressOf();
	for( asUINT n = 0; n < func->byteCode.GetLength(); )
	{
		int c = *(asBYTE*)&bc[n];
		if( c == BC_FREE ||
			c == BC_REFCPY || c == BC_OBJTYPE )
		{
			// Translate the index to the true object type
			asPTRWORD *ot = (asPTRWORD*)&bc[n+1];
			*(asCObjectType**)ot = FindObjectType(*(int*)ot);
		}
		else  if( c == BC_TYPEID )
		{
			// Translate the index to the type id
			int *tid = (int*)&bc[n+1];
			*tid = FindTypeId(*tid);
		}
		else if( c == BC_CALL ||
			     c == BC_CALLINTF ||
				 c == BC_CALLSYS )
		{
			// Translate the index to the func id
			int *fid = (int*)&bc[n+1];
			*fid = FindFunction(*fid)->id;
		}
		else if( c == BC_ALLOC )
		{
			// Translate the index to the true object type
			asPTRWORD *arg = (asPTRWORD*)&bc[n+1];
			*(asCObjectType**)arg = FindObjectType(*(int*)arg);

			// If the object type is a script class then the constructor id must be translated
			asCObjectType *ot = *(asCObjectType**)arg;
			if( ot->flags & asOBJ_SCRIPT_OBJECT )
			{
				int *fid = (int*)&bc[n+1+PTR_SIZE];
				*fid = FindFunction(*fid)->id;
			}
		}

		n += asCByteCode::SizeOfType(bcTypes[c]);
	}
}

int asCRestore::FindTypeIdIdx(int typeId)
{
	asUINT n;
	for( n = 0; n < usedTypeIds.GetLength(); n++ )
	{
		if( usedTypeIds[n] == typeId )
			return n;
	}

	usedTypeIds.PushLast(typeId);
	return (int)usedTypeIds.GetLength() - 1;
}

int asCRestore::FindTypeId(int idx)
{
	return usedTypeIds[idx];
}

int asCRestore::FindObjectTypeIdx(asCObjectType *obj)
{
	asUINT n;
	for( n = 0; n < usedTypes.GetLength(); n++ )
	{
		if( usedTypes[n] == obj )
			return n;
	}

	usedTypes.PushLast(obj);
	return (int)usedTypes.GetLength() - 1;
}

asCObjectType *asCRestore::FindObjectType(int idx)
{
	return usedTypes[idx];
}

void asCRestore::ReadByteCode(asDWORD *bc, int length)
{
	while( length )
	{
		asDWORD c;
		READ_NUM(c);
		*bc = c;
		bc += 1;
		c = *(asBYTE*)&c;

		// Read the bc as is
		for( int n = 1; n < asCByteCode::SizeOfType(bcTypes[c]); n++ )
			READ_NUM(*bc++);

		length -= asCByteCode::SizeOfType(bcTypes[c]);
	}
}

void asCRestore::WriteGlobalVarPointers()
{
	int c = (int)module->globalVarPointers.GetLength();
	WRITE_NUM(c);

	for( int n = 0; n < c; n++ )
	{
		size_t *p = (size_t*)module->globalVarPointers[n];
		
		// First search for the global in the module
		int idx = -1;
		for( int i = 0; i < (signed)module->scriptGlobals.GetLength(); i++ )
		{
			if( p == module->scriptGlobals[i]->GetAddressOfValue() )
			{
				idx = i;
				break;
			}
		}

		// If it is not in the module, it must be an application registered property
		if( idx == -1 )
		{
			idx = 0;
			for( int i = 0; i < (signed)engine->globalPropAddresses.GetLength(); i++ )
			{
				if( engine->globalPropAddresses[i] == p )
				{
					idx = -i - 1;
					break;
				}
			}
			asASSERT( idx != 0 );
		}

		WRITE_NUM(idx);
	}
}

void asCRestore::ReadGlobalVarPointers()
{
	int c;
	READ_NUM(c);

	module->globalVarPointers.SetLength(c);

	for( int n = 0; n < c; n++ )
	{
		int idx;
		READ_NUM(idx);

		if( idx < 0 ) 
			module->globalVarPointers[n] = (void*)(engine->globalPropAddresses[-idx - 1]);
		else
			module->globalVarPointers[n] = (void*)(module->scriptGlobals[idx]->GetAddressOfValue());
	}
}

END_AS_NAMESPACE

