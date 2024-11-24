// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef COMMON_UTILS_NOTE_TYPE_H
#define COMMON_UTILS_NOTE_TYPE_H

#include <string_view>

namespace zrythm::utils
{

enum class NoteLength
{
  Bar,
  Beat,
  Note_2_1,
  Note_1_1,
  Note_1_2,
  Note_1_4,
  Note_1_8,
  Note_1_16,
  Note_1_32,
  Note_1_64,
  Note_1_128
};

enum class NoteType
{
  Normal,
  Dotted, ///< 2/3 of its original size
  Triplet ///< 3/2 of its original size
};

std::string_view
note_length_to_str (NoteLength len);

std::string_view
note_type_to_str (NoteType type);

}; // namespace zrythm::utils

#endif // COMMON_UTILS_NOTE_TYPE_H
