// SPDX-FileCopyrightText: © 2022-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <glibmm.h>

std::string
ChordPresetPackManager::get_user_packs_path ()
{
  auto *      dir_mgr = ZrythmDirectoryManager::getInstance ();
  std::string zrythm_dir = dir_mgr->get_dir (ZrythmDirType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return Glib::build_filename (zrythm_dir, UserPacksDirName);
}

void
ChordPresetPackManager::add_standard_packs ()
{
  packs_.reserve (100);
#define ADD_SIMPLE_CHORD(root, chord_type) \
  pset.descr_.emplace_back ( \
    root, false, root, chord_type, ChordAccent::None, 0);

#define ADD_SIMPLE_CHORDS( \
  n0, t0, n1, t1, n2, t2, n3, t3, n4, t4, n5, t5, n6, t6, n7, t7, n8, t8, n9, \
  t9, n10, t10, n11, t11) \
  ADD_SIMPLE_CHORD (n0, t0); \
  ADD_SIMPLE_CHORD (n1, t1); \
  ADD_SIMPLE_CHORD (n2, t2); \
  ADD_SIMPLE_CHORD (n3, t3); \
  ADD_SIMPLE_CHORD (n4, t4); \
  ADD_SIMPLE_CHORD (n5, t5); \
  ADD_SIMPLE_CHORD (n6, t6); \
  ADD_SIMPLE_CHORD (n7, t7); \
  ADD_SIMPLE_CHORD (n8, t8); \
  ADD_SIMPLE_CHORD (n9, t9); \
  ADD_SIMPLE_CHORD (n10, t10); \
  ADD_SIMPLE_CHORD (n11, t11)

#define ADD_SIMPLE_4CHORDS(n0, t0, n1, t1, n2, t2, n3, t3) \
  ADD_SIMPLE_CHORDS ( \
    n0, t0, n1, t1, n2, t2, n3, t3, MusicalNote::C, ChordType::None, \
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None, \
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None, \
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None, \
    MusicalNote::C, ChordType::None)

  /* --- euro pop pack --- */

  auto pack = std::make_unique<ChordPresetPack> (_ ("Euro Pop"), true);

  auto pset = ChordPreset (_ ("4 Chord Song"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::A, ChordType::Minor, MusicalNote::C, ChordType::Major,
    MusicalNote::F, ChordType::Major, MusicalNote::G, ChordType::Major,
    MusicalNote::G, ChordType::Major, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pset.descr_[4].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  /* Johann Pachelbel, My Chemical Romance */
  pset = ChordPreset (_ ("Canon in D"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::D, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::B, ChordType::Minor, MusicalNote::FSharp, ChordType::Minor,
    MusicalNote::G, ChordType::Major, MusicalNote::D, ChordType::Major,
    MusicalNote::G, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  pset = ChordPreset (_ ("Love Progression"));
  ADD_SIMPLE_4CHORDS (
    MusicalNote::C, ChordType::Major, MusicalNote::A, ChordType::Minor,
    MusicalNote::F, ChordType::Major, MusicalNote::G, ChordType::Major);
  pack->add_preset (pset);

  pset = ChordPreset (_ ("Pop Chords 1"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::C, ChordType::Major, MusicalNote::G, ChordType::Major,
    MusicalNote::A, ChordType::Minor, MusicalNote::F, ChordType::Major,
    MusicalNote::E, ChordType::Major, MusicalNote::B, ChordType::Major,
    MusicalNote::CSharp, ChordType::Minor, MusicalNote::A, ChordType::Major,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  pset = ChordPreset (_ ("Most Often Used Chords"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::G, ChordType::Major, MusicalNote::F, ChordType::Major,
    MusicalNote::C, ChordType::Major, MusicalNote::A, ChordType::Minor,
    MusicalNote::D, ChordType::Minor, MusicalNote::E, ChordType::Minor,
    MusicalNote::E, ChordType::Major, MusicalNote::D, ChordType::Major,
    MusicalNote::ASharp, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- j/k pop --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Eastern Pop"), true);

  pset = ChordPreset (_ ("Fight Together"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::G, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::D, ChordType::Major, MusicalNote::G, ChordType::Major,
    MusicalNote::A, ChordType::Major, MusicalNote::B, ChordType::Minor,
    MusicalNote::D, ChordType::Major, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  pset = ChordPreset (_ ("Gee"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::A, ChordType::Major, MusicalNote::FSharp, ChordType::Minor,
    MusicalNote::GSharp, ChordType::Minor, MusicalNote::GSharp, ChordType::Minor,
    MusicalNote::CSharp, ChordType::Minor, MusicalNote::CSharp, ChordType::Minor,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pset.descr_[0].accent_ = ChordAccent::Seventh;
  pset.descr_[3].accent_ = ChordAccent::Seventh;
  pset.descr_[5].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  /* yuriyurarararayuruyuri */
  pset = ChordPreset (_ ("Daijiken"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::DSharp, ChordType::Major, MusicalNote::ASharp,
    ChordType::Minor, MusicalNote::F, ChordType::Major, MusicalNote::ASharp,
    ChordType::Minor, MusicalNote::GSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Major, MusicalNote::G, ChordType::Minor,
    MusicalNote::F, ChordType::Minor, MusicalNote::ASharp, ChordType::Major,
    MusicalNote::GSharp, ChordType::Major, MusicalNote::C, ChordType::Minor,
    MusicalNote::C, ChordType::Major);
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- dance --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Dance"), true);

  /* the idolm@ster 2 */
  pset = ChordPreset (_ ("Idol 2"));
  ADD_SIMPLE_4CHORDS (
    MusicalNote::C, ChordType::Major, MusicalNote::D, ChordType::Major,
    MusicalNote::B, ChordType::Minor, MusicalNote::E, ChordType::Minor);
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- ballad --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Ballad"), true);

  /* snow halation */
  pset = ChordPreset (_ ("Snow Halation"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::D, ChordType::Major, MusicalNote::E, ChordType::Major,
    MusicalNote::CSharp, ChordType::Minor, MusicalNote::FSharp, ChordType::Minor,
    MusicalNote::B, ChordType::Minor, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::E, ChordType::Major, MusicalNote::CSharp, ChordType::Minor,
    MusicalNote::CSharp, ChordType::Major, MusicalNote::B, ChordType::Major,
    MusicalNote::E, ChordType::Major, MusicalNote::A, ChordType::Major);
  pset.descr_[4].accent_ = ChordAccent::Seventh;
  pset.descr_[5].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  /* connect */
  pset = ChordPreset (_ ("Connect"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::B, ChordType::Major, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::ASharp, ChordType::Minor, MusicalNote::DSharp,
    ChordType::Minor, MusicalNote::GSharp, ChordType::Minor, MusicalNote::B,
    ChordType::Major, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Major, MusicalNote::GSharp,
    ChordType::Major, MusicalNote::ASharp, ChordType::Major, MusicalNote::G,
    ChordType::Major, MusicalNote::C, ChordType::Minor);
  pset.descr_[8].accent_ = ChordAccent::Seventh;
  pset.descr_[10].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  /* secret base */
  pset = ChordPreset (_ ("Secret Base"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::B, ChordType::Major, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Minor, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::B, ChordType::Major, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::FSharp, ChordType::Major, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pset.descr_[2].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- eurodance --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Eurodance"), true);

  /* what is love */
  pset = ChordPreset (_ ("What is Love"));
  ADD_SIMPLE_4CHORDS (
    MusicalNote::G, ChordType::Minor, MusicalNote::ASharp, ChordType::Major,
    MusicalNote::D, ChordType::Minor, MusicalNote::F, ChordType::Major);
  pset.descr_[2].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  /* blue */
  pset = ChordPreset (_ ("Blue"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::G, ChordType::Minor, MusicalNote::F, ChordType::Major,
    MusicalNote::DSharp, ChordType::Major, MusicalNote::C, ChordType::Minor,
    MusicalNote::C, ChordType::Major, MusicalNote::D, ChordType::Minor,
    MusicalNote::GSharp, ChordType::Major, MusicalNote::ASharp, ChordType::Major,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- eurobeat --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Eurobeat"), true);

  pset = ChordPreset (_ ("Burning Night"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::CSharp, ChordType::Major, MusicalNote::DSharp, ChordType::Major,
    MusicalNote::C, ChordType::Minor, MusicalNote::F, ChordType::Minor,
    MusicalNote::ASharp, ChordType::Minor, MusicalNote::B, ChordType::Diminished,
    MusicalNote::C, ChordType::Major, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  /* believe / dreamin' of you */
  pset = ChordPreset (_ ("Dreamin' Of You"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::F, ChordType::Major, MusicalNote::C, ChordType::Major,
    MusicalNote::D, ChordType::Minor, MusicalNote::ASharp, ChordType::Major,
    MusicalNote::G, ChordType::Minor, MusicalNote::A, ChordType::Major,
    MusicalNote::G, ChordType::Major, MusicalNote::D, ChordType::Major,
    MusicalNote::FSharp, ChordType::Minor, MusicalNote::B, ChordType::Minor,
    MusicalNote::E, ChordType::Minor, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  /* get me power */
  pset = ChordPreset (_ ("Get Me Power"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::B, ChordType::Minor, MusicalNote::E, ChordType::Minor,
    MusicalNote::D, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::G, ChordType::Major, MusicalNote::C, ChordType::Major,
    MusicalNote::FSharp, ChordType::Major, MusicalNote::A, ChordType::Minor,
    MusicalNote::D, ChordType::Minor, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  /* night of fire */
  pset = ChordPreset (_ ("Night of Fire"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::DSharp, ChordType::Minor, MusicalNote::B, ChordType::Major,
    MusicalNote::CSharp, ChordType::Major, MusicalNote::FSharp,
    ChordType::Major, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Minor, MusicalNote::FSharp,
    ChordType::Major, MusicalNote::GSharp, ChordType::Major, MusicalNote::B,
    ChordType::Major, MusicalNote::GSharp, ChordType::Minor, MusicalNote::ASharp,
    ChordType::Minor, MusicalNote::CSharp, ChordType::Major);
  pack->add_preset (pset);

  /* super fever night */
  pset = ChordPreset (_ ("Super Fever Night"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::B, ChordType::Minor, MusicalNote::G, ChordType::Major,
    MusicalNote::A, ChordType::Major, MusicalNote::B, ChordType::Minor,
    MusicalNote::G, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::D, ChordType::Major, MusicalNote::E, ChordType::Major,
    MusicalNote::FSharp, ChordType::Minor, MusicalNote::FSharp, ChordType::Major,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pset.descr_[4].accent_ = ChordAccent::Seventh;
  pack->add_preset (pset);

  /* break in2 the nite */
  pset = ChordPreset (_ ("Break In2 The Nite"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::D, ChordType::Minor, MusicalNote::F, ChordType::Major,
    MusicalNote::C, ChordType::Major, MusicalNote::D, ChordType::Minor,
    MusicalNote::ASharp, ChordType::Major, MusicalNote::C, ChordType::Major,
    MusicalNote::D, ChordType::Minor, MusicalNote::G, ChordType::Minor,
    MusicalNote::F, ChordType::Major, MusicalNote::C, ChordType::Major,
    MusicalNote::A, ChordType::Minor, MusicalNote::C, ChordType::None);
  pset.descr_[4].inversion_ = -2;
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- progressive trance --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Progressive Trance"), true);

  pset = ChordPreset (_ ("Sajek Valley"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::A, ChordType::Minor, MusicalNote::D, ChordType::Minor,
    MusicalNote::F, ChordType::Major, MusicalNote::C, ChordType::Major,
    MusicalNote::G, ChordType::Major, MusicalNote::D, ChordType::Minor,
    MusicalNote::E, ChordType::Minor, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pset.descr_[1].accent_ = ChordAccent::Seventh;
  pset.descr_[3].inversion_ = 1;
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

  /* --- rock --- */

  pack = std::make_unique<ChordPresetPack> (_ ("Rock"), true);

  pset = ChordPreset (_ ("Overdrive"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::FSharp, ChordType::Major, MusicalNote::GSharp,
    ChordType::Major, MusicalNote::ASharp, ChordType::Minor,
    MusicalNote::ASharp, ChordType::Major, MusicalNote::FSharp,
    ChordType::Major, MusicalNote::GSharp, ChordType::Major,
    MusicalNote::ASharp, ChordType::Minor, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::GSharp, ChordType::Major, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  /* kokoro */
  pset = ChordPreset (_ ("Kokoro"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::FSharp, ChordType::Major, MusicalNote::F, ChordType::Minor,
    MusicalNote::ASharp, ChordType::Minor, MusicalNote::DSharp,
    ChordType::Minor, MusicalNote::GSharp, ChordType::Major,
    MusicalNote::CSharp, ChordType::Major, MusicalNote::FSharp,
    ChordType::Major, MusicalNote::FSharp, ChordType::Major, MusicalNote::C,
    ChordType::None, MusicalNote::C, ChordType::None, MusicalNote::C,
    ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  pset = ChordPreset (_ ("Pray"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::B, ChordType::Minor, MusicalNote::G, ChordType::Major,
    MusicalNote::D, ChordType::Major, MusicalNote::A, ChordType::Major,
    MusicalNote::E, ChordType::Major, MusicalNote::B, ChordType::Major,
    MusicalNote::CSharp, ChordType::Minor, MusicalNote::CSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Major, MusicalNote::GSharp, ChordType::Minor,
    MusicalNote::FSharp, ChordType::Major, MusicalNote::E, ChordType::Major);
  pack->add_preset (pset);

  /* no thank you */
  pset = ChordPreset (_ ("No Thank You"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::E, ChordType::Minor, MusicalNote::D, ChordType::Major,
    MusicalNote::A, ChordType::Major, MusicalNote::C, ChordType::Major,
    MusicalNote::G, ChordType::Major, MusicalNote::A, ChordType::Minor,
    MusicalNote::B, ChordType::Minor, MusicalNote::C, ChordType::Major,
    MusicalNote::D, ChordType::Major, MusicalNote::G, ChordType::Major,
    MusicalNote::B, ChordType::Minor, MusicalNote::E, ChordType::Minor);
  pack->add_preset (pset);

  /* boulevard of broken dreams */
  pset = ChordPreset (_ ("Broken Dreams"));
  ADD_SIMPLE_CHORDS (
    MusicalNote::F, ChordType::Minor, MusicalNote::GSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Major, MusicalNote::ASharp, ChordType::Major,
    MusicalNote::CSharp, ChordType::Major, MusicalNote::GSharp, ChordType::Major,
    MusicalNote::DSharp, ChordType::Major, MusicalNote::F, ChordType::Minor,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None,
    MusicalNote::C, ChordType::None, MusicalNote::C, ChordType::None);
  pack->add_preset (pset);

  packs_.push_back (std::move (pack));

#undef ADD_SIMPLE_CHORD
#undef ADD_SIMPLE_CHORDS
#undef ADD_SIMPLE_4CHORDS
}

void
ChordPresetPackManager::add_user_packs ()
{
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      /* add user preset packs */
      std::string main_path = get_user_packs_path ();
      z_debug ("Reading user chord packs from {}...", main_path);

      StringArray pack_paths;
      try
        {
          pack_paths = io_get_files_in_dir_ending_in (main_path, true, ".json");
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Could not read user chord packs from {}: {}", main_path, e.what ());
        }
      if (!pack_paths.isEmpty ())
        {
          for (const auto &pack_path : pack_paths)
            {
              if (
                !Glib::file_test (
                  pack_path.toStdString (), Glib::FileTest::EXISTS)
                || Glib::file_test (
                  pack_path.toStdString (), Glib::FileTest::IS_DIR))
                {
                  continue;
                }

              z_debug ("checking file {}", pack_path.toStdString ());

              std::string json;
              try
                {
                  Glib::file_get_contents (pack_path.toStdString ());
                }
              catch (Glib::FileError &err)
                {
                  z_warning (
                    "Failed to read json from %s", pack_path.toStdString ());
                  continue;
                }

              try
                {
                  auto pack = std::make_unique<ChordPresetPack> ();
                  pack->deserialize_from_json_string (json.c_str ());
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
  packs_.emplace_back (pack.clone_unique ());

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

  EVENTS_PUSH (EventType::ET_CHORD_PRESET_PACK_ADDED, nullptr);
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

  EVENTS_PUSH (EventType::ET_CHORD_PRESET_PACK_REMOVED, nullptr);
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
  auto it =
    std::find_if (packs_.begin (), packs_.end (), [&pack] (const auto &p) {
      return *p == pack;
    });
  if (it != packs_.end ())
    return it - packs_.begin ();
  else
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
  if (!pack)
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
  std::string main_path = get_user_packs_path ();
  z_return_if_fail (!main_path.empty () && main_path.length () > 2);
  z_debug ("Writing user chord packs to {}...", main_path);

  for (const auto &pack : packs_)
    {
      if (pack->is_standard_)
        continue;

      z_return_if_fail (!pack->name_.empty ());

      std::string pack_dir = Glib::build_filename (main_path, pack->name_);
      std::string pack_path =
        Glib::build_filename (pack_dir, UserPackJsonFilename);
      try
        {
          io_mkdir (pack_dir);

          auto pack_json = pack->serialize_to_json_string ();
          io_write_file_atomic (pack_path, pack_json.c_str ());
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException (
            fmt::format ("Unable to write chord preset pack {}", pack_path));
        }
    }
}
