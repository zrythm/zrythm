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

#if JUCE_LASH

//==============================================================================
/* Got an include error here? If so, you've either not got LASH installed, or
   you've not got your paths set up correctly to find its header files.

   The package you need to install to get LASH support is "liblash-dev".

   If you don't have the LASH library and don't want to build juce with LASH support,
   just disable the JUCE_LASH flag in juce_Config.h
*/
#include <lash/lash.h>

BEGIN_JUCE_NAMESPACE

//==============================================================================
LashManager::LashManager (AudioProcessor* filter_)
  : filter (filter_),
    client (0)
{
    int argc = 0;
    char** argv = 0;

    if (filter)
    {
        client = lash_init (lash_extract_args(&argc, &argv),
                            filter->getName(),
                            LASH_Config_File,
                            LASH_PROTOCOL(2, 0));

        if (client)
        {
            lash_event_t* event;

            event = lash_event_new_with_type (LASH_Client_Name);
            lash_event_set_string (event, filter->getName());
            lash_send_event ((lash_client_t*) client, event);

            event = lash_event_new_with_type (LASH_Jack_Client_Name);
            lash_event_set_string (event, filter->getName());
            lash_send_event ((lash_client_t*) client, event);

            startTimer (1000 / 2);
        }
    }
}

LashManager::~LashManager()
{
    if (client && isTimerRunning ())
        stopTimer ();
}

//==============================================================================
void LashManager::loadState (const File& fileToLoad)
{
    MemoryBlock fileData;

    if (fileToLoad.existsAsFile()
        && fileToLoad.loadFileAsData (fileData))
    {
        filter->setStateInformation (fileData.getData (), fileData.getSize());
    }
}

void LashManager::saveState (const File& fileToSave)
{
    MemoryBlock fileData;
    filter->getStateInformation (fileData);

    if (fileToSave.replaceWithData (fileData.getData (), fileData.getSize()))
    {
    }
}

//==============================================================================
void LashManager::timerCallback()
{
    if (! client || ! filter)
        return;

    lash_event_t* event;

    while ((event = lash_get_event ((lash_client_t*) client)) != 0)
    {
        String eventString (lash_event_get_string (event));
        
        switch (lash_event_get_type (event))
        {
        case LASH_Save_File:
            printf ("Asked to save data in %s\n", (const char*) eventString);
            saveState (File (eventString + "/" + filter->getName () + ".lash"));
            lash_send_event ((lash_client_t*) client, lash_event_new_with_type (LASH_Save_File));
            break;

        case LASH_Restore_File:
            printf ("Asked to restore data from %s\n", (const char*) eventString);
            loadState (File (eventString + "/" + filter->getName () + ".lash"));
            lash_send_event ((lash_client_t*) client, lash_event_new_with_type (LASH_Restore_File));
            break;

        case LASH_Quit:
            printf ("Asked to quit !\n");
            stopTimer ();
            break;

        case LASH_Server_Lost:
            printf ("Server lost !\n");
            stopTimer ();
            lash_event_destroy (event);
            return;

        default:
            printf("Got unhandled LADCCA event\n");
            break;
        }
        
        lash_event_destroy (event);
    }

/*
    while ((config = lash_get_config(client)) != 0)
    {
        printf ("Unexpected LASH config: %s\n", lash_config_get_key(config));

        lash_config_free(config);
        free(config);
        config = 0;
    }
*/
}

END_JUCE_NAMESPACE

#endif
