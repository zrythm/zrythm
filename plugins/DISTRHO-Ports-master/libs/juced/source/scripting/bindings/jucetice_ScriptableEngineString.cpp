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
  #include "../../dependancies/angelscript/include/angelscript.h"
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

#include "../jucetice_ScriptableEngine.h"

using namespace angelscriptNamespace;

namespace Bindings {

//==============================================================================
asString::asString () {
    refCount = 1;
}
asString::asString (const char s) {
    refCount = 1;
	buffer << s;
}
asString::asString (const char *s) {
    refCount = 1;
	buffer = s;
}
asString::asString (const tchar s) {
    refCount = 1;
	buffer << s;
}
asString::asString (const tchar *s) {
    refCount = 1;
	buffer = s;
}
asString::asString (const String &s) {
    refCount = 1;
	buffer = s;
}
asString::asString (const asString &s) {
    refCount = 1;
	buffer = s.buffer;
}
asString::~asString() {
    jassert (refCount == 0);
}

//==============================================================================
void asString::addRef()
{
	refCount++;
}

void asString::release()
{
	if( --refCount == 0 )
		delete this;
}

//==============================================================================
bool asString::equalsIgnoreCase(const asString &str) {
	return buffer.equalsIgnoreCase(str.buffer);
}
int asString::compare (const asString& other) {
	return buffer.compare(other.buffer);
}
int asString::compareIgnoreCase (const asString& other) {
	return buffer.compareIgnoreCase(other.buffer);
}
int asString::compareLexicographically (const asString& other) {
	return buffer.compareLexicographically(other.buffer);
}
bool asString::startsWith (const asString& text) {
	return buffer.startsWith(text.buffer);
}
bool asString::startsWithIgnoreCase (const asString& text) {
	return buffer.startsWithIgnoreCase(text.buffer);
}
bool asString::endsWith (const asString& text) {
	return buffer.endsWith(text.buffer);
}
bool asString::endsWithIgnoreCase (const asString& text) {
	return buffer.endsWithIgnoreCase(text.buffer);
}
bool asString::contains (const asString& text) {
	return buffer.contains(text.buffer);
}
bool asString::containsIgnoreCase (const asString& text) {
	return buffer.containsIgnoreCase(text.buffer);
}
bool asString::containsWholeWord (const asString& wordToLookFor) {
	return buffer.containsWholeWord(wordToLookFor.buffer);
}
bool asString::containsWholeWordIgnoreCase (const asString& wordToLookFor) {
	return buffer.containsWholeWordIgnoreCase(wordToLookFor.buffer);
}
bool asString::containsAnyOf (const asString& charactersItMightContain) {
	return buffer.containsAnyOf(charactersItMightContain.buffer);
}
bool asString::containsOnly (const asString& charactersItMightContain) {
	return buffer.containsOnly(charactersItMightContain.buffer);
}
bool asString::matchesWildcard (const asString& wildcard, const bool ignoreCase) {
	return buffer.matchesWildcard(wildcard.buffer,ignoreCase);
}
int asString::indexOf (const asString& other) {
	return buffer.indexOf (other.buffer);
}
int asString::indexOf (const int startIndex, const asString& textToLookFor) {
	return buffer.indexOf (startIndex, textToLookFor.buffer);
}
int asString::indexOfIgnoreCase (const asString& textToLookFor) {
	return buffer.indexOfIgnoreCase (textToLookFor.buffer);
}
int asString::indexOfIgnoreCase (const int startIndex, const asString& textToLookFor) {
	return buffer.indexOfIgnoreCase (startIndex, textToLookFor.buffer);
}
int asString::lastIndexOf (const asString& textToLookFor) {
	return buffer.lastIndexOf (textToLookFor.buffer);
}
int asString::lastIndexOfIgnoreCase (const asString& textToLookFor) {
	return buffer.lastIndexOfIgnoreCase (textToLookFor.buffer);
}
asString* asString::substring (int startIndex, int endIndex) {
	return new asString(buffer.substring(startIndex,endIndex));
}
asString* asString::substring (const int startIndex) {
	return new asString(buffer.substring(startIndex));
}
asString* asString::dropLastCharacters (const int numberToDrop) {
	return new asString(buffer.dropLastCharacters(numberToDrop));
}
asString* asString::fromFirstOccurrenceOf (const asString& substringToStartFrom,
                                           const bool includeSubStringInResult,
                                           const bool ignoreCase) {
	return new asString(buffer.fromFirstOccurrenceOf(substringToStartFrom.buffer,includeSubStringInResult,ignoreCase));
}
asString* asString::fromLastOccurrenceOf (const asString& substringToFind,
                                          const bool includeSubStringInResult,
                                          const bool ignoreCase) {
	return new asString(buffer.fromLastOccurrenceOf(substringToFind.buffer,includeSubStringInResult,ignoreCase));
}
asString* asString::upToFirstOccurrenceOf (const asString& substringToEndWith,
                                           const bool includeSubStringInResult,
                                           const bool ignoreCase) {
	return new asString(buffer.upToFirstOccurrenceOf(substringToEndWith.buffer,includeSubStringInResult,ignoreCase));
}
asString* asString::upToLastOccurrenceOf (const asString& substringToFind,
                                          const bool includeSubStringInResult,
                                          const bool ignoreCase) {
	return new asString(buffer.upToLastOccurrenceOf(substringToFind.buffer,includeSubStringInResult,ignoreCase));
}
asString* asString::trim() {
	return new asString(buffer.trim());
}
asString* asString::trimStart() {
	return new asString(buffer.trimStart());
}
asString* asString::trimEnd() {
	return new asString(buffer.trimEnd());
}
asString* asString::toUpperCase() {
	return new asString(buffer.toUpperCase());
}
asString* asString::toLowerCase() {
	return new asString(buffer.toLowerCase());
}
asString* asString::replaceSection (int startIndex, int numCharactersToReplace, const asString& stringToInsert){
	return new asString(buffer.replaceSection(startIndex,numCharactersToReplace,stringToInsert.buffer));
}
asString* asString::replace (const asString& stringToReplace,const asString& stringToInsertInstead,const bool ignoreCase){
	return new asString(buffer.replace(stringToReplace.buffer,stringToInsertInstead.buffer,ignoreCase));
}
asString* asString::replaceCharacter (const asString& characterToReplace, const asString& characterToInsertInstead) {
	return new asString(buffer.replaceCharacter(characterToReplace.buffer[0],characterToInsertInstead.buffer[0]));
}
asString* asString::replaceCharacters (const asString& charactersToReplace, const asString& charactersToInsertInstead){
	return new asString(buffer.replaceCharacters(charactersToReplace.buffer,charactersToInsertInstead.buffer));
}
asString* asString::retainCharacters (const asString& charactersToRetain) {
	return new asString(buffer.retainCharacters(charactersToRetain.buffer));
}
asString* asString::removeCharacters (const asString& charactersToRemove) {
	return new asString(buffer.removeCharacters(charactersToRemove.buffer));
}
asString* asString::initialSectionContainingOnly (const asString& charactersToRetain) {
	return new asString(buffer.initialSectionContainingOnly(charactersToRetain.buffer));
}
asString* asString::initialSectionNotContaining (const asString& charactersNotToRetain) {
	return new asString(buffer.initialSectionNotContaining(charactersNotToRetain.buffer));
}
asString* asString::unquoted () {
	return new asString(buffer.unquoted());
}
asString* asString::quoted (const asString& quoteCharacter) {
	return new asString(buffer.quoted(quoteCharacter.buffer[0]));
}


//==============================================================================
bool operator== (const asString &a, const asString &b) { return a.buffer == b.buffer; }
bool operator!= (const asString &a, const asString &b) { return a.buffer != b.buffer; }
bool operator<= (const asString &a, const asString &b) { return a.buffer <= b.buffer; }
bool operator>= (const asString &a, const asString &b) { return a.buffer >= b.buffer; }
bool operator< (const asString &a, const asString &b) { return a.buffer < b.buffer; }
bool operator> (const asString &a, const asString &b) { return a.buffer > b.buffer; }
bool operator== (const asString &a, const float b) { return a.buffer.getFloatValue() == b; }
bool operator!= (const asString &a, const float b) { return a.buffer.getFloatValue() != b; }
bool operator<= (const asString &a, const float b) { return a.buffer.getFloatValue() <= b; }
bool operator>= (const asString &a, const float b) { return a.buffer.getFloatValue() >= b; }
bool operator< (const asString &a, const float b) { return a.buffer.getFloatValue() < b; }
bool operator> (const asString &a, const float b) { return a.buffer.getFloatValue() > b; }
bool operator== (const asString &a, const int b) { return a.buffer.getIntValue() == b; }
bool operator!= (const asString &a, const int b) { return a.buffer.getIntValue() != b; }
bool operator<= (const asString &a, const int b) { return a.buffer.getIntValue() <= b; }
bool operator>= (const asString &a, const int b) { return a.buffer.getIntValue() >= b; }
bool operator< (const asString &a, const int b) { return a.buffer.getIntValue() < b; }
bool operator> (const asString &a, const int b) { return a.buffer.getIntValue() > b; }


//==============================================================================
asString &asString::operator= (const asString &other) {
	buffer = other.buffer;
	return *this;
}

asString &asString::operator+= (const asString &other) {
	buffer += other.buffer;
	return *this;
}

asString *operator+ (const asString &a, const asString &b) {
	return new asString (a.buffer + b.buffer);
}

//==============================================================================
asString* StringShiftLeftBool (asString* dest, bool b) {
	dest->buffer << ((b==true)?1:0);
	return dest;
}

asString* StringShiftLeftInt (asString* dest, int i) {
	dest->buffer << i;
	return dest;
}

asString* StringShiftLeftUInt (asString* dest, unsigned int ui) {
	dest->buffer << (int)ui;
	return dest;
}

asString* StringShiftLeftFloat (asString* dest, float f) {
	dest->buffer << f;
	return dest;
}

asString* StringShiftLeftDouble (asString* dest, double d) {
	dest->buffer << d;
	return dest;
}

asString* StringShiftLeftString (asString* dest, asString& other) {
	dest->buffer << other.buffer;
	return dest;
}

//==============================================================================
static asString &AssignBoolToString (bool b, asString &dest)
{
	String stream;
	stream << ((b==true)?1:0);
	dest.buffer = stream;
	return dest;
}

static asString &AssignUIntToString (unsigned int i, asString &dest)
{
	String stream;
	stream.printf("%u", i);
	dest.buffer = stream;
	return dest;
}

static asString &AssignIntToString (int i, asString &dest)
{
	String stream;
	stream << i;
	dest.buffer = stream;
	return dest;
}

static asString &AssignFloatToString (float f, asString &dest)
{
	String stream;
	stream << f;
	dest.buffer = stream;
	return dest;
}

static asString &AssignDoubleToString (double f, asString &dest)
{
	String stream;
	stream << f;
	dest.buffer = stream;
	return dest;
}

//==============================================================================
static asString &AddAssignBoolToString (bool b, asString &dest)
{
	String stream;
	stream << ((b==true)?1:0);
	dest.buffer += stream;
	return dest;
}

static asString &AddAssignUIntToString (unsigned int i, asString &dest)
{
	String stream;
	stream.printf("%u", i);
	dest.buffer += stream;
	return dest;
}

static asString &AddAssignIntToString (int i, asString &dest)
{
	String stream;
	stream << i;
	dest.buffer += stream;
	return dest;
}

static asString &AddAssignFloatToString (float f, asString &dest)
{
	String stream;
	stream << f;
	dest.buffer += stream;
	return dest;
}

static asString &AddAssignDoubleToString (double f, asString &dest)
{
	String stream;
	stream << f;
	dest.buffer += stream;
	return dest;
}

//==============================================================================
static asString *AddStringBool (const asString &str,bool b)
{
	String stream;
	stream << (const char*)str.buffer << ((b==true)?1:0);
	return new asString(stream);
}

static asString *AddStringUInt (const asString &str, unsigned int i)
{
	String stream;
	stream.printf("%u", i );
	return new asString(str.buffer + stream);
}

static asString *AddStringInt (const asString &str, int i)
{
	String stream;
	stream << (const char*) str.buffer << i;
	return new asString(stream);
}

static asString *AddStringFloat (const asString &str, float f)
{
	String stream;
	stream << (const char*) str.buffer << f;
	return new asString(stream);
}

static asString *AddStringDouble (const asString &str, double f)
{
	String stream;
	stream << (const char*) str.buffer << f;
	return new asString(stream);
}

//==============================================================================
static asString *AddBoolString (bool b, const asString &str)
{
	String stream;
	stream << ((b==true)?1:0) << (const char*)str.buffer;
	return new asString(stream);
}

static asString *AddUIntString (unsigned int i, const asString &str)
{
	String stream;
	stream.printf("%u", i );
	return new asString(stream + str.buffer);
}

static asString *AddIntString (int i, const asString &str)
{
	String stream;
	stream << i << (const char*)str.buffer;
	return new asString(stream);
}

static asString *AddFloatString (float f, const asString &str)
{
	String stream;
	stream << f << (const char*)str.buffer;
	return new asString(stream);
}

static asString *AddDoubleString (double f, const asString &str)
{
	String stream;
	stream << f << (const char*)str.buffer;
	return new asString(stream);
}

//==============================================================================
static asString* StringCharAt (asUINT i, const asString &str)
{
	if( i >= (unsigned int) str.buffer.length() )
	{
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException("Out of range");

		return 0;
	}

	return new asString (str.buffer.substring(i, i + 1));
}

//==============================================================================
// This is the string factory that creates new strings for the script
static asString* StringFactory (asUINT length, const char* text)
{
	return new asString (text);
}

static asString *StringDefaultFactory()
{
	return new asString();
}

static asString* StringBoolFactory (bool b)
{
	String text;
	text << (b ? 1 : 0);
	return new asString (text);
}

static asString* StringUintFactory (unsigned int i)
{
	String text;
	text.printf ("%u", i);
	return new asString (text);
}

static asString* StringIntFactory (int i)
{ 
	String text;
	text << i;
	return new asString (text);
}

static asString* StringFloatFactory (float f)
{
	String text;
	text << f;
	return new asString (text);
}

static asString* StringDoubleFactory (double d)
{
	String text;
	text << d;
	return new asString (text);
}

static asString *StringCopyFactory (const asString &other)
{
	// Allocate and initialize with the copy constructor
	return new asString (other.buffer);
}


//==============================================================================
// This is where we register the string type
void RegisterObjectTypeString (ScriptableEngine* scriptEngine)
{
	int r;

    asIScriptEngine* engine = (asIScriptEngine*) scriptEngine->getCurrentEngine ();

	// Register the type
	r = engine->RegisterObjectType ("String", sizeof(asString), asOBJ_REF); jassert( r >= 0 );

	// Register the object operator overloads
	// Note: We don't have to register the destructor, since the object uses reference counting
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f()",                 asFUNCTION(StringDefaultFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f(const bool)",       asFUNCTION(StringBoolFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f(const uint)",       asFUNCTION(StringUintFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f(const int)",        asFUNCTION(StringIntFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f(const float)",      asFUNCTION(StringFloatFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f(const double)",     asFUNCTION(StringDoubleFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_FACTORY,    "String @f(const String &in)", asFUNCTION(StringCopyFactory), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADDREF,     "void f()",                    asMETHOD(asString,addRef), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_RELEASE,    "void f()",                    asMETHOD(asString,release), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ASSIGNMENT, "String &f(const String &in)", asMETHODPR(asString, operator =, (const asString&), asString&), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADD_ASSIGN, "String &f(const String &in)", asMETHODPR(asString, operator+=, (const asString&), asString&), asCALL_THISCALL); jassert( r >= 0 );

	// Register the factory to return a handle to a new string
	r = engine->RegisterStringFactory ("String@", asFUNCTION(StringFactory), asCALL_CDECL); jassert( r >= 0 );

#if 0
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f(const String& in)",    asFUNCTION(ConstructStringByString), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f(bool)",                asFUNCTION(ConstructStringByBool), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f(uint)",                asFUNCTION(ConstructStringByUInt), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f(int)",                 asFUNCTION(ConstructStringByInt), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f(float)",               asFUNCTION(ConstructStringByFloat), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_CONSTRUCT,  "void f(double)",              asFUNCTION(ConstructStringByDouble), asCALL_CDECL_OBJLAST); assertPrint (r);
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructString), asCALL_CDECL_OBJLAST); assertPrint (r);
#endif

	// Register the global operator overloads
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(const String &in, const String &in)", asFUNCTIONPR(operator +, (const asString &, const asString &), asString*), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_EQUAL,       "bool f(const String &in, const String &in)",    asFUNCTIONPR(operator==, (const asString &, const asString &), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_NOTEQUAL,    "bool f(const String &in, const String &in)",    asFUNCTIONPR(operator!=, (const asString &, const asString &), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_LEQUAL,      "bool f(const String &in, const String &in)",    asFUNCTIONPR(operator<=, (const asString &, const asString &), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_GEQUAL,      "bool f(const String &in, const String &in)",    asFUNCTIONPR(operator>=, (const asString &, const asString &), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_LESSTHAN,    "bool f(const String &in, const String &in)",    asFUNCTIONPR(operator <, (const asString &, const asString &), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_GREATERTHAN, "bool f(const String &in, const String &in)",    asFUNCTIONPR(operator >, (const asString &, const asString &), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_EQUAL,       "bool f(const String &in, const float)",         asFUNCTIONPR(operator==, (const asString &, const float), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_NOTEQUAL,    "bool f(const String &in, const float)",         asFUNCTIONPR(operator!=, (const asString &, const float), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_LEQUAL,      "bool f(const String &in, const float)",         asFUNCTIONPR(operator<=, (const asString &, const float), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_GEQUAL,      "bool f(const String &in, const float)",         asFUNCTIONPR(operator>=, (const asString &, const float), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_LESSTHAN,    "bool f(const String &in, const float)",         asFUNCTIONPR(operator <, (const asString &, const float), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_GREATERTHAN, "bool f(const String &in, const float)",         asFUNCTIONPR(operator >, (const asString &, const float), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_EQUAL,       "bool f(const String &in, const int)",           asFUNCTIONPR(operator==, (const asString &, const int), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_NOTEQUAL,    "bool f(const String &in, const int)",           asFUNCTIONPR(operator!=, (const asString &, const int), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_LEQUAL,      "bool f(const String &in, const int)",           asFUNCTIONPR(operator<=, (const asString &, const int), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_GEQUAL,      "bool f(const String &in, const int)",           asFUNCTIONPR(operator>=, (const asString &, const int), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_LESSTHAN,    "bool f(const String &in, const int)",           asFUNCTIONPR(operator <, (const asString &, const int), bool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_GREATERTHAN, "bool f(const String &in, const int)",           asFUNCTIONPR(operator >, (const asString &, const int), bool), asCALL_CDECL); jassert( r >= 0 );

	// Register the index operator
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_INDEX, "String@ f(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
//	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_INDEX, "uint8 &f(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
//	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_INDEX, "const uint8 &f(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); jassert( r >= 0 );

	// Register the object methods
	r = engine->RegisterObjectMethod ("String", "int length() const",										asMETHOD(String,length), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int hashCode() const",										asMETHOD(String,hashCode), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool isEmpty() const",										asMETHOD(String,isEmpty), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool isNotEmpty() const",									asMETHOD(String,isNotEmpty), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool equalsIgnoreCase(const String& in) const",				asMETHOD(asString, equalsIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int compare(const String& in) const",						asMETHOD(asString, compare), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int compareIgnoreCase(const String& in) const",				asMETHOD(asString, compareIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int compareLexicographically(const String& in) const",		asMETHOD(asString, compareLexicographically), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool startsWith(const String& in) const",					asMETHOD(asString, startsWith), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool startsWithIgnoreCase(const String& in) const",			asMETHOD(asString, startsWithIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool endsWith(const String& in) const",						asMETHOD(asString, endsWith), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool endsWithIgnoreCase(const String& in) const",			asMETHOD(asString, endsWithIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool contains(const String& in) const",						asMETHOD(asString, contains), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool containsIgnoreCase(const String& in) const",			asMETHOD(asString, containsIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool containsWholeWord(const String& in) const",			asMETHOD(asString, containsWholeWord), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool containsWholeWordIgnoreCase(const String& in) const",	asMETHOD(asString, containsWholeWordIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool containsAnyOf(const String& in) const",				asMETHOD(asString, containsAnyOf), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool containsOnly(const String& in) const",					asMETHOD(asString, containsOnly), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool matchesWildcard(const String& in, const bool) const",	asMETHOD(asString, matchesWildcard), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int indexOf(const String& in) const",						asMETHODPR(asString, indexOf, (const asString&), int), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int indexOf(const int, const String& in) const",			asMETHODPR(asString, indexOf, (const int, const asString&), int), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int indexOfIgnoreCase(const String& in) const",				asMETHODPR(asString, indexOfIgnoreCase, (const asString&), int), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int indexOfIgnoreCase(const int, const String& in) const",	asMETHODPR(asString, indexOfIgnoreCase, (const int, const asString&), int), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int lastIndexOf(const String& in) const",					asMETHOD(asString, lastIndexOf), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "int lastIndexOfIgnoreCase(const String& in) const",			asMETHOD(asString, lastIndexOfIgnoreCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ substring(int)",						            asMETHODPR(asString, substring, (int), asString*), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ substring(int,int)",						        asMETHODPR(asString, substring, (int,int), asString*), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ dropLastCharacters(int)",						    asMETHOD(asString, dropLastCharacters), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ fromFirstOccurrenceOf(const String& in,bool,bool)",	asMETHOD(asString, fromFirstOccurrenceOf), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ fromLastOccurrenceOf(const String& in,bool,bool)",	asMETHOD(asString, fromLastOccurrenceOf), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ upToFirstOccurrenceOf(const String& in,bool,bool)",	asMETHOD(asString, upToFirstOccurrenceOf), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ upToLastOccurrenceOf(const String& in,bool,bool)",	asMETHOD(asString, upToLastOccurrenceOf), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ trim()",	                                        asMETHOD(asString, trim), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ trimStart()",	                                    asMETHOD(asString, trimStart), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ trimEnd()",	                                        asMETHOD(asString, trimEnd), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ toUpperCase()",	                                    asMETHOD(asString, toUpperCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ toLowerCase()",	                                    asMETHOD(asString, toLowerCase), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ replaceSection(int,int,const String& in)",          asMETHOD(asString, replaceSection), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ replace(const String& in,const String& in,bool)",   asMETHOD(asString, replace), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ replaceCharacter(const String& in,const String& in)",asMETHOD(asString, replaceCharacter), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ replaceCharacters(const String& in,const String& in)",asMETHOD(asString, replaceCharacters), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ retainCharacters(const String& in)",                asMETHOD(asString, retainCharacters), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ removeCharacters(const String& in)",                asMETHOD(asString, removeCharacters), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ initialSectionContainingOnly(const String& in)",    asMETHOD(asString, initialSectionContainingOnly), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ initialSectionNotContaining(const String& in)",     asMETHOD(asString, initialSectionNotContaining), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "bool isQuotedString() const",		                        asMETHOD(String,isQuotedString), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ unquoted()",	                                    asMETHOD(asString, unquoted), asCALL_THISCALL); jassert( r >= 0 );
	r = engine->RegisterObjectMethod ("String", "String@ quoted(const String& in)",	                        asMETHOD(asString, quoted), asCALL_THISCALL); jassert( r >= 0 );

	// Register global operators
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ASSIGNMENT, "String &f(double)",                 asFUNCTION(AssignDoubleToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADD_ASSIGN, "String &f(double)",                 asFUNCTION(AddAssignDoubleToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(const String &in, double)",        asFUNCTION(AddStringDouble), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(double, const String &in)",        asFUNCTION(AddDoubleString), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ASSIGNMENT, "String &f(float)",                  asFUNCTION(AssignFloatToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADD_ASSIGN, "String &f(float)",                  asFUNCTION(AddAssignFloatToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(const String &in, float)",         asFUNCTION(AddStringFloat), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(float, const String &in)",         asFUNCTION(AddFloatString), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ASSIGNMENT, "String &f(int)",                    asFUNCTION(AssignIntToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADD_ASSIGN, "String &f(int)",                    asFUNCTION(AddAssignIntToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(const String &in, int)",           asFUNCTION(AddStringInt), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(int, const String &in)",           asFUNCTION(AddIntString), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ASSIGNMENT, "String &f(uint)",                   asFUNCTION(AssignUIntToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADD_ASSIGN, "String &f(uint)",                   asFUNCTION(AddAssignUIntToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(const String &in, uint)",          asFUNCTION(AddStringUInt), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(uint, const String &in)",          asFUNCTION(AddUIntString), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ASSIGNMENT, "String &f(bool)",                   asFUNCTION(AssignBoolToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterObjectBehaviour ("String", asBEHAVE_ADD_ASSIGN, "String &f(bool)",                   asFUNCTION(AddAssignBoolToString), asCALL_CDECL_OBJLAST); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(const String &in, bool)",          asFUNCTION(AddStringBool), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_ADD,         "String@ f(bool, const String &in)",          asFUNCTION(AddBoolString), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_BIT_SLL,     "String& f(const String &in, String &in)",    asFUNCTION(StringShiftLeftString), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_BIT_SLL,     "String& f(const String &in, double)",        asFUNCTION(StringShiftLeftDouble), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_BIT_SLL,     "String& f(const String &in, float)",         asFUNCTION(StringShiftLeftFloat), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_BIT_SLL,     "String& f(const String &in, int)",           asFUNCTION(StringShiftLeftInt), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_BIT_SLL,     "String& f(const String &in, uint)",          asFUNCTION(StringShiftLeftUInt), asCALL_CDECL); jassert( r >= 0 );
	r = engine->RegisterGlobalBehaviour (asBEHAVE_BIT_SLL,     "String& f(const String &in, bool)",          asFUNCTION(StringShiftLeftBool), asCALL_CDECL); jassert( r >= 0 );
}

} // end namespace Bindings

END_JUCE_NAMESPACE

#endif // JUCE_SUPPORT_SCRIPTING

