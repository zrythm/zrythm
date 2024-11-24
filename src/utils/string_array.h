// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_STRING_ARRAY_H__
#define __UTILS_STRING_ARRAY_H__

#include <QStringList>

#include "juce_wrapper.h"
#include "utils/types.h"

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * String array that auto-converts given char pointers to UTF8 (so JUCE doesn't
 * complain.
 */
class StringArray
{
public:
  StringArray () = default;
  StringArray (const std::initializer_list<const char *> &_strings)
  {
    arr_ = juce::StringArray (_strings);
  }
  StringArray (const std::initializer_list<std::string> &_strings)
  {
    arr_ = juce::StringArray (_strings);
  }
  StringArray (const char * const * strs);

  StringArray (const QStringList &qlist);

  void insert (int index, const char * s)
  {
    arr_.insert (index, juce::CharPointer_UTF8 (s));
  };
  void   add (const std::string &s) { arr_.add (s); }
  size_t size () const { return arr_.size (); }
  void   add (const char * s) { arr_.add (juce::CharPointer_UTF8 (s)); };
  void   set (int index, const char * s)
  {
    arr_.set (index, juce::CharPointer_UTF8 (s));
  };
  juce::String getReference (int index) const
  {
    return arr_.getReference (index);
  }
  const char * operator[] (size_t i)
  {
    return arr_.getReference (i).toRawUTF8 ();
  };
  void removeString (const char * s)
  {
    arr_.removeString (juce::CharPointer_UTF8 (s));
  };

  void removeDuplicates (bool ignore_case = false)
  {
    arr_.removeDuplicates (ignore_case);
  }

  void addArray (const StringArray &other) { arr_.addArray (other.arr_); };

  void remove (int index) { arr_.remove (index); };

  bool isEmpty () const { return arr_.isEmpty (); };
  bool contains (const juce::StringRef &s, bool ignore_case = false) const
  {
    return arr_.contains (s, ignore_case);
  }

  std::vector<std::string> toStdStringVector () const;

  QStringList toQStringList () const;

  void print (std::string title) const;

  // Iterator support
  auto begin () { return arr_.begin (); }
  auto end () { return arr_.end (); }
  auto begin () const { return arr_.begin (); }
  auto end () const { return arr_.end (); }

  // Range-based for loop support
  auto &operator* () { return *this; }
  auto &operator++ () { return *this; }
  bool  operator!= (const StringArray &) const { return false; }

private:
  juce::StringArray arr_;

  JUCE_HEAVYWEIGHT_LEAK_DETECTOR (StringArray)
};

/**
 * @}
 */

#endif // __UTILS_STRING_ARRAY_H__
