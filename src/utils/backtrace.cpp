// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
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
 *
 * ---
 */

#include "zrythm-config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#  include <windows.h>
/*#define DBGHELP_TRANSLATE_TCHAR*/
#  include <dbghelp.h>
#else
#  include <execinfo.h>
#endif

#include "utils/backtrace.h"
#include "utils/datetime.h"
#include "utils/io.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

#ifdef HAVE_LIBBACKTRACE
#  include <backtrace-supported.h>
#  include <backtrace.h>
#  if BACKTRACE_SUPPORTED
#    define CAN_USE_LIBBACKTRACE 1
#  endif
#endif

#if !defined(_WIN32)
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
  char addr2line_cmd[512] = {};

/* have addr2line map the address to the relent line in the code */
#  ifdef __APPLE__
  /* apple does things differently... */
  /* FIXME re-enable */
  // sprintf(addr2line_cmd,"atos -o %.256s %p", program_name, addr);
  return 1;
#  else
  sprintf (addr2line_cmd, "addr2line -f -e %.256s %p", program_name, addr);
#  endif

  /* This will print a nicely formatted string specifying the
  function and source line of the address */

  FILE * fp;
  char   outLine1[1035];
  char   outLine2[1035];

  /* Open the command for reading. */
  /* FIXME use reproc, this uses fork () */
  fp = popen (addr2line_cmd, "r");
  if (fp == NULL)
    return 1;

  while (fgets (outLine1, sizeof (outLine1) - 1, fp) != NULL)
    {
      // if we have a pair of lines
      if (fgets (outLine2, sizeof (outLine2) - 1, fp) != NULL)
        {
          // if symbols are readable
          if (outLine2[0] != '?')
            {
              // only let func name in outLine1
              int i;
              for (i = 0; i < 1035; ++i)
                {
                  if (outLine1[i] == '\r' || outLine1[i] == '\n')
                    {
                      outLine1[i] = '\0';
                      break;
                    }
                }

              // don't display the whole path
              int lastSlashPos = 0;

              for (i = 0; i < 1035; ++i)
                {
                  if (outLine2[i] == '\0')
                    break;
                  if (outLine2[i] == '/')
                    lastSlashPos = i + 1;
                }
              sprintf (
                buf, "[%i] %p in %s at %s", lineNb, addr, outLine1,
                outLine2 + lastSlashPos);
            }
          else
            {
              pclose (fp);
              return 1;
            }
        }
      else
        {
          pclose (fp);
          return 1;
        }
    }

  /* close */
  pclose (fp);

  return 0;
}
#endif

#ifdef CAN_USE_LIBBACKTRACE
static struct backtrace_state * state = NULL;

/**
 * Note: to get the line number for external libs
 * use `addr2line -e /lib64/libc.so.6 0x27a50`.
 *
 * __libc_start_main in ../csu/libc-start.c:332 from
 * /lib64/libc.so.6(+0x27a50)[0x7feffe060000].
 */
static void
syminfo_cb (
  void *       data,
  uintptr_t    pc,
  const char * symname,
  uintptr_t    symval,
  uintptr_t    symsize
#  ifdef BACKTRACE_HAVE_BIN_FILENAME_AND_BASE_ADDR
  ,
  const char * binary_filename,
  uintptr_t    base_address
#  endif
)
{
  GString * msg_str = (GString *) data;

  if (!symname)
    {
      symname = "unknown";
    }

#  ifdef BACKTRACE_HAVE_BIN_FILENAME_AND_BASE_ADDR
  if (base_address != 0 || symval != 0)
    {
      g_string_append_printf (
        msg_str, " %s from %s(+0x%lx)[0x%lx]", symname, binary_filename,
        symval - base_address, base_address);
    }
  else
#  endif
    {
      g_string_append_printf (msg_str, " %s", symname);
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
  GString ** msg_str = (GString **) data;

  if (!(*msg_str))
    {
      return -1;
    }

  if (filename)
    {
      g_string_append_printf (*msg_str, "%s", filename);
    }
  else
    {
      g_string_append_printf (*msg_str, "%s", "???");
    }

  if (function)
    {
      g_string_append_printf (*msg_str, " (%s:%d)", function, lineno);
    }
  else
    {
      backtrace_syminfo (state, pc, syminfo_cb, NULL, *msg_str);
    }

  /*g_string_append_printf (*/
  /*msg_str, " [%lu]\n", pc);*/
  g_string_append_printf (*msg_str, "%s", "\n");

  return 0;
}

static void
bt_error_cb (void * data, const char * msg, int errnum)
{
  GString ** msg_str = (GString **) data;
  g_message ("backtrace error: %s", msg);

  if (*msg_str)
    {
      g_string_free (*msg_str, true);
      *msg_str = NULL;
    }
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

  state = backtrace_create_state (exe_path, true, bt_error_cb, &msg_str);

  if (state && msg_str)
    {

      if (write_to_file && gZrythm)
        {
          char * str_datetime = datetime_get_for_filename ();
          char * user_bt_dir =
            gZrythm->dir_mgr->get_dir (ZRYTHM_DIR_USER_BACKTRACE);
          char * backtrace_filepath = g_strdup_printf (
            "%s%sbacktrace_%s.txt", user_bt_dir, G_DIR_SEPARATOR_S,
            str_datetime);
          GError * err = NULL;
          bool     success = io_mkdir (user_bt_dir, &err);
          if (!success)
            {
              g_warning ("failed to create directory file %s", user_bt_dir);
              goto call_backtrace_full;
            }
          FILE * f = fopen (backtrace_filepath, "a");
          if (f)
            {
              backtrace_print (state, 0, f);
              fclose (f);
            }
          else
            {
              g_message ("failed to open file %s", backtrace_filepath);
              g_free (str_datetime);
              g_free (user_bt_dir);
              g_free (backtrace_filepath);
              goto call_backtrace_full;
            }
          g_free (str_datetime);
          g_free (user_bt_dir);
          g_free (backtrace_filepath);
        }

call_backtrace_full:
      if (msg_str)
        {
          g_message ("getting bt");
          int ret = backtrace_full (state, 0, full_cb, bt_error_cb, &msg_str);
          g_message ("ret %d", ret);

          if (msg_str)
            {
              /* replace multiple instances of ??? with a single
               * one */
              char * bt_str = g_string_free (msg_str, false);
              string_replace_regex (&bt_str, "(\\?\\?\\?\n)+\\1", "??? ...\n");

              return bt_str;
            }
          else
            {
              goto read_traditional_bt;
            }
        }
      else
        {
          goto read_traditional_bt;
        }
    }
  else
    {
read_traditional_bt:
      g_message (
        "failed to get a backtrace state, creating traditional "
        "backtrace...");
    }
#endif /* CAN_USE_LIBBACKTRACE */

#if defined(_WIN32)
  void * stack[100];
  HANDLE process = GetCurrentProcess ();

  SymInitialize (process, NULL, TRUE);

  unsigned short frames = CaptureStackBackTrace (0, 100, stack, NULL);
  SYMBOL_INFO *  symbol =
    (SYMBOL_INFO *) calloc (sizeof (SYMBOL_INFO) + 256 * sizeof (char), 1);
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

  IMAGEHLP_LINE64 * line = (IMAGEHLP_LINE64 *) calloc (
    sizeof (IMAGEHLP_LINE64) + 256 * sizeof (char), 1);
  line->SizeOfStruct = sizeof (IMAGEHLP_LINE64);

  for (unsigned int i = 0; i < frames; i++)
    {
      DWORD64 address = (DWORD64) (stack[i]);
      SymFromAddr (process, address, 0, symbol);

      bool got_line = false;
      if (with_lines)
        {
          DWORD dwDisplacement;
          got_line =
            SymGetLineFromAddr64 (process, address, &dwDisplacement, line);

          if (!got_line)
            {
              g_message ("failed to get line info for %s", symbol->Name);
            }
        }

      if (got_line)
        {
          sprintf (
            current_line, "%u: %s - 0x%0X %s:%d\n", frames - i - 1,
            symbol->Name, symbol->Address, line->FileName, line->LineNumber);
        }
      else
        {
          sprintf (
            current_line, "%u: %s - 0x%0X\n", frames - i - 1, symbol->Name,
            symbol->Address);
        }

      strcat (message, current_line);
    }
  free (symbol);
  free (line);
#else /* else if not _WIN32 */
  void *  array[max_lines];
  char ** strings;

  /* get void*'s for all entries on the stack */
  int size = backtrace (array, max_lines);

  /* print out all the frames to stderr */
  strings = backtrace_symbols (array, size);
  for (int i = 0; i < size - 2; i++)
    {
      /* if addr2line failed, print what we can */
      if (
        !with_lines
        || addr2line (exe_path, array[i], current_line, size - 2 - i - 1) != 0)
        {
          sprintf (current_line, "%s\n", strings[i]);
        }
      strcat (message, current_line);
    }
  free (strings);
#endif

  return g_strdup (message);
}
