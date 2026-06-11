// SPDX-FileCopyrightText: © 2022-2024, 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <fmt/std.h>

#include "gui/backend/backend/settings/chord_preset_pack_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/zrythm_application.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/io_utils.h"
#include "utils/qt.h"

using namespace zrythm;

std::filesystem::path
ChordPresetPackManager::get_user_packs_path ()
{
  auto zrythm_dir =
    dynamic_cast<gui::ZrythmApplication *> (qApp)
      ->get_directory_manager ()
      .get_dir (IDirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return zrythm_dir / UserPacksDirName;
}

void
ChordPresetPackManager::add_standard_packs ()
{
  using namespace zrythm::dsp;

  packs_.reserve (100);

  struct ChordDef
  {
    MusicalNote root;
    ChordType   type;
    ChordAccent accent = ChordAccent::None;
    int         inversion = 0;
  };

  const auto add_chords =
    [] (ChordPreset &pset, std::initializer_list<ChordDef> chords) {
      for (const auto &def : chords)
        {
          auto descr = zrythm::utils::make_qobject_unique<ChordDescriptor> (
            def.root, def.type);
          if (def.accent != ChordAccent::None)
            descr->setChordAccent (def.accent);
          if (def.inversion != 0)
            descr->setInversion (def.inversion);
          pset.descr_.push_back (std::move (descr));
        }
    };

  /* --- euro pop pack --- */

  auto pack = std::make_unique<ChordPresetPack> (QObject::tr ("Euro Pop"), true);

  {
    ChordPreset pset (QObject::tr ("4 Chord Song"));
    add_chords (
      pset,
      {
        { MusicalNote::A, ChordType::Minor },
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::F, ChordType::Major },
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::G, ChordType::Major, ChordAccent::Seventh },
    });
    pack->add_preset (pset);
  }

  {
    /* Johann Pachelbel, My Chemical Romance */
    ChordPreset pset (QObject::tr ("Canon in D"));
    add_chords (
      pset,
      {
        { MusicalNote::D,      ChordType::Major },
        { MusicalNote::A,      ChordType::Major },
        { MusicalNote::B,      ChordType::Minor },
        { MusicalNote::FSharp, ChordType::Minor },
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::D,      ChordType::Major },
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::A,      ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    ChordPreset pset (QObject::tr ("Love Progression"));
    add_chords (
      pset,
      {
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::A, ChordType::Minor },
        { MusicalNote::F, ChordType::Major },
        { MusicalNote::G, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    ChordPreset pset (QObject::tr ("Pop Chords 1"));
    add_chords (
      pset,
      {
        { MusicalNote::C,      ChordType::Major },
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::A,      ChordType::Minor },
        { MusicalNote::F,      ChordType::Major },
        { MusicalNote::E,      ChordType::Major },
        { MusicalNote::B,      ChordType::Major },
        { MusicalNote::CSharp, ChordType::Minor },
        { MusicalNote::A,      ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    ChordPreset pset (QObject::tr ("Most Often Used Chords"));
    add_chords (
      pset,
      {
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::F,      ChordType::Major },
        { MusicalNote::C,      ChordType::Major },
        { MusicalNote::A,      ChordType::Minor },
        { MusicalNote::D,      ChordType::Minor },
        { MusicalNote::E,      ChordType::Minor },
        { MusicalNote::E,      ChordType::Major },
        { MusicalNote::D,      ChordType::Major },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::A,      ChordType::Major },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- j/k pop --- */

  pack = std::make_unique<ChordPresetPack> (QObject::tr ("Eastern Pop"), true);

  {
    ChordPreset pset (QObject::tr ("Fight Together"));
    add_chords (
      pset,
      {
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::A, ChordType::Major },
        { MusicalNote::D, ChordType::Major },
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::A, ChordType::Major },
        { MusicalNote::B, ChordType::Minor },
        { MusicalNote::D, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    ChordPreset pset (QObject::tr ("Gee"));
    add_chords (
      pset,
      {
        { MusicalNote::A, ChordType::Major, ChordAccent::Seventh },
        { MusicalNote::FSharp, ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Minor, ChordAccent::Seventh },
        { MusicalNote::CSharp, ChordType::Minor },
        { MusicalNote::CSharp, ChordType::Minor, ChordAccent::Seventh },
    });
    pack->add_preset (pset);
  }

  {
    /* yuriyurarararayuruyuri */
    ChordPreset pset (QObject::tr ("Daijiken"));
    add_chords (
      pset,
      {
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::F,      ChordType::Major },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::G,      ChordType::Minor },
        { MusicalNote::F,      ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::C,      ChordType::Minor },
        { MusicalNote::C,      ChordType::Major },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- dance --- */

  pack = std::make_unique<ChordPresetPack> (QObject::tr ("Dance"), true);

  {
    /* the idolm@ster 2 */
    ChordPreset pset (QObject::tr ("Idol 2"));
    add_chords (
      pset,
      {
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::D, ChordType::Major },
        { MusicalNote::B, ChordType::Minor },
        { MusicalNote::E, ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- ballad --- */

  pack = std::make_unique<ChordPresetPack> (QObject::tr ("Ballad"), true);

  {
    /* snow halation */
    ChordPreset pset (QObject::tr ("Snow Halation"));
    add_chords (
      pset,
      {
        { MusicalNote::D, ChordType::Major },
        { MusicalNote::E, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Minor },
        { MusicalNote::FSharp, ChordType::Minor },
        { MusicalNote::B, ChordType::Minor, ChordAccent::Seventh },
        { MusicalNote::CSharp, ChordType::Major, ChordAccent::Seventh },
        { MusicalNote::E, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Minor },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::B, ChordType::Major },
        { MusicalNote::E, ChordType::Major },
        { MusicalNote::A, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    /* connect */
    ChordPreset pset (QObject::tr ("Connect"));
    add_chords (
      pset,
      {
        { MusicalNote::B, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::DSharp, ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Minor },
        { MusicalNote::B, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major, ChordAccent::Seventh },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::G, ChordType::Major, ChordAccent::Seventh },
        { MusicalNote::C, ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  {
    /* secret base */
    ChordPreset pset (QObject::tr ("Secret Base"));
    add_chords (
      pset,
      {
        { MusicalNote::B, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Minor, ChordAccent::Seventh },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::B, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::FSharp, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- eurodance --- */

  pack = std::make_unique<ChordPresetPack> (QObject::tr ("Eurodance"), true);

  {
    /* what is love */
    ChordPreset pset (QObject::tr ("What is Love"));
    add_chords (
      pset,
      {
        { MusicalNote::G, ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::D, ChordType::Minor, ChordAccent::Seventh },
        { MusicalNote::F, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    /* blue */
    ChordPreset pset (QObject::tr ("Blue"));
    add_chords (
      pset,
      {
        { MusicalNote::G,      ChordType::Minor },
        { MusicalNote::F,      ChordType::Major },
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::C,      ChordType::Minor },
        { MusicalNote::C,      ChordType::Major },
        { MusicalNote::D,      ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::ASharp, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- eurobeat --- */

  pack = std::make_unique<ChordPresetPack> (QObject::tr ("Eurobeat"), true);

  {
    ChordPreset pset (QObject::tr ("Burning Night"));
    add_chords (
      pset,
      {
        { MusicalNote::CSharp, ChordType::Major      },
        { MusicalNote::DSharp, ChordType::Major      },
        { MusicalNote::C,      ChordType::Minor      },
        { MusicalNote::F,      ChordType::Minor      },
        { MusicalNote::ASharp, ChordType::Minor      },
        { MusicalNote::B,      ChordType::Diminished },
        { MusicalNote::C,      ChordType::Major      },
    });
    pack->add_preset (pset);
  }

  {
    /* believe / dreamin' of you */
    ChordPreset pset (QObject::tr ("Dreamin' Of You"));
    add_chords (
      pset,
      {
        { MusicalNote::F,      ChordType::Major },
        { MusicalNote::C,      ChordType::Major },
        { MusicalNote::D,      ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::G,      ChordType::Minor },
        { MusicalNote::A,      ChordType::Major },
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::D,      ChordType::Major },
        { MusicalNote::FSharp, ChordType::Minor },
        { MusicalNote::B,      ChordType::Minor },
        { MusicalNote::E,      ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  /* get me power */
  {
    ChordPreset pset (QObject::tr ("Get Me Power"));
    add_chords (
      pset,
      {
        { MusicalNote::B,      ChordType::Minor },
        { MusicalNote::E,      ChordType::Minor },
        { MusicalNote::D,      ChordType::Major },
        { MusicalNote::A,      ChordType::Major },
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::C,      ChordType::Major },
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::A,      ChordType::Minor },
        { MusicalNote::D,      ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  {
    /* night of fire */
    ChordPreset pset (QObject::tr ("Night of Fire"));
    add_chords (
      pset,
      {
        { MusicalNote::DSharp, ChordType::Minor },
        { MusicalNote::B,      ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Minor },
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::B,      ChordType::Major },
        { MusicalNote::GSharp, ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::CSharp, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    /* super fever night */
    ChordPreset pset (QObject::tr ("Super Fever Night"));
    add_chords (
      pset,
      {
        { MusicalNote::B, ChordType::Minor },
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::A, ChordType::Major },
        { MusicalNote::B, ChordType::Minor },
        { MusicalNote::G, ChordType::Major, ChordAccent::Seventh },
        { MusicalNote::A, ChordType::Major },
        { MusicalNote::D, ChordType::Major },
        { MusicalNote::E, ChordType::Major },
        { MusicalNote::FSharp, ChordType::Minor },
        { MusicalNote::FSharp, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    /* break in2 the nite */
    ChordPreset pset (QObject::tr ("Break In2 The Nite"));
    add_chords (
      pset,
      {
        { MusicalNote::D, ChordType::Minor },
        { MusicalNote::F, ChordType::Major },
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::D, ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Major, {}, -2 },
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::D, ChordType::Minor },
        { MusicalNote::G, ChordType::Minor },
        { MusicalNote::F, ChordType::Major },
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::A, ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- progressive trance --- */

  pack = std::make_unique<ChordPresetPack> (
    QObject::tr ("Progressive Trance"), true);

  {
    ChordPreset pset (QObject::tr ("Sajek Valley"));
    add_chords (
      pset,
      {
        { MusicalNote::A, ChordType::Minor },
        { MusicalNote::D, ChordType::Minor, ChordAccent::Seventh },
        { MusicalNote::F, ChordType::Major },
        { MusicalNote::C, ChordType::Major, {}, 1 },
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::D, ChordType::Minor },
        { MusicalNote::E, ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  packs_.push_back (std::move (pack));

  /* --- rock --- */

  pack = std::make_unique<ChordPresetPack> (QObject::tr ("Rock"), true);

  {
    ChordPreset pset (QObject::tr ("Overdrive"));
    add_chords (
      pset,
      {
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    /* kokoro */
    ChordPreset pset (QObject::tr ("Kokoro"));
    add_chords (
      pset,
      {
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::F,      ChordType::Minor },
        { MusicalNote::ASharp, ChordType::Minor },
        { MusicalNote::DSharp, ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::FSharp, ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    ChordPreset pset (QObject::tr ("Pray"));
    add_chords (
      pset,
      {
        { MusicalNote::B,      ChordType::Minor },
        { MusicalNote::G,      ChordType::Major },
        { MusicalNote::D,      ChordType::Major },
        { MusicalNote::A,      ChordType::Major },
        { MusicalNote::E,      ChordType::Major },
        { MusicalNote::B,      ChordType::Major },
        { MusicalNote::CSharp, ChordType::Minor },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Minor },
        { MusicalNote::FSharp, ChordType::Major },
        { MusicalNote::E,      ChordType::Major },
    });
    pack->add_preset (pset);
  }

  {
    /* no thank you */
    ChordPreset pset (QObject::tr ("No Thank You"));
    add_chords (
      pset,
      {
        { MusicalNote::E, ChordType::Minor },
        { MusicalNote::D, ChordType::Major },
        { MusicalNote::A, ChordType::Major },
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::A, ChordType::Minor },
        { MusicalNote::B, ChordType::Minor },
        { MusicalNote::C, ChordType::Major },
        { MusicalNote::D, ChordType::Major },
        { MusicalNote::G, ChordType::Major },
        { MusicalNote::B, ChordType::Minor },
        { MusicalNote::E, ChordType::Minor },
    });
    pack->add_preset (pset);
  }

  /* boulevard of broken dreams */
  {
    ChordPreset pset (QObject::tr ("Broken Dreams"));
    add_chords (
      pset,
      {
        { MusicalNote::F,      ChordType::Minor },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::ASharp, ChordType::Major },
        { MusicalNote::CSharp, ChordType::Major },
        { MusicalNote::GSharp, ChordType::Major },
        { MusicalNote::DSharp, ChordType::Major },
        { MusicalNote::F,      ChordType::Minor },
    });
    pack->add_preset (pset);
    packs_.push_back (std::move (pack));
  }
}

void
ChordPresetPackManager::add_user_packs ()
{
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      /* add user preset packs */
      const auto main_path = get_user_packs_path ();
      z_debug ("Reading user chord packs from {}...", main_path);

      std::vector<std::filesystem::path> pack_paths;
      if (fs::is_directory (main_path))
        {
          try
            {
              pack_paths = utils::io::get_files_in_dir_ending_in (
                main_path, true, u8".json");
            }
          catch (const ZrythmException &e)
            {
              z_warning (
                "Could not read user chord packs from {}: {}", main_path,
                e.what ());
            }
        }
      if (!pack_paths.empty ())
        {
          for (const auto &pack_path : pack_paths)
            {
              QFileInfo file_info (
                utils::Utf8String::from_path (pack_path).to_qstring ());
              if (!file_info.exists () || file_info.isDir ())
                {
                  continue;
                }

              z_debug ("checking file {}", pack_path);

              QFile f (file_info.absoluteFilePath ());
              auto  json = utils::Utf8String::from_qstring (f.readAll ());

              try
                {
                  auto           pack = std::make_unique<ChordPresetPack> ();
                  nlohmann::json j = nlohmann::json::parse (json.view ());
                  from_json (j, *pack);
                  packs_.push_back (std::move (pack));
                }
              catch (const ZrythmException &e)
                {
                  z_warning ("failed to load chord preset pack: {}", e.what ());
                }
            }
        }
      else
        {
          z_info ("no user chord presets found");
        }
    }
}

size_t
ChordPresetPackManager::get_num_packs () const
{
  return packs_.size ();
}

ChordPresetPack *
ChordPresetPackManager::get_pack_at (size_t idx)
{
  z_return_val_if_fail (idx < packs_.size (), nullptr);
  return packs_[idx].get ();
}

void
ChordPresetPackManager::add_pack (const ChordPresetPack &pack, bool _serialize)
{
  packs_.emplace_back (utils::clone_unique (pack));

  if (_serialize)
    {
      try
        {
          serialize ();
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to serialize chord preset packs");
        }
    }

  /* EVENTS_PUSH (EventType::ET_CHORD_PRESET_PACK_ADDED, nullptr); */
}

void
ChordPresetPackManager::delete_pack (const ChordPresetPack &pack, bool _serialize)
{
  auto it =
    std::find_if (packs_.begin (), packs_.end (), [&pack] (const auto &p) {
      return *p == pack;
    });
  if (it != packs_.end ())
    {
      packs_.erase (it);
    }

  if (_serialize)
    {
      try
        {
          serialize ();
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to serialize chord preset packs");
        }
    }

  /* EVENTS_PUSH (EventType::ET_CHORD_PRESET_PACK_REMOVED, nullptr); */
}

ChordPresetPack *
ChordPresetPackManager::get_pack_for_preset (const ChordPreset &pset)
{
  for (const auto &pack : packs_)
    {
      if (pack->contains_preset (pset))
        {
          return pack.get ();
        }
    }

  z_return_val_if_reached (nullptr);
}

int
ChordPresetPackManager::get_pack_index (const ChordPresetPack &pack) const
{
  auto it = std::ranges::find_if (packs_, [&pack] (const auto &p) {
    return *p == pack;
  });
  if (it != packs_.end ())
    return it - packs_.begin ();
  z_return_val_if_reached (-1);
}

int
ChordPresetPackManager::get_pset_index (const ChordPreset &pset)
{
  ChordPresetPack * pack = get_pack_for_preset (pset);
  z_return_val_if_fail (pack, -1);

  return pack->get_preset_index (pset);
}

void
ChordPresetPackManager::
  add_preset (ChordPresetPack &pack, const ChordPreset &pset, bool _serialize)
{
  pack.add_preset (pset);

  if (_serialize)
    {
      try
        {
          serialize ();
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to serialize chord preset packs");
        }
    }
}

void
ChordPresetPackManager::delete_preset (const ChordPreset &pset, bool _serialize)
{
  ChordPresetPack * pack = get_pack_for_preset (pset);
  if (pack == nullptr)
    return;

  pack->delete_preset (pset);

  if (_serialize)
    {
      try
        {
          serialize ();
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to serialize chord preset packs");
        }
    }
}

void
ChordPresetPackManager::serialize ()
{
  /* TODO backup existing packs first */

  z_debug ("Serializing user preset packs...");
  const auto main_path = get_user_packs_path ();
  z_return_if_fail (!main_path.empty ());
  z_debug ("Writing user chord packs to {}...", main_path);

  for (const auto &pack : packs_)
    {
      if (pack->is_standard_)
        continue;

      z_return_if_fail (!pack->name_.isEmpty ());

      const auto pack_dir =
        main_path / utils::Utf8String::from_qstring (pack->name_);
      const auto pack_path = pack_dir / UserPackJsonFilename;
      try
        {
          utils::io::mkdir (pack_dir);

          nlohmann::json pack_json = *pack;
          utils::io::set_file_contents (
            pack_path,
            utils::Utf8String::from_utf8_encoded_string (pack_json.dump ()));
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException (
            fmt::format ("Unable to write chord preset pack {}", pack_path));
        }
    }
}
