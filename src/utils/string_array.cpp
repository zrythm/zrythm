// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/logger.h"
#include "utils/string_array.h"

StringArray::StringArray (const char * const * strs)
{
  int          count = 0;
  const char * s;
  while (strs[count] != nullptr)
    {
      s = strs[count];

      add (juce::CharPointer_UTF8 (s));
      count++;
    }
}

StringArray::StringArray (const QStringList &qlist)
{
  for (const auto &s : qlist)
    {
      add (utils::qstring_to_std_string (s));
    }
}
void
StringArray::print (std::string title) const
{
  std::string str = title + ":";
  for (const auto &cur_str : arr_)
    {
      str += " " + utils::juce_string_to_std_string (cur_str) + " |";
    }
  str.erase (str.size () - 1);
  z_info ("{}", str);
}

QStringList
StringArray::toQStringList () const
{
  return arr_ | std::views::transform (utils::juce_string_to_qstring)
         | std::ranges::to<QStringList> ();
}

std::vector<std::string>
StringArray::toStdStringVector () const
{
  return arr_ | std::views::transform (utils::juce_string_to_std_string)
         | std::ranges::to<std::vector> ();
}
