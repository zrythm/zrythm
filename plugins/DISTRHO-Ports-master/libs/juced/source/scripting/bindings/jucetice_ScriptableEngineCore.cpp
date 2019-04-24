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
#include "jucetice_ScriptableEngineString.h"

using namespace angelscriptNamespace;

namespace Bindings {

//==============================================================================
void warningAlert (const asString &text)
{
    AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                                 "Alert",
                                 text.buffer);
}

void errorAlert (const asString &text)
{
    AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                 "Error",
                                 text.buffer);
}

int askAlert (const asString &text)
{
    return (int) AlertWindow::showOkCancelBox (AlertWindow::QuestionIcon,
                                               "Question",
                                               text.buffer);
}

void print (const asString &text)
{
    printf ("%s", (const char*) text.buffer);
}

void println (const asString &text)
{
    printf ("%s\n", (const char*) text.buffer);
}

void throwException (const asString &text)
{
    asIScriptContext *context = asGetActiveContext();
    if (context)
	    context->SetException ((const char *) text.buffer);
}

/*
static void executeScript (const String& script) {
    ScriptableEngine::getInstance ()->executeString (script);
}
static void abortApplication () {
   	ScriptableEngine::getInstance ()->abortExecution ();
}
static void quitApplication () {
    JUCEApplication::getInstance()->systemRequestedQuit();
}
static uint8* getReference (uint8* p)  {
   return p;
}
*/

void RegisterObjectTypeCore (ScriptableEngine* scriptEngine)
{
	int r;
	
	asIScriptEngine* engine = (asIScriptEngine*) scriptEngine->getCurrentEngine ();

	r = engine->RegisterGlobalFunction ("void alert(const String &in)",
	    asFUNCTIONPR (warningAlert, (const asString&), void), asCALL_CDECL); jassert( r >= 0 );

	r = engine->RegisterGlobalFunction ("void error(const String &in)",
	    asFUNCTIONPR (errorAlert, (const asString&), void), asCALL_CDECL); jassert( r >= 0 );

	r = engine->RegisterGlobalFunction ("int ask(const String &in)",
	    asFUNCTIONPR (askAlert, (const asString&), int), asCALL_CDECL); jassert( r >= 0 );

    r = engine->RegisterGlobalFunction ("void print(const String &in)",
        asFUNCTIONPR (print, (const asString&), void), asCALL_CDECL); jassert( r >= 0 );

	r = engine->RegisterGlobalFunction ("void println(const String &in)",
	    asFUNCTIONPR (println, (const asString&), void), asCALL_CDECL); jassert( r >= 0 );

	r = engine->RegisterGlobalFunction ("void throw(const String &in)",
	    asFUNCTIONPR (throwException, (const asString&), void), asCALL_CDECL); jassert( r >= 0 );
}

} // end namespace Bindings

#endif // JUCE_SUPPORT_SCRIPTING

