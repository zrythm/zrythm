// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

namespace zrythm::utils
{
Q_NAMESPACE

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
Q_ENUM_NS (NoteLength);

enum class NoteType
{
  Normal,
  Dotted, ///< 2/3 of its original size
  Triplet ///< 3/2 of its original size
};
Q_ENUM_NS (NoteType)

std::string_view
note_length_to_str (NoteLength len);

std::string_view
note_type_to_str (NoteType type);

}; // namespace zrythm::utils
