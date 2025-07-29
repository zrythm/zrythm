// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_map>

#include <QtTranslation>

#include "./logger.h"
#include "./note_type.h"

namespace zrythm::utils
{
static const std::unordered_map<NoteLength, std::string_view> note_length_strings = {
  { NoteLength::Bar,        QT_TR_NOOP_UTF8 ("bar")  },
  { NoteLength::Beat,       QT_TR_NOOP_UTF8 ("beat") },
  { NoteLength::Note_2_1,   "2/1"                    },
  { NoteLength::Note_1_1,   "1/1"                    },
  { NoteLength::Note_1_2,   "1/2"                    },
  { NoteLength::Note_1_4,   "1/4"                    },
  { NoteLength::Note_1_8,   "1/8"                    },
  { NoteLength::Note_1_16,  "1/16"                   },
  { NoteLength::Note_1_32,  "1/32"                   },
  { NoteLength::Note_1_64,  "1/64"                   },
  { NoteLength::Note_1_128, "1/128"                  },
};

static const std::unordered_map<NoteType, std::string_view> note_type_strings = {
  { NoteType::Normal,  QT_TR_NOOP_UTF8 ("normal")  },
  { NoteType::Dotted,  QT_TR_NOOP_UTF8 ("dotted")  },
  { NoteType::Triplet, QT_TR_NOOP_UTF8 ("triplet") },
};

std::string_view
note_length_to_str (NoteLength len)
{
  auto it = note_length_strings.find (len);
  z_return_val_if_fail (it != note_length_strings.end (), "invalid");
  return it->second;
}

std::string_view
note_type_to_str (NoteType type)
{
  auto it = note_type_strings.find (type);
  z_return_val_if_fail (it != note_type_strings.end (), "invalid");
  return it->second;
}

} // namespace zrythm::utils
