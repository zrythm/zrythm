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
// as_scriptfunction.h
//
// A container for a compiled script function
//



#ifndef AS_SCRIPTFUNCTION_H
#define AS_SCRIPTFUNCTION_H

#include "as_config.h"
#include "as_string.h"
#include "as_array.h"
#include "as_datatype.h"

BEGIN_AS_NAMESPACE

class asCScriptEngine;
class asCModule;

struct asSScriptVariable
{
	asCString name;
	asCDataType type;
	int stackOffset;
};

const int asFUNC_SYSTEM    = 0;
const int asFUNC_SCRIPT    = 1;
const int asFUNC_INTERFACE = 2;
const int asFUNC_IMPORTED  = 3;
const int asFUNC_VIRTUAL   = 4;

struct asSSystemFunctionInterface;

// TODO: Need a method for obtaining the function type, so that the application can differenciate between the types
//       This should replace the IsClassMethod and IsInterfaceMethod

// TODO: Need a method for obtaining the read-only flag for class methods

// TODO: GetModuleName should be exchanged for GetModule and should return asIScriptModule pointer

class asCScriptFunction : public asIScriptFunction
{
public:
	// From asIScriptFunction
	asIScriptEngine     *GetEngine() const;
	const char          *GetModuleName() const;
	asIObjectType       *GetObjectType() const;
	const char          *GetObjectName() const;
	const char          *GetName() const;
	const char          *GetDeclaration(bool includeObjectName = true) const;
	const char          *GetScriptSectionName() const;
	const char          *GetConfigGroup() const;

	bool                 IsClassMethod() const;
	bool                 IsInterfaceMethod() const;

	int                  GetParamCount() const;
	int                  GetParamTypeId(int index, asDWORD *flags = 0) const;
	int                  GetReturnTypeId() const;

public:
	asCScriptFunction(asCScriptEngine *engine, asCModule *mod);
	~asCScriptFunction();

	void      AddVariable(asCString &name, asCDataType &type, int stackOffset);

	int       GetSpaceNeededForArguments();
	int       GetSpaceNeededForReturnValue();
	asCString GetDeclarationStr(bool includeObjectName = true) const;
	int       GetLineNumber(int programPosition);
	void      ComputeSignatureId();
	bool      IsSignatureEqual(const asCScriptFunction *func) const;

	void      AddReferences();
	void      ReleaseReferences();

	asCScriptEngine             *engine;
	asCModule                   *module;

	// Function signature
	asCString                        name;
	asCDataType                      returnType;
	asCArray<asCDataType>            parameterTypes;
	asCArray<asETypeModifiers>       inOutFlags;
	bool                             isReadOnly;
	asCObjectType                   *objectType;
	int                              signatureId;

	int                          id;

	int                          funcType;

	// Used by asFUNC_SCRIPT
	asCArray<asDWORD>            byteCode;
	asCArray<asCObjectType*>     objVariableTypes;
	asCArray<int>	             objVariablePos;
	int                          stackNeeded;
	asCArray<int>                lineNumbers;      // debug info
	asCArray<asSScriptVariable*> variables;        // debug info
	int                          scriptSectionIdx; // debug info
	bool                         dontCleanUpOnException;   // Stub functions don't own the object and parameters

	// Used by asFUNC_VIRTUAL
	int                          vfTableIdx;

	// Used by asFUNC_SYSTEM
	asSSystemFunctionInterface  *sysFuncIntf;
};

END_AS_NAMESPACE

#endif
