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

#ifndef __JUCETICE_SCRIPTABLEENGINESTRING_HEADER__
#define __JUCETICE_SCRIPTABLEENGINESTRING_HEADER__

#if JUCE_SUPPORT_SCRIPTING

class ScriptableEngine;

namespace Bindings {

//==============================================================================
/** Manages Strings in angel scripts

    @see ScriptableEngine, String
*/
class asString
{
public:

    asString ();
    asString (const asString &other);
    asString (const char s);
    asString (const char* s);
    asString (const tchar s);
    asString (const tchar* s);
    asString (const String &s);

    void addRef ();
    void release ();

    asString& operator= (const asString &other);
    asString& operator+= (const asString &other);

    bool equalsIgnoreCase (const asString& other);
    int compare (const asString& other);
    int compareIgnoreCase (const asString& other);
    int compareLexicographically (const asString& other);
    bool startsWith (const asString& text);
    bool startsWithIgnoreCase (const asString& text);
    bool endsWith (const asString& text);
    bool endsWithIgnoreCase (const asString& text);
    bool contains (const asString& text);
    bool containsIgnoreCase (const asString& text);
    bool containsWholeWord (const asString& wordToLookFor);
    bool containsWholeWordIgnoreCase (const asString& wordToLookFor);
    bool containsAnyOf (const asString& charactersItMightContain);
    bool containsOnly (const asString& charactersItMightContain);
    bool matchesWildcard (const asString& wildcard, const bool ignoreCase);
    int indexOf (const asString& other);
    int indexOf (const int startIndex, const asString& textToLookFor);
    int indexOfIgnoreCase (const asString& textToLookFor);
    int indexOfIgnoreCase (const int startIndex, const asString& textToLookFor);
    int lastIndexOf (const asString& textToLookFor);
    int lastIndexOfIgnoreCase (const asString& textToLookFor);
    asString* substring (int startIndex, int endIndex);
    asString* substring (const int startIndex);
    asString* dropLastCharacters (const int numberToDrop);
    asString* fromFirstOccurrenceOf (const asString& substringToStartFrom,
                                     const bool includeSubStringInResult,
                                     const bool ignoreCase);
    asString* fromLastOccurrenceOf (const asString& substringToFind,
                                    const bool includeSubStringInResult,
                                    const bool ignoreCase);
    asString* upToFirstOccurrenceOf (const asString& substringToEndWith,
                                     const bool includeSubStringInResult,
                                     const bool ignoreCase);
    asString* upToLastOccurrenceOf (const asString& substringToFind,
                                    const bool includeSubStringInResult,
                                    const bool ignoreCase);
    asString* trim();
    asString* trimStart();
    asString* trimEnd();
    asString* toUpperCase();
    asString* toLowerCase();
    asString* replaceSection (int startIndex,
                              int numCharactersToReplace,
                              const asString& stringToInsert);
    asString* replace (const asString& stringToReplace,
                       const asString& stringToInsertInstead,
                       const bool ignoreCase = false);
    asString* replaceCharacter (const asString& characterToReplace,
                                const asString& characterToInsertInstead);
    asString* replaceCharacters (const asString& charactersToReplace,
                                 const asString& charactersToInsertInstead);
    asString* retainCharacters (const asString& charactersToRetain);
    asString* removeCharacters (const asString& charactersToRemove);
    asString* initialSectionContainingOnly (const asString& charactersToRetain);
    asString* initialSectionNotContaining (const asString& charactersNotToRetain);
    asString* unquoted();
    asString* quoted (const asString& quoteCharacter = T('"'));

	String buffer;

protected:

	~asString();
	int refCount;
};

void RegisterObjectTypeString (ScriptableEngine* scriptEngine);

} // end namespace Bindings

#endif // JUCE_SUPPORT_SCRIPTING

#endif // __JUCETICE_SCRIPTABLEENGINESTRING_HEADER__
