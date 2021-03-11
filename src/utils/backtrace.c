/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
  Copyright (C) 2016 Florian Cabot

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "utils/datetime.h"
#include "utils/io.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#ifdef HAVE_LIBBACKTRACE
#include <backtrace-supported.h>
#include <backtrace.h>
#if BACKTRACE_SUPPORTED
#define CAN_USE_LIBBACKTRACE 1
#endif
#endif

#if !defined (_WOE32) && \
  !defined (CAN_USE_LIBBACKTRACE)
/**
 * Resolve symbol name and source location given
 * the path to the executable and an address.
 *
 * @param buf Buffer to save the output in.
 *
 * @return 0 if address has been resolved and a
 * message has been printed; else returns 1.
 */
static int
addr2line (
  char const * const program_name,
  void const * const addr,
  char *             buf,
  int                lineNb)
{
  char addr2line_cmd[512] = {0};

  /* have addr2line map the address to the relent line in the code */
  #ifdef __APPLE__
  /* apple does things differently... */
  /* FIXME re-enable */
  // sprintf(addr2line_cmd,"atos -o %.256s %p", program_name, addr);
  return 1;
  #else
  sprintf (
    addr2line_cmd, "addr2line -f -e %.256s %p",
    program_name, addr);
  #endif

  /* This will print a nicely formatted string specifying the
  function and source line of the address */

  FILE *fp;
  char outLine1[1035];
  char outLine2[1035];

  /* Open the command for reading. */
  /* FIXME use reproc, this uses fork () */
  fp = popen (addr2line_cmd, "r");
  if (fp == NULL)
    return 1;

  while (fgets(outLine1, sizeof(outLine1)-1, fp) != NULL)
  {
    //if we have a pair of lines
    if(fgets(outLine2, sizeof(outLine2)-1, fp) != NULL)
    {
      //if symbols are readable
      if(outLine2[0] != '?')
      {
        //only let func name in outLine1
        int i;
        for(i = 0; i < 1035; ++i)
        {
          if(outLine1[i] == '\r' || outLine1[i] == '\n')
          {
            outLine1[i] = '\0';
            break;
          }
        }

        //don't display the whole path
        int lastSlashPos=0;

        for(i = 0; i < 1035; ++i)
        {
          if(outLine2[i] == '\0')
            break;
          if(outLine2[i] == '/')
            lastSlashPos = i+1;
        }
        sprintf (
          buf, "[%i] %p in %s at %s",
          lineNb, addr, outLine1,
          outLine2+lastSlashPos);
      }
      else
      {
        pclose(fp);
        return 1;
      }
    }
    else
    {
      pclose(fp);
      return 1;
    }
  }

  /* close */
  pclose(fp);

  return 0;
}
#endif

#ifdef CAN_USE_LIBBACKTRACE
static struct backtrace_state * state = NULL;

static void
syminfo_cb (
  void *       data,
  uintptr_t    pc,
  const char * symname,
  uintptr_t    symval,
  uintptr_t    symsize)
{
  GString * msg_str = (GString *) data;

  if (symname)
    {
      g_string_append_printf (
        msg_str, " %s", symname);
    }
}

static int
full_cb (
  void *       data,
  uintptr_t    pc,
  const char * filename,
  int          lineno,
  const char * function)
{
  GString * msg_str = (GString *) data;

  if (filename)
    {
      g_string_append_printf (
        msg_str, "%s", filename);
    }
  else
    {
      g_string_append_printf (
        msg_str, "%s", "???");
    }

  if (function)
    {
      g_string_append_printf (
        msg_str, " (%s:%d)",
        function, lineno);
    }
  else
    {
      backtrace_syminfo (
        state, pc, syminfo_cb, NULL, msg_str);
    }

  g_string_append_printf (
    msg_str, " [%lu]\n", pc);

  return 0;
}

#endif

/**
 * Returns the backtrace with \ref max_lines
 * number of lines and a string prefix.
 *
 * @param exe_path Executable path for running
 *   addr2line.
 * @param with_lines Whether to show line numbers.
 *   This is very slow.
 */
char *
_backtrace_get (
  const char * exe_path,
  const char * prefix,
  int          max_lines,
  bool         with_lines,
  bool         write_to_file)
{
  char message[12000];
  char current_line[2000];
  (void) current_line;

  strcpy (message, prefix);

#ifdef CAN_USE_LIBBACKTRACE
  GString * msg_str = g_string_new (prefix);

  state =
    backtrace_create_state (
      exe_path, true, NULL, NULL);

  if (write_to_file)
    {
      char * str_datetime =
        datetime_get_for_filename ();
      char * user_bt_dir =
        zrythm_get_dir (ZRYTHM_DIR_USER_BACKTRACE);
      char * backtrace_filepath =
        g_strdup_printf (
          "%s%sbacktrace_%s.txt",
          user_bt_dir, G_DIR_SEPARATOR_S,
          str_datetime);
      io_mkdir (user_bt_dir);
      FILE * f = fopen (backtrace_filepath, "a");
      backtrace_print (state, 0, f);
      fclose (f);
      g_free (str_datetime);
      g_free (user_bt_dir);
      g_free (backtrace_filepath);
    }

  backtrace_full (
    state, 0, full_cb, NULL, msg_str);

  return g_string_free (msg_str, false);

#elif defined (_WOE32)
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

  IMAGEHLP_LINE64 * line =
    calloc (
      sizeof (IMAGEHLP_LINE64) +
        256 * sizeof( char ),
      1);
  line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  for (i = 0; i < frames; i++)
    {
      SymFromAddr (
        process, (DWORD64) (stack[i]), 0,
        symbol);

      bool got_line = false;
      if (with_lines)
        {
          got_line =
            SymGetLineFromAddr64 (
              process, (DWORD64) (stack[i]), 0,
              line);

          if (!got_line)
            {
              g_message (
                "failed to get line info for %s",
                symbol->Name);
            }
        }

      if (got_line)
        {
          sprintf (
            current_line,
            "%u: %s - 0x%0X %s:%d\n",
            frames - i - 1,
            symbol->Name, symbol->Address,
            line->FileName, line->LineNumber);
        }
      else
        {
          sprintf (
            current_line,
            "%u: %s - 0x%0X\n", frames - i - 1,
            symbol->Name, symbol->Address);
        }

      strcat (message, current_line);
    }
  free (symbol);
  free (line);
#else
  void * array[max_lines];
  char ** strings;

  /* get void*'s for all entries on the stack */
  int size = backtrace (array, max_lines);

  /* print out all the frames to stderr */
  strings = backtrace_symbols (array, size);
  for (int i = 0; i < size - 2; i++)
    {
      /* if addr2line failed, print what we can */
      if (!with_lines ||
          addr2line (
            exe_path, array[i],
            current_line,
            size - 2 - i - 1) != 0)
        {
          sprintf (
            current_line, "%s\n", strings[i]);
        }
      strcat (message, current_line);
    }
  free (strings);
#endif

  return g_strdup (message);
}
