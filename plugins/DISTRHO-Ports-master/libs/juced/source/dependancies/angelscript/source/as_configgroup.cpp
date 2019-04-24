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
// as_configgroup.cpp
//
// This class holds configuration groups for the engine
//



#include "as_config.h"
#include "as_configgroup.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

asCConfigGroup::asCConfigGroup()
{
	refCount = 0;
	defaultAccess = true;
}

asCConfigGroup::~asCConfigGroup()
{
}

int asCConfigGroup::AddRef()
{
	refCount++;
	return refCount;
}

int asCConfigGroup::Release()
{
	// Don't delete the object here, the engine will delete the object when ready
	refCount--;
	return refCount;
}

asCObjectType *asCConfigGroup::FindType(const char *obj)
{
	for( asUINT n = 0; n < objTypes.GetLength(); n++ )
		if( objTypes[n]->name == obj )
			return objTypes[n];

	return 0;
}

void asCConfigGroup::RefConfigGroup(asCConfigGroup *group)
{
	if( group == this || group == 0 ) return;

	// Verify if the group is already referenced
	for( asUINT n = 0; n < referencedConfigGroups.GetLength(); n++ )
		if( referencedConfigGroups[n] == group )
			return;

	referencedConfigGroups.PushLast(group);
	group->AddRef();
}

bool asCConfigGroup::HasLiveObjects(asCScriptEngine * /*engine*/)
{
	for( asUINT n = 0; n < objTypes.GetLength(); n++ )
		if( objTypes[n]->GetRefCount() != 0 )
			return true;

	return false;
}

void asCConfigGroup::RemoveConfiguration(asCScriptEngine *engine)
{
	asASSERT( refCount == 0 );

	asUINT n;

	// Remove global variables
	for( n = 0; n < globalProps.GetLength(); n++ )
	{
		int index = engine->registeredGlobalProps.IndexOf(globalProps[n]);
		if( index >= 0 )
		{
			asDELETE(engine->registeredGlobalProps[index],asCGlobalProperty);
			engine->registeredGlobalProps[index] = 0;
		}
	}

	// Remove global behaviours
	for( n = 0; n < globalBehaviours.GetLength(); n++ )
	{
		int id = engine->globalBehaviours.operators[globalBehaviours[n]+1];
		engine->globalBehaviours.operators[globalBehaviours[n]] = 0;
		engine->globalBehaviours.operators[globalBehaviours[n]+1] = 0;
	
		// Remove the system function as well
		engine->DeleteScriptFunction(id);
	}

	// Remove global functions
	for( n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		engine->DeleteScriptFunction(scriptFunctions[n]->id);
	}

	// Remove behaviours and members of object types
	for( n = 0; n < objTypes.GetLength(); n++ )
	{
		asUINT m;
		asCObjectType *obj = objTypes[n];

		// Don't remove behaviours for interface types as they are built-in
		if( !(obj->flags & asOBJ_SCRIPT_OBJECT) )
		{
			for( m = 0; m < obj->beh.factories.GetLength(); m++ )
			{
				engine->DeleteScriptFunction(obj->beh.factories[m]);
			}

			for( m = 0; m < obj->beh.constructors.GetLength(); m++ )
			{
				engine->DeleteScriptFunction(obj->beh.constructors[m]);
			}

			for( m = 1; m < obj->beh.operators.GetLength(); m += 2 )
			{
				engine->DeleteScriptFunction(obj->beh.operators[m]);
			}

			engine->DeleteScriptFunction(obj->beh.addref);
			engine->DeleteScriptFunction(obj->beh.release);
			engine->DeleteScriptFunction(obj->beh.addref);
			engine->DeleteScriptFunction(obj->beh.gcGetRefCount);
			engine->DeleteScriptFunction(obj->beh.gcSetFlag);
			engine->DeleteScriptFunction(obj->beh.gcGetFlag);
			engine->DeleteScriptFunction(obj->beh.gcEnumReferences);
			engine->DeleteScriptFunction(obj->beh.gcReleaseAllReferences);
		}

		for( m = 0; m < obj->methods.GetLength(); m++ )
		{
			engine->DeleteScriptFunction(obj->methods[m]);
		}
	}


	// Remove object types
	for( n = 0; n < objTypes.GetLength(); n++ )
	{
		asCObjectType *t = objTypes[n];
		int idx = engine->objectTypes.IndexOf(t);
		if( idx >= 0 )
		{
#ifdef AS_DEBUG
			ValidateNoUsage(engine, t);
#endif

			engine->objectTypes.RemoveIndex(idx);

			if( t->flags & asOBJ_TYPEDEF )
				engine->registeredTypeDefs.RemoveValue(t);
			else if( t->flags & asOBJ_ENUM )
				engine->registeredEnums.RemoveValue(t);
			else
				engine->registeredObjTypes.RemoveValue(t);

			asDELETE(t, asCObjectType);
		}
	}

	// Release other config groups
	for( n = 0; n < referencedConfigGroups.GetLength(); n++ )
		referencedConfigGroups[n]->refCount--;
	referencedConfigGroups.SetLength(0);
}

#ifdef AS_DEBUG
void asCConfigGroup::ValidateNoUsage(asCScriptEngine *engine, asCObjectType *type)
{
	for( asUINT n = 0; n < engine->scriptFunctions.GetLength(); n++ )
	{
		asCScriptFunction *func = engine->scriptFunctions[n];
		if( func == 0 ) continue;

		asASSERT(func->returnType.GetObjectType() != type);

		for( asUINT p = 0; p < func->parameterTypes.GetLength(); p++ )
		{
			asASSERT(func->parameterTypes[p].GetObjectType() != type);
		}
	}

	// TODO: Check also usage of the type in global variables 

	// TODO: Check also usage of the type in local variables in script functions

	// TODO: Check also usage of the type as members of classes

	// TODO: Check also usage of the type as sub types in other types
}
#endif

int asCConfigGroup::SetModuleAccess(const char *module, bool hasAccess)
{
	if( module == asALL_MODULES )
	{
		// Set default module access
		defaultAccess = hasAccess;
	}
	else
	{
		asCString mod(module ? module : "");
		asSMapNode<asCString,bool> *cursor = 0;
		if( moduleAccess.MoveTo(&cursor, mod) )
		{
			moduleAccess.GetValue(cursor) = hasAccess;
		}
		else
		{
			moduleAccess.Insert(mod, hasAccess);
		}
	}

	return 0;
}

bool asCConfigGroup::HasModuleAccess(const char *module)
{
	asCString mod(module ? module : "");
	asSMapNode<asCString,bool> *cursor = 0;
	if( moduleAccess.MoveTo(&cursor, mod) )
		return moduleAccess.GetValue(cursor);
	
	return defaultAccess;
}

END_AS_NAMESPACE
