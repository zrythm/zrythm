/*
  Copyright (c) 2018 vikyd

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */

/* This file is taken from
 * https://github.com/vikyd/cpu_usage */

#ifdef _WOE32

#  include <Windows.h>

#  include <stdio.h>

#  include <iostream>

using namespace std;

typedef long long          int64_t;
typedef unsigned long long uint64_t;

/// time convert
static uint64_t
file_time_2_utc (const FILETIME * ftime)
{
  LARGE_INTEGER li;

  li.LowPart = ftime->dwLowDateTime;
  li.HighPart = ftime->dwHighDateTime;
  return li.QuadPart;
}

// get CPU num
static int
get_processor_number ()
{
  SYSTEM_INFO info;
  GetSystemInfo (&info);
  return (int) info.dwNumberOfProcessors;
}

extern "C" {

int
cpu_windows_get_usage (int pid)
{
  if (pid == -1)
    {
      pid = GetCurrentProcessId ();
    }

  static int     processor_count_ = -1;
  static int64_t last_time_ = 0;
  static int64_t last_system_time_ = 0;

  FILETIME now;
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  int64_t  system_time;
  int64_t  time;
  int64_t  system_time_delta;
  int64_t  time_delta;

  int cpu = -1;

  if (processor_count_ == -1)
    {
      processor_count_ = get_processor_number ();
    }

  GetSystemTimeAsFileTime (&now);

  HANDLE hProcess =
    OpenProcess (PROCESS_ALL_ACCESS, false, pid);
  if (!GetProcessTimes (
        hProcess, &creation_time, &exit_time, &kernel_time,
        &user_time))
    {
      // can not find the process
      exit (EXIT_FAILURE);
    }
  system_time =
    (file_time_2_utc (&kernel_time)
     + file_time_2_utc (&user_time))
    / processor_count_;
  time = file_time_2_utc (&now);

  if ((last_system_time_ == 0) || (last_time_ == 0))
    {
      last_system_time_ = system_time;
      last_time_ = time;
      return cpu_windows_get_usage (pid);
    }

  system_time_delta = system_time - last_system_time_;
  time_delta = time - last_time_;

  if (time_delta == 0)
    {
      return cpu_windows_get_usage (pid);
    }

  cpu =
    (int) ((system_time_delta * 100 + time_delta / 2) / time_delta);
  last_system_time_ = system_time;
  last_time_ = time;
  return cpu;
}

} // extern "C"

#endif // _WOE32
