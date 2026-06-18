// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/chord_audition_state.h"
#include "dsp/chord_descriptor.h"
#include "dsp/midi_event.h"
#include "utils/qt.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class ChordAuditionStateTest : public ::testing::Test
{
protected:
  static dsp::graph::ProcessBlockInfo make_block_info (
    units::sample_u32_t offset = units::samples (0u),
    units::sample_u32_t nframes = units::samples (256u))
  {
    dsp::graph::ProcessBlockInfo info{};
    info.buffer_offset_ = offset;
    info.nframes_ = nframes;
    return info;
  }

  static auto collect_note_on_pitches (const MidiEventBuffer &buf)
  {
    auto pitches =
      buf | std::views::filter ([] (const auto &ev) {
        return utils::midi::midi_is_note_on (ev.data ());
      })
      | std::views::transform ([] (const auto &ev) {
          return utils::midi::midi_get_note_number (ev.data ());
        })
      | std::ranges::to<std::vector> ();
    std::ranges::sort (pitches);
    return pitches;
  }

  static auto collect_note_off_pitches (const MidiEventBuffer &buf)
  {
    auto pitches =
      buf | std::views::filter ([] (const auto &ev) {
        return utils::midi::midi_is_note_off (ev.data ());
      })
      | std::views::transform ([] (const auto &ev) {
          return utils::midi::midi_get_note_number (ev.data ());
        })
      | std::ranges::to<std::vector> ();
    std::ranges::sort (pitches);
    return pitches;
  }

  static auto collect_note_on_velocities (const MidiEventBuffer &buf)
  {
    return buf | std::views::filter ([] (const auto &ev) {
             return utils::midi::midi_is_note_on (ev.data ());
           })
           | std::views::transform ([] (const auto &ev) {
               return utils::midi::midi_get_velocity (ev.data ());
             })
           | std::ranges::to<std::vector> ();
  }

  auto make_chord (MusicalNote root, ChordType type)
  {
    auto desc = zrythm::utils::make_qobject_unique<ChordDescriptor> ();
    desc->setRootNote (root);
    desc->setChordType (type);
    return desc;
  }

  ChordAuditionState state_;
  MidiEventBuffer    out_ = MidiEventBuffer::make_reserved ();
};

TEST_F (ChordAuditionStateTest, StartThenProcessSendsNoteOns)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  auto ons = collect_note_on_pitches (out_);
  ASSERT_EQ (ons.size (), 3u);
  EXPECT_EQ (ons[0], 48);
  EXPECT_EQ (ons[1], 52);
  EXPECT_EQ (ons[2], 55);
  EXPECT_TRUE (collect_note_off_pitches (out_).empty ());
}

TEST_F (ChordAuditionStateTest, StartUsesCustomVelocity)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major, 80);
  state_.process (out_, make_block_info ());

  auto velocities = collect_note_on_velocities (out_);
  ASSERT_EQ (velocities.size (), 3u);
  EXPECT_EQ (velocities[0], 80);
  EXPECT_EQ (velocities[1], 80);
  EXPECT_EQ (velocities[2], 80);
}

TEST_F (ChordAuditionStateTest, StartUsesDefaultVelocity)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  auto velocities = collect_note_on_velocities (out_);
  ASSERT_EQ (velocities.size (), 3u);
  for (auto v : velocities)
    EXPECT_EQ (v, ChordAuditionState::kDefaultVelocity);
}

TEST_F (ChordAuditionStateTest, StopSendsNoteOffs)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  out_.clear ();
  state_.stop (*c_major);
  state_.process (out_, make_block_info ());

  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());
  auto offs = collect_note_off_pitches (out_);
  ASSERT_EQ (offs.size (), 3u);
  EXPECT_EQ (offs[0], 48);
  EXPECT_EQ (offs[1], 52);
  EXPECT_EQ (offs[2], 55);
}

TEST_F (ChordAuditionStateTest, StopByPitchesSendsNoteOffs)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  out_.clear ();
  // Stop using the pitches overload (the caller need not keep a descriptor).
  state_.stop (c_major->getMidiPitches ());
  state_.process (out_, make_block_info ());

  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());
  auto offs = collect_note_off_pitches (out_);
  ASSERT_EQ (offs.size (), 3u);
  EXPECT_EQ (offs[0], 48);
  EXPECT_EQ (offs[1], 52);
  EXPECT_EQ (offs[2], 55);
}

TEST_F (ChordAuditionStateTest, StopByPitchesNonMatchingIsNoOp)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());
  ASSERT_EQ (collect_note_on_pitches (out_).size (), 3u);

  out_.clear ();
  // D minor pitches do not match the active C major entry.
  auto d_minor = make_chord (MusicalNote::D, ChordType::Minor);
  state_.stop (d_minor->getMidiPitches ());
  state_.process (out_, make_block_info ());

  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());
  EXPECT_TRUE (collect_note_off_pitches (out_).empty ())
    << "Non-matching pitches must not stop the active chord";

  // The original chord is still active and can be stopped normally.
  out_.clear ();
  state_.stop (c_major->getMidiPitches ());
  state_.process (out_, make_block_info ());
  EXPECT_EQ (collect_note_off_pitches (out_).size (), 3u);
}

TEST_F (ChordAuditionStateTest, NoOpProcessSendsNothing)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  out_.clear ();
  state_.process (out_, make_block_info ());

  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());
  EXPECT_TRUE (collect_note_off_pitches (out_).empty ());
}

TEST_F (ChordAuditionStateTest, ChangeChordSendsOffsThenOns)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  auto d_minor = make_chord (MusicalNote::D, ChordType::Minor);

  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  out_.clear ();
  state_.stop (*c_major);
  state_.start (*d_minor);
  state_.process (out_, make_block_info ());

  auto offs = collect_note_off_pitches (out_);
  auto ons = collect_note_on_pitches (out_);
  ASSERT_EQ (offs.size (), 3u);
  ASSERT_EQ (ons.size (), 3u);
  EXPECT_EQ (ons[0], 50);
  EXPECT_EQ (ons[1], 53);
  EXPECT_EQ (ons[2], 57);
}

TEST_F (ChordAuditionStateTest, OverlappingChordsSharePitches)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  // C major = [48, 52, 55], C7 = [48, 52, 55, 58] — shares all C major pitches
  auto c_dom7 = make_chord (MusicalNote::C, ChordType::Major);
  c_dom7->setChordAccent (ChordAccent::Seventh);

  state_.start (*c_major);
  state_.start (*c_dom7);
  state_.process (out_, make_block_info ());

  // Merged: [48, 52, 55, 58] — shared pitches counted twice but deduped
  auto ons = collect_note_on_pitches (out_);
  ASSERT_EQ (ons.size (), 4u);
  EXPECT_EQ (ons[3], 58);

  out_.clear ();
  // Stop one chord — shared pitches (48, 52, 55) must NOT get note-offs
  state_.stop (*c_major);
  state_.process (out_, make_block_info ());

  auto offs = collect_note_off_pitches (out_);
  EXPECT_TRUE (offs.empty ())
    << "Shared pitches should not be cut off when one chord stops";

  out_.clear ();
  // Stop the other — now everything stops
  state_.stop (*c_dom7);
  state_.process (out_, make_block_info ());

  offs = collect_note_off_pitches (out_);
  ASSERT_EQ (offs.size (), 4u);
}

TEST_F (ChordAuditionStateTest, StopAllCutsEverything)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  auto d_minor = make_chord (MusicalNote::D, ChordType::Minor);

  state_.start (*c_major);
  state_.start (*d_minor);
  state_.process (out_, make_block_info ());

  out_.clear ();
  state_.stopAll ();
  state_.process (out_, make_block_info ());

  auto offs = collect_note_off_pitches (out_);
  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());
  EXPECT_GT (offs.size (), 0u);
}

TEST_F (ChordAuditionStateTest, DuplicateStartRequiresDuplicateStop)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.start (*c_major);
  state_.start (*c_major);
  state_.process (out_, make_block_info ());
  EXPECT_EQ (collect_note_on_pitches (out_).size (), 3u);

  out_.clear ();
  state_.stop (*c_major);
  state_.process (out_, make_block_info ());
  EXPECT_TRUE (collect_note_off_pitches (out_).empty ())
    << "Shared pitches should not be cut off when one duplicate stops";

  out_.clear ();
  state_.stop (*c_major);
  state_.process (out_, make_block_info ());
  EXPECT_EQ (collect_note_off_pitches (out_).size (), 3u);
}

TEST_F (ChordAuditionStateTest, StopWithoutStartIsNoOp)
{
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  state_.stop (*c_major);
  state_.process (out_, make_block_info ());
  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());
  EXPECT_TRUE (collect_note_off_pitches (out_).empty ());
}

TEST_F (ChordAuditionStateTest, NoneChordTypeIsNoOp)
{
  ChordDescriptor none_chord;
  none_chord.setChordType (ChordType::None);
  state_.start (none_chord);
  state_.process (out_, make_block_info ());
  EXPECT_TRUE (collect_note_on_pitches (out_).empty ());

  state_.stop (none_chord);
  state_.process (out_, make_block_info ());
  EXPECT_TRUE (collect_note_off_pitches (out_).empty ());
}

TEST_F (ChordAuditionStateTest, PartialOverlapRetriggersAll)
{
  // C major = [48, 52, 55], E minor = [52, 55, 59] — shares [52, 55]
  auto c_major = make_chord (MusicalNote::C, ChordType::Major);
  auto e_minor = make_chord (MusicalNote::E, ChordType::Minor);

  state_.start (*c_major);
  state_.process (out_, make_block_info ());

  auto ons = collect_note_on_pitches (out_);
  ASSERT_EQ (ons.size (), 3u);

  out_.clear ();
  state_.start (*e_minor);
  state_.process (out_, make_block_info ());

  auto offs = collect_note_off_pitches (out_);
  ons = collect_note_on_pitches (out_);

  // Retrigger: note-offs for [48,52,55], note-ons for merged [48,52,55,59]
  ASSERT_EQ (offs.size (), 3u);
  ASSERT_EQ (ons.size (), 4u);
  EXPECT_EQ (ons[0], 48);
  EXPECT_EQ (ons[1], 52);
  EXPECT_EQ (ons[2], 55);
  EXPECT_EQ (ons[3], 59);

  out_.clear ();
  // Stop C major — remaining pitches: E minor [52, 55, 59]
  // Retrigger: note-offs for [48,52,55,59], note-ons for [52,55,59]
  // Shared pitches (52, 55) are retrigged; unique pitch 48 is removed.
  state_.stop (*c_major);
  state_.process (out_, make_block_info ());

  offs = collect_note_off_pitches (out_);
  ons = collect_note_on_pitches (out_);
  ASSERT_EQ (offs.size (), 4u);
  ASSERT_EQ (ons.size (), 3u);
  EXPECT_EQ (ons[0], 52);
  EXPECT_EQ (ons[1], 55);
  EXPECT_EQ (ons[2], 59);
}

TEST_F (ChordAuditionStateTest, ActivePitchesEquality)
{
  ChordAuditionState::ActivePitches a{};
  a.push_back ({ 48, 100 });
  a.push_back ({ 52, 100 });

  ChordAuditionState::ActivePitches b{};
  b.push_back ({ 48, 100 });
  b.push_back ({ 52, 100 });

  EXPECT_EQ (a, b);

  b[1].pitch = 55;
  EXPECT_NE (a, b);

  ChordAuditionState::ActivePitches empty{};
  EXPECT_NE (a, empty);
  EXPECT_EQ (empty, empty);
}

} // namespace zrythm::dsp
