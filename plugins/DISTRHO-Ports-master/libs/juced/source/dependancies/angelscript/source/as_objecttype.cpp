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
// as_objecttype.cpp
//
// A class for storing object type information
//

// TODO: Need a public GetTypeId() for the asIObjectType interface


#include <stdio.h>

#include "as_config.h"
#include "as_objecttype.h"
#include "as_configgroup.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

asCObjectType::asCObjectType()
{
	engine      = 0; 
	refCount.set(0); 
	derivedFrom = 0;

	acceptValueSubType = true;
	acceptRefSubType = true;
}

asCObjectType::asCObjectType(asCScriptEngine *engine) 
{
	this->engine = engine; 
	refCount.set(0); 
	derivedFrom  = 0;

	acceptValueSubType = true;
	acceptRefSubType = true;
}

void asCObjectType::AddRef()
{
	refCount.atomicInc();
}

void asCObjectType::Release()
{
	refCount.atomicDec();
}

int asCObjectType::GetRefCount()
{
	return refCount.get();
}

asCObjectType::~asCObjectType()
{
	// Release the object type held by the templateSubType
	if( templateSubType.GetObjectType() )
		templateSubType.GetObjectType()->Release();

	if( derivedFrom )
		derivedFrom->Release();

	asUINT n;
	for( n = 0; n < properties.GetLength(); n++ )
		if( properties[n] ) 
		{
			if( flags & asOBJ_SCRIPT_OBJECT )
			{
				// Release the config group for script classes that are being destroyed
				asCConfigGroup *group = engine->FindConfigGroupForObjectType(properties[n]->type.GetObjectType());
				if( group != 0 ) group->Release();
			}

			asDELETE(properties[n],asCObjectProperty);
		}

	properties.SetLength(0);

	methods.SetLength(0);

	for( n = 0; n < enumValues.GetLength(); n++ )
	{
		if( enumValues[n] )
			asDELETE(enumValues[n],asSEnumValue);
	}

	enumValues.SetLength(0);
}

bool asCObjectType::Implements(const asCObjectType *objType) const
{
	if( this == objType )
		return true;

	for( asUINT n = 0; n < interfaces.GetLength(); n++ )
		if( interfaces[n] == objType ) return true;

	return false;
}

bool asCObjectType::DerivesFrom(const asCObjectType *objType) const
{
	if( this == objType )
		return true;

	asCObjectType *base = derivedFrom;
	while( base )
	{
		if( base == objType )
			return true;

		base = base->derivedFrom;
	}

	return false;
}

const char *asCObjectType::GetName() const
{
	return name.AddressOf();
}

asDWORD asCObjectType::GetFlags() const
{
	return flags;
}

asUINT asCObjectType::GetSize() const
{
	return size;
}

#ifdef AS_DEPRECATED
// deprecated since 2009-02-26, 2.16.0
asIObjectType *asCObjectType::GetSubType() const
{
	return subType;
}
#endif

int asCObjectType::GetInterfaceCount() const
{
	return (int)interfaces.GetLength();
}

asIObjectType *asCObjectType::GetInterface(asUINT index) const
{
	assert(index < interfaces.GetLength());

	return interfaces[index];
}

// internal
bool asCObjectType::IsInterface() const
{
	if( (flags & asOBJ_SCRIPT_OBJECT) && size == 0 )
		return true;

	return false;
}

asIScriptEngine *asCObjectType::GetEngine() const
{
	return engine;
}

int asCObjectType::GetFactoryCount() const
{
	return (int)beh.factories.GetLength();
}

int asCObjectType::GetFactoryIdByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= beh.factories.GetLength() )
		return asINVALID_ARG;

	return beh.factories[index];
}

int asCObjectType::GetFactoryIdByDecl(const char *decl) const
{
	if( beh.factories.GetLength() == 0 )
		return asNO_FUNCTION;

	// Let the engine parse the string and find the appropriate factory function
	return engine->GetFactoryIdByDecl(this, decl);
}

int asCObjectType::GetMethodCount() const
{
	return (int)methods.GetLength();
}

int asCObjectType::GetMethodIdByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= methods.GetLength() )
		return asINVALID_ARG;

	return methods[index];
}

int asCObjectType::GetMethodIdByName(const char *name) const
{
	int id = -1;
	for( size_t n = 0; n < methods.GetLength(); n++ )
	{
		if( engine->scriptFunctions[methods[n]]->name == name )
		{
			if( id == -1 )
				id = methods[n];
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

int asCObjectType::GetMethodIdByDecl(const char *decl) const
{
	// Get the module from one of the methods
	if( methods.GetLength() == 0 )
		return asNO_FUNCTION;

	asCModule *mod = engine->scriptFunctions[methods[0]]->module;
	if( mod == 0 )
	{
		if( engine->scriptFunctions[methods[0]]->funcType == asFUNC_INTERFACE )
			return engine->GetMethodIdByDecl(this, decl, 0);

		return asNO_MODULE;
	}

	return engine->GetMethodIdByDecl(this, decl, mod);
}

asIScriptFunction *asCObjectType::GetMethodDescriptorByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= methods.GetLength() ) 
		return 0;

	return engine->scriptFunctions[methods[index]];
}

int asCObjectType::GetPropertyCount() const
{
	return (int)properties.GetLength();
}

int asCObjectType::GetPropertyTypeId(asUINT prop) const
{
	if( prop >= properties.GetLength() )
		return asINVALID_ARG;

	return engine->GetTypeIdFromDataType(properties[prop]->type);
}

const char *asCObjectType::GetPropertyName(asUINT prop) const
{
	if( prop >= properties.GetLength() )
		return 0;

	return properties[prop]->name.AddressOf();
}

asIObjectType *asCObjectType::GetBaseType() const
{
	return derivedFrom; 
}

int asCObjectType::GetPropertyOffset(asUINT prop) const
{
	if( prop >= properties.GetLength() )
		return 0;

	return properties[prop]->byteOffset;
}

int asCObjectType::GetBehaviourCount() const
{
	// Count the number of behaviours (except factory functions)
	int count = 0;
	
	if( beh.destruct )               count++;
	if( beh.addref )                 count++;
	if( beh.release )                count++;
	if( beh.gcGetRefCount )          count++;
	if( beh.gcSetFlag )              count++;
	if( beh.gcGetFlag )              count++;
	if( beh.gcEnumReferences )       count++;
	if( beh.gcReleaseAllReferences ) count++; 

	count += (int)beh.constructors.GetLength();
	count += (int)beh.operators.GetLength() / 2;

	return count;
}

int asCObjectType::GetBehaviourByIndex(asUINT index, asEBehaviours *outBehaviour) const
{
	// Find the correct behaviour
	int count = 0;

	if( beh.destruct && count++ == (int)index ) // only increase count if the behaviour is registered
	{ 
		if( outBehaviour ) *outBehaviour = asBEHAVE_DESTRUCT;
		return beh.destruct;
	}

	if( beh.addref && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_ADDREF;
		return beh.addref;
	}

	if( beh.release && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_RELEASE;
		return beh.release;
	}

	if( beh.gcGetRefCount && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_GETREFCOUNT;
		return beh.gcGetRefCount;
	}

	if( beh.gcSetFlag && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_SETGCFLAG;
		return beh.gcSetFlag;
	}

	if( beh.gcGetFlag && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_GETGCFLAG;
		return beh.gcGetFlag;
	}

	if( beh.gcEnumReferences && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_ENUMREFS;
		return beh.gcEnumReferences;
	}

	if( beh.gcReleaseAllReferences && count++ == (int)index )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_RELEASEREFS;
		return beh.gcReleaseAllReferences;
	}

	if( index - count < beh.constructors.GetLength() )
	{
		if( outBehaviour ) *outBehaviour = asBEHAVE_CONSTRUCT;
		return beh.constructors[index - count];
	}
	else 
		count += (int)beh.constructors.GetLength();

	if( index - count < beh.operators.GetLength() / 2 )
	{
		index = 2*(index - count);

		if( outBehaviour ) *outBehaviour = static_cast<asEBehaviours>(beh.operators[index]);
		return beh.operators[index + 1];
	}

	return asINVALID_ARG;
}

// interface
const char *asCObjectType::GetConfigGroup() const
{
	asCConfigGroup *group = engine->FindConfigGroupForObjectType(this);
	if( group == 0 )
		return 0;

	return group->groupName.AddressOf();
}


END_AS_NAMESPACE



