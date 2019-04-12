#include "project.h"
#include "zrythm.h"

#include "test/automations.h"

#include <glib.h>
#include <locale.h>

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  ZRYTHM = calloc (1, sizeof (Zrythm));
  PROJECT = calloc (1, sizeof (Project));
  g_message ("test message");

  // Define the tests.
  g_test_add (
    "/automations/at_get_x_relevant_to_pos",
    AutomationTrackFixture, "some-user-data",
    at_fixture_set_up,
    test_at_get_x_relevant_to_pos,
    at_fixture_tear_down);

  /*g_test_add ("/my-object/test2", MyObjectFixture, "some-user-data",*/
              /*my_object_fixture_set_up, test_my_object_test2,*/
              /*my_object_fixture_tear_down);*/

  return g_test_run ();
}
