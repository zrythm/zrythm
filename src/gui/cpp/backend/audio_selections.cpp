// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "gui/cpp/backend/audio_selections.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "project.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

AudioSelections::AudioSelections () : ArrangerSelections (Type::Audio) { }

void
AudioSelections::set_has_range (bool has_range)
{
  has_selection_ = true;

  EVENTS_PUSH (EventType::ET_AUDIO_SELECTIONS_RANGE_CHANGED, nullptr);
}

bool
AudioSelections::contains_clip (const AudioClip &clip) const
{
  return pool_id_ == clip.pool_id_;
}

bool
AudioSelections::can_be_pasted_at_impl (const Position pos, const int idx) const
{
  Region * r = CLIP_EDITOR->get_region ();

  if (!r || !r->is_audio ())
    return false;

  if (r->pos_.frames_ + pos.frames_ < 0)
    return false;

  /* TODO */
  return false;
}
