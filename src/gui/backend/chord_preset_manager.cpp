// SPDX-FileCopyrightText: © 2022-2024, 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/chord_preset_manager.h"
#include "gui/backend/zrythm_application.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/io_utils.h"
#include "utils/qt.h"

#include <nlohmann/json.hpp>

using namespace zrythm;

ChordPresetManager::ChordPresetManager (QObject * parent)
    : QAbstractListModel (parent)
{
  add_builtin_presets ();
}

QHash<int, QByteArray>
ChordPresetManager::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[PresetRole] = "preset";
  roles[NameRole] = "name";
  roles[CategoryRole] = "category";
  roles[IsBuiltinRole] = "isBuiltin";
  return roles;
}

int
ChordPresetManager::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (presets_.size ());
}

QVariant
ChordPresetManager::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || index.row () >= static_cast<int> (presets_.size ()))
    return {};

  auto &preset = presets_.at (index.row ());

  switch (role)
    {
    case PresetRole:
      return QVariant::fromValue (preset.get ());
    case NameRole:
      return preset->name ();
    case CategoryRole:
      return preset->category ();
    case IsBuiltinRole:
      return preset->isBuiltin ();
    default:
      return {};
    }
}

QStringList
ChordPresetManager::categories () const
{
  QStringList cats;
  for (const auto &preset : presets_)
    {
      if (!cats.contains (preset->category ()))
        cats.push_back (preset->category ());
    }
  return cats;
}

QVariantList
ChordPresetManager::presetsInCategory (const QString &category) const
{
  QVariantList result;
  for (const auto &preset : presets_)
    {
      if (preset->category () == category)
        result.push_back (QVariant::fromValue (preset.get ()));
    }
  return result;
}

void
ChordPresetManager::add_builtin_presets ()
{
  using namespace zrythm::dsp;

  struct ChordDef
  {
    MusicalNote root;
    ChordType   type;
    ChordAccent accent = ChordAccent::None;
    int         inversion = 0;
  };

  const auto make_preset =
    [this] (
      const QString &name, const QString &category,
      std::initializer_list<ChordDef> chords) {
      auto pset = zrythm::utils::make_qobject_unique<ChordPreset> (
        name, category, true, this);
      for (const auto &def : chords)
        {
          auto descr = zrythm::utils::make_qobject_unique<ChordDescriptor> (
            def.root, def.type);
          if (def.accent != ChordAccent::None)
            descr->setChordAccent (def.accent);
          if (def.inversion != 0)
            descr->setInversion (def.inversion);
          pset->addDescriptor (std::move (descr));
        }
      presets_.push_back (std::move (pset));
    };

  const QString pop = QObject::tr ("Pop");

  make_preset (
    QObject::tr ("4-Chord Pop"), pop,
    {
      { MusicalNote::A, ChordType::Minor },
      { MusicalNote::C, ChordType::Major },
      { MusicalNote::F, ChordType::Major },
      { MusicalNote::G, ChordType::Major },
      { MusicalNote::G, ChordType::Major, ChordAccent::Seventh },
  });

  /* Source: Johann Pachelbel - Canon in D; also My Chemical Romance */
  make_preset (
    QObject::tr ("Canon Progression"), pop,
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

  make_preset (
    QObject::tr ("Love Progression"), pop,
    {
      { MusicalNote::C, ChordType::Major },
      { MusicalNote::A, ChordType::Minor },
      { MusicalNote::F, ChordType::Major },
      { MusicalNote::G, ChordType::Major },
  });

  make_preset (
    QObject::tr ("Pop Chords 1"), pop,
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

  make_preset (
    QObject::tr ("Common Pop Chords"), pop,
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

  const QString j_pop = QObject::tr ("J-Pop");

  /* Source: Namie Amuro - Fight Together (2011) */
  make_preset (
    QObject::tr ("J-Pop Uplifting"), j_pop,
    {
      { MusicalNote::G, ChordType::Major },
      { MusicalNote::A, ChordType::Major },
      { MusicalNote::D, ChordType::Major },
      { MusicalNote::G, ChordType::Major },
      { MusicalNote::A, ChordType::Major },
      { MusicalNote::B, ChordType::Minor },
      { MusicalNote::D, ChordType::Major },
  });

  /* Source: Yuruyuri OP - Yuriyurarararayuruyuri (Daijiken) */
  make_preset (
    QObject::tr ("J-Pop Modulating"), j_pop,
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

  /* Source: THE IDOLM@STER 2; the 王道進行 (Royal Road) progression */
  make_preset (
    QObject::tr ("Royal Road (IV-V-iii-vi)"), j_pop,
    {
      { MusicalNote::C, ChordType::Major },
      { MusicalNote::D, ChordType::Major },
      { MusicalNote::B, ChordType::Minor },
      { MusicalNote::E, ChordType::Minor },
  });

  const QString k_pop = QObject::tr ("K-Pop");

  /* Source: Girls' Generation (SNSD) - Gee (2009) */
  make_preset (
    QObject::tr ("K-Pop Bright"), k_pop,
    {
      { MusicalNote::A, ChordType::Major, ChordAccent::Seventh },
      { MusicalNote::FSharp, ChordType::Minor },
      { MusicalNote::GSharp, ChordType::Minor },
      { MusicalNote::GSharp, ChordType::Minor, ChordAccent::Seventh },
      { MusicalNote::CSharp, ChordType::Minor },
      { MusicalNote::CSharp, ChordType::Minor, ChordAccent::Seventh },
  });

  const QString anime_ballad = QObject::tr ("Anime Ballad");

  /* Source: Love Live! - Snow Halation */
  make_preset (
    QObject::tr ("Ballad — Bright Modulation"), anime_ballad,
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

  /* Source: ClariS - Connect (Madoka Magica OP, 2011) */
  make_preset (
    QObject::tr ("Ballad — Chromatic"), anime_ballad,
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

  /* Source: ZONE - Secret Base (AnoHana ED, 2011) */
  make_preset (
    QObject::tr ("Ballad — Nostalgic"), anime_ballad,
    {
      { MusicalNote::B, ChordType::Major },
      { MusicalNote::CSharp, ChordType::Major },
      { MusicalNote::DSharp, ChordType::Minor, ChordAccent::Seventh },
      { MusicalNote::CSharp, ChordType::Major },
      { MusicalNote::B, ChordType::Major },
      { MusicalNote::CSharp, ChordType::Major },
      { MusicalNote::FSharp, ChordType::Major },
  });

  const QString eurodance = QObject::tr ("Eurodance");

  /* Source: Haddaway - What Is Love (1993) */
  make_preset (
    QObject::tr ("Eurodance Anthem"), eurodance,
    {
      { MusicalNote::G, ChordType::Minor },
      { MusicalNote::ASharp, ChordType::Major },
      { MusicalNote::D, ChordType::Minor, ChordAccent::Seventh },
      { MusicalNote::F, ChordType::Major },
  });

  /* Source: Eiffel 65 - Blue (Da Ba Dee) (1999) */
  make_preset (
    QObject::tr ("Eurodance Minor"), eurodance,
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

  const QString eurobeat = QObject::tr ("Eurobeat");

  /* Source: samfree - Lilly Lilly Burning Night */
  make_preset (
    QObject::tr ("Eurobeat — Chromatic Minor"), eurobeat,
    {
      { MusicalNote::CSharp, ChordType::Major      },
      { MusicalNote::DSharp, ChordType::Major      },
      { MusicalNote::C,      ChordType::Minor      },
      { MusicalNote::F,      ChordType::Minor      },
      { MusicalNote::ASharp, ChordType::Minor      },
      { MusicalNote::B,      ChordType::Diminished },
      { MusicalNote::C,      ChordType::Major      },
  });

  /* Source: "Believe" / "Dreamin' Of You" eurobeat */
  make_preset (
    QObject::tr ("Eurobeat — Long Modulation"), eurobeat,
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

  /* Source: "Get Me Power" eurobeat */
  make_preset (
    QObject::tr ("Eurobeat — Modal Minor"), eurobeat,
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

  /* Source: Niko - Night of Fire */
  make_preset (
    QObject::tr ("Eurobeat — Minor Anthem"), eurobeat,
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

  /* Source: "Super Fever Night" eurobeat */
  make_preset (
    QObject::tr ("Eurobeat — Uplifting Major"), eurobeat,
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

  /* Source: "Break In2 The Nite" eurobeat */
  make_preset (
    QObject::tr ("Eurobeat — Minor Groove"), eurobeat,
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

  const QString trance = QObject::tr ("Trance");

  /* Source: "Sajek Valley" - progressive house (NCS?); origin uncertain */
  make_preset (
    QObject::tr ("Uplifting Trance"), trance,
    {
      { MusicalNote::A, ChordType::Minor },
      { MusicalNote::D, ChordType::Minor, ChordAccent::Seventh },
      { MusicalNote::F, ChordType::Major },
      { MusicalNote::C, ChordType::Major, {}, 1 },
      { MusicalNote::G, ChordType::Major },
      { MusicalNote::D, ChordType::Minor },
      { MusicalNote::E, ChordType::Minor },
  });

  const QString rock = QObject::tr ("Rock");

  /* Source: Harada Hitomi - Overdrive (anime) */
  make_preset (
    QObject::tr ("Overdrive"), rock,
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

  /* Source: Green Day - Boulevard of Broken Dreams */
  make_preset (
    QObject::tr ("Rock Ballad"), rock,
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

  const QString j_rock = QObject::tr ("J-Rock");

  /* Source: "Kokoro" */
  make_preset (
    QObject::tr ("J-Rock — Modal Mix"), j_rock,
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

  /* Source: Heavenly - Pray (Gintama) */
  make_preset (
    QObject::tr ("J-Rock — Cinematic"), j_rock,
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

  /* Source: "No Thank You" */
  make_preset (
    QObject::tr ("J-Rock — Diatonic Run"), j_rock,
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
}

std::filesystem::path
ChordPresetManager::get_user_presets_path ()
{
  auto zrythm_dir =
    dynamic_cast<gui::ZrythmApplication *> (qApp)
      ->get_directory_manager ()
      .get_dir (IDirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return zrythm_dir / "chord-presets";
}

void
ChordPresetManager::load_user_presets ()
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    return;

  const auto main_path = get_user_presets_path ();
  z_debug ("Reading user chord presets from {}...", main_path);

  std::vector<std::filesystem::path> preset_paths;
  if (fs::is_directory (main_path))
    {
      try
        {
          preset_paths =
            utils::io::get_files_in_dir_ending_in (main_path, true, u8".json");
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Could not read user chord presets from {}: {}", main_path,
            e.what ());
        }
    }

  if (preset_paths.empty ())
    {
      z_info ("no user chord presets found");
      return;
    }

  const auto start_row = static_cast<int> (presets_.size ());

  std::vector<utils::QObjectUniquePtr<ChordPreset>> loaded;
  for (const auto &preset_path : preset_paths)
    {
      QFileInfo file_info (
        utils::Utf8String::from_path (preset_path).to_qstring ());
      if (!file_info.exists () || file_info.isDir ())
        continue;

      z_debug ("checking file {}", preset_path);

      QFile f (file_info.absoluteFilePath ());
      if (!f.open (QIODevice::ReadOnly | QIODevice::Text))
        {
          z_warning ("failed to open chord preset file: {}", preset_path);
          continue;
        }

      auto json = utils::Utf8String::from_qstring (f.readAll ());

      auto pset = utils::make_qobject_unique<ChordPreset> (this);
      try
        {
          nlohmann::json j = nlohmann::json::parse (json.view ());
          from_json (j, *pset);
          loaded.push_back (std::move (pset));
        }
      catch (const std::exception &e)
        {
          z_warning ("failed to load chord preset: {}", e.what ());
        }
    }

  if (loaded.empty ())
    return;

  const auto end_row = start_row + static_cast<int> (loaded.size ()) - 1;
  beginInsertRows ({}, start_row, end_row);
  for (auto &pset : loaded)
    presets_.push_back (std::move (pset));
  endInsertRows ();
}
