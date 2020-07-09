/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WOE32
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif

#include "utils/backtrace.h"

#include <gtk/gtk.h>

/**
 * Returns the backtrace with \ref max_lines number of
 * lines and a string prefix.
 */
char *
backtrace_get (
  const char * prefix,
  int          max_lines)
{
  char message[12000];
  char current_line[2000];

  strcpy (message, prefix);

#ifdef _WOE32
  unsigned int   i;
  void         * stack[ 100 ];
  unsigned short frames;
  SYMBOL_INFO  * symbol;
  HANDLE         process;

  process = GetCurrentProcess();

  SymInitialize (process, NULL, TRUE);

  frames =
    CaptureStackBackTrace (0, 100, stack, NULL);
  symbol =
    (SYMBOL_INFO *)
    calloc (
      sizeof (SYMBOL_INFO) + 256 * sizeof( char ),
      1);
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

  for (i = 0; i < frames; i++)
    {
      SymFromAddr (
        process, (DWORD64) (stack[i]), 0,
        symbol);

      sprintf (
        current_line,
        "%u: %s - 0x%0X\n", frames - i - 1,
        symbol->Name, symbol->Address);
      strcat (message, current_line);
    }
  free (symbol);
#else
  void * array[max_lines];
  char ** strings;

  /* get void*'s for all entries on the stack */
  int size = backtrace (array, max_lines);

  /* print out all the frames to stderr */
  strings = backtrace_symbols (array, size);
  for (int i = 0; i < size; i++)
    {
      sprintf (current_line, "%s\n", strings[i]);
      strcat (message, current_line);
    }
  free (strings);
#endif

  return g_strdup (message);
}
