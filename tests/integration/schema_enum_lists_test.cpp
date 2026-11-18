// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "dsp/audio_port.h"
#include "dsp/chord_descriptor.h"
#include "dsp/curve.h"
#include "dsp/musical_scale.h"
#include "dsp/note_type.h"
#include "dsp/parameter.h"
#include "dsp/port_fwd.h"
#include "dsp/snap_grid.h"
#include "dsp/speaker_arrangement.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/timebase.h"
#include "dsp/timestretch_engine.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_protocol.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/midi_control_event.h"
#include "structure/tracks/automation_track.h"
#include "utils/serialization.h"

#include <gtest/gtest.h>

namespace
{

using nlohmann::json;

json
load_schema (std::string_view filename)
{
  const auto path =
    std::string (ZRYTHM_SCHEMAS_DIR) + "/" + std::string (filename);
  std::ifstream file (path);
  if (!file)
    {
      throw std::runtime_error ("cannot open schema file: " + path);
    }
  return json::parse (file);
}

template <typename E>
json
reflected_names ()
{
  auto names = json::array ();
  for (const auto name : magic_enum::enum_names<E> ())
    {
      names.push_back (std::string (name));
    }
  return names;
}

/**
 * Verifies that the schema node at @p pointer carries an "enum" listing
 * exactly the reflected enumerator names of @p E, in the same order.
 *
 * With ZRYTHM_DUMP_SCHEMA_ENUM_LISTS set, also prints one line per field
 * (file, pointer, JSON name array) to stdout for regenerating the lists.
 */
template <typename E>
void
check_enum_list (
  const json      &schema,
  std::string_view file,
  std::string_view pointer)
{
  const auto expected = reflected_names<E> ();
  const auto json_pointer = json::json_pointer (std::string (pointer));
  if (std::getenv ("ZRYTHM_DUMP_SCHEMA_ENUM_LISTS") != nullptr)
    {
      std::cout << file << '\t' << pointer << '\t' << expected.dump () << '\n';
    }
  ASSERT_TRUE (schema.contains (json_pointer))
    << "schema has no node at " << pointer;
  const auto &node = schema.at (json_pointer);
  ASSERT_TRUE (node.contains ("enum")) << "no enum list at " << pointer;
  EXPECT_EQ (node.at ("enum"), expected) << pointer;
}

} // namespace

TEST (SchemaEnumLists, CommonSchemaListsReflectedEnumeratorNames)
{
  const auto schema = load_schema ("common.schema.json");
  using namespace zrythm;
  constexpr auto file = "common.schema.json";

  check_enum_list<dsp::CurveOptions::Algorithm> (
    schema, file, "/definitions/curveOptions/properties/algorithm");
  check_enum_list<dsp::notes::NoteLength> (
    schema, file, "/definitions/noteLength");
  check_enum_list<dsp::notes::NoteType> (schema, file, "/definitions/noteType");
  check_enum_list<dsp::SnapGrid::NoteLengthType> (
    schema, file, "/definitions/snapGrid/properties/lengthType");
  check_enum_list<dsp::MusicalNote> (
    schema, file, "/definitions/chordDescriptor/properties/rootNote");
  check_enum_list<dsp::ChordType> (
    schema, file, "/definitions/chordDescriptor/properties/type");
  check_enum_list<dsp::ChordAccent> (
    schema, file, "/definitions/chordDescriptor/properties/accent");
  check_enum_list<dsp::MusicalNote> (
    schema, file, "/definitions/chordDescriptor/properties/bassNote");
  check_enum_list<dsp::PortFlow> (
    schema, file, "/definitions/portBase/allOf/1/properties/flow");
  check_enum_list<dsp::AudioPort::Purpose> (
    schema, file, "/definitions/audioPort/allOf/1/properties/purpose");
  check_enum_list<dsp::SpeakerArrangement::Kind> (
    schema, file,
    "/definitions/audioPort/allOf/1/properties/speakerArrangement/properties/kind");
  check_enum_list<dsp::SpeakerArrangement::AmbisonicOrdering> (
    schema, file,
    "/definitions/audioPort/allOf/1/properties/speakerArrangement/properties/"
    "ambisonicOrdering");
  check_enum_list<dsp::SpeakerArrangement::AmbisonicNormalization> (
    schema, file,
    "/definitions/audioPort/allOf/1/properties/speakerArrangement/properties/"
    "ambisonicNormalization");
  check_enum_list<dsp::PortType> (
    schema, file, "/definitions/channelSend/allOf/1/properties/signalType");
  check_enum_list<plugins::Protocol::ProtocolType> (
    schema, file, "/definitions/plugin/allOf/1/properties/protocol");
  check_enum_list<dsp::ParameterRange::Type> (
    schema, file, "/definitions/parameterRange/properties/type");
  check_enum_list<dsp::Timebase> (
    schema, file, "/definitions/trackBase/allOf/1/properties/timebase");
  check_enum_list<dsp::Timebase> (
    schema, file, "/definitions/arrangerObjectBase/allOf/1/properties/timebase");
  check_enum_list<structure::arrangement::MidiControlEvent::EventType> (
    schema, file,
    "/definitions/midiControlEvent/allOf/1/properties/controlType");
  check_enum_list<dsp::StretchOptions::Algorithm> (
    schema, file, "/definitions/audioClip/allOf/1/properties/stretchAlgorithm");
  check_enum_list<structure::arrangement::Marker::MarkerType> (
    schema, file, "/definitions/marker/allOf/1/properties/markerType");
  check_enum_list<dsp::MusicalScale::ScaleType> (
    schema, file,
    "/definitions/scaleObject/allOf/1/properties/scale/properties/type");
  check_enum_list<dsp::MusicalNote> (
    schema, file,
    "/definitions/scaleObject/allOf/1/properties/scale/properties/rootKey");
  check_enum_list<dsp::TempoEventWrapper::CurveType> (
    schema, file, "/definitions/tempoObject/allOf/1/properties/curveAlgorithm");
  check_enum_list<structure::tracks::AutomationTrack::AutomationMode> (
    schema, file, "/definitions/automationTrack/properties/automationMode");
  check_enum_list<structure::tracks::AutomationTrack::AutomationRecordMode> (
    schema, file, "/definitions/automationTrack/properties/recordMode");
}

TEST (SchemaEnumLists, KnownPluginsSchemaListsReflectedEnumeratorNames)
{
  const auto schema = load_schema ("known-plugins.schema.json");
  using namespace zrythm;
  constexpr auto file = "known-plugins.schema.json";

  check_enum_list<plugins::PluginCategory> (
    schema, file, "/properties/plugins/items/properties/category");
  check_enum_list<plugins::PluginArchitecture> (
    schema, file, "/properties/plugins/items/properties/architecture");
  check_enum_list<plugins::Protocol::ProtocolType> (
    schema, file, "/properties/plugins/items/properties/protocol");
  check_enum_list<plugins::Protocol::ProtocolType> (
    schema, file,
    "/properties/blacklistedIdentifiers/items/properties/protocol");
}
