// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/chord_object.h"
#include "gui/cpp/gtk_widgets/chord_selector_window.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/timeline_arranger.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/chord_descriptor.h"
#include "common/dsp/chord_object.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/scale_object.h"
#include "common/dsp/tracklist.h"
#include "common/utils/flags.h"
#include "common/utils/resources.h"
#include "common/utils/rt_thread_id.h"

G_DEFINE_TYPE (
  ChordSelectorWindowWidget,
  chord_selector_window_widget,
  ADW_TYPE_DIALOG)

static void
on_closed (AdwDialog * dialog, ChordSelectorWindowWidget * self)
{
  self->descr_clone->update_notes ();

  /* if chord changed, perform undoable action */
  if (*self->descr_clone == CHORD_EDITOR->chords_[self->chord_idx])
    {
      z_debug ("no chord change");
    }
  else
    {
      CHORD_EDITOR->apply_single_chord (
        *self->descr_clone, self->chord_idx, F_UNDOABLE);

      EVENTS_PUSH (
        EventType::ET_CHORD_KEY_CHANGED,
        &CHORD_EDITOR->chords_[self->chord_idx]);
    }

  delete self->descr_clone;
  self->descr_clone = nullptr;
}

/**
 * Returns the MusicalNote corresponding to the
 * given GtkFlowBoxChild.
 */
static MusicalNote
get_note_from_creator_root_notes (
  ChordSelectorWindowWidget * self,
  GtkFlowBoxChild *           child)
{
  for (int i = 0; i < 12; i++)
    {
      if (self->creator_root_notes[i] == child)
        return (MusicalNote) i;
    }
  z_return_val_if_reached (ENUM_INT_TO_VALUE (MusicalNote, 0));
}

static ChordType
get_type_from_creator_types (
  ChordSelectorWindowWidget * self,
  GtkFlowBoxChild *           child)
{
  for (
    int i = ENUM_VALUE_TO_INT (ChordType::Major);
    i <= ENUM_VALUE_TO_INT (ChordType::Augmented); i++)
    {
      if (self->creator_types[i - ENUM_VALUE_TO_INT (ChordType::Major)] == child)
        {
          return ENUM_INT_TO_VALUE (ChordType, i);
        }
    }
  z_return_val_if_reached (ENUM_INT_TO_VALUE (ChordType, 0));
}

/**
 * Returns the currently selected root note, or
 * -1 if no selection.
 */
static int
get_selected_root_note (ChordSelectorWindowWidget * self)
{
  int     note = -1;
  GList * list =
    gtk_flow_box_get_selected_children (self->creator_root_note_flowbox);

  if (list)
    {
      GtkFlowBoxChild * selected_root_note =
        GTK_FLOW_BOX_CHILD (g_list_first (list)->data);
      if (selected_root_note)
        note = (int) get_note_from_creator_root_notes (self, selected_root_note);
      g_list_free (list);
    }

  return note;
}

/**
 * Returns the currently selected chord type, or
 * -1 if no selection.
 */
static int
get_selected_chord_type (ChordSelectorWindowWidget * self)
{
  int type = -1;

  GList * list = gtk_flow_box_get_selected_children (self->creator_type_flowbox);
  if (list)
    {
      GtkFlowBoxChild * selected_type =
        GTK_FLOW_BOX_CHILD (g_list_first (list)->data);
      if (selected_type)
        type = (int) get_type_from_creator_types (self, selected_type);
      g_list_free (list);
    }

  return type;
}

static void
creator_select_root_note (
  GtkFlowBox *                box,
  GtkFlowBoxChild *           child,
  ChordSelectorWindowWidget * self)
{
  self->descr_clone->root_note_ = get_note_from_creator_root_notes (self, child);
}

static void
creator_select_type (
  GtkFlowBox *                box,
  GtkFlowBoxChild *           child,
  ChordSelectorWindowWidget * self)
{
  for (
    int i = ENUM_VALUE_TO_INT (ChordType::Major);
    i <= ENUM_VALUE_TO_INT (ChordType::Augmented); i++)
    {
      if (self->creator_types[i - ENUM_VALUE_TO_INT (ChordType::Major)] != child)
        continue;

      self->descr_clone->type_ = ENUM_INT_TO_VALUE (ChordType, i);
    }
}

static void
creator_select_bass_note (
  GtkFlowBox *                box,
  GtkFlowBoxChild *           child,
  ChordSelectorWindowWidget * self)
{
  for (int i = 0; i < 12; i++)
    {
      if (self->creator_bass_notes[i] != child)
        continue;

      self->descr_clone->has_bass_ = true;
      self->descr_clone->bass_note_ = ENUM_INT_TO_VALUE (MusicalNote, i);
      z_debug ("bass note {}", ENUM_NAME_FROM_INT (MusicalNote, i));
      return;
    }

  z_return_if_reached ();
}

static void
on_creator_root_note_selected_children_changed (
  GtkFlowBox *                flowbox,
  ChordSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox, (GtkFlowBoxForeachFunc) creator_select_root_note, self);
  if (gtk_check_button_get_active (self->creator_visibility_in_scale))
    {
      gtk_flow_box_unselect_all (self->creator_type_flowbox);
      gtk_flow_box_unselect_all (self->creator_accent_flowbox);
      gtk_flow_box_unselect_all (self->creator_bass_note_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_type_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_accent_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_bass_note_flowbox);
    }
}

static void
on_creator_type_selected_children_changed (
  GtkFlowBox *                flowbox,
  ChordSelectorWindowWidget * self)
{
  gtk_flow_box_selected_foreach (
    flowbox, (GtkFlowBoxForeachFunc) creator_select_type, self);

  if (gtk_check_button_get_active (self->creator_visibility_in_scale))
    {
      gtk_flow_box_unselect_all (self->creator_accent_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_accent_flowbox);
    }
}

static void
on_creator_accent_child_activated (
  GtkFlowBox *                flowbox,
  GtkFlowBoxChild *           child,
  ChordSelectorWindowWidget * self)
{
  for (unsigned int i = 0; i < ENUM_COUNT (ChordAccent); i++)
    {
      if (self->creator_accents[i] != child)
        continue;

      ChordAccent accent = (ChordAccent) (i + 1);

      /* if selected, deselect it */
      if (self->descr_clone->accent_ == accent)
        {
          gtk_flow_box_unselect_child (flowbox, child);
          self->descr_clone->accent_ = ChordAccent::None;
        }
      /* else select it */
      else
        {
          self->descr_clone->accent_ = accent;
        }
    }
}

static void
on_creator_bass_note_selected_children_changed (
  GtkFlowBox *                flowbox,
  ChordSelectorWindowWidget * self)
{
  GList * list = gtk_flow_box_get_selected_children (flowbox);
  if (list)
    {
      gtk_flow_box_selected_foreach (
        flowbox, (GtkFlowBoxForeachFunc) creator_select_bass_note, self);
      g_list_free (list);
    }
  else
    {
      z_debug ("removing bass");
      self->descr_clone->has_bass_ = false;
    }
}

static void
setup_creator_tab (ChordSelectorWindowWidget * self)
{
  auto &descr = self->descr_clone;

#define SELECT_CHILD(flowbox, child) \
  gtk_flow_box_select_child ( \
    GTK_FLOW_BOX (self->flowbox), GTK_FLOW_BOX_CHILD (self->child))

  /* set root note */
  SELECT_CHILD (
    creator_root_note_flowbox,
    creator_root_notes[ENUM_VALUE_TO_INT (descr->root_note_)]);

#define SELECT_CHORD_TYPE(uppercase, lowercase) \
  case ChordType::uppercase: \
    SELECT_CHILD (creator_type_flowbox, creator_type_##lowercase); \
    break

  /* set chord type */
  switch (descr->type_)
    {
      SELECT_CHORD_TYPE (Major, maj);
      SELECT_CHORD_TYPE (Minor, min);
      SELECT_CHORD_TYPE (Diminished, dim);
      SELECT_CHORD_TYPE (SuspendedFourth, sus4);
      SELECT_CHORD_TYPE (SuspendedSecond, sus2);
      SELECT_CHORD_TYPE (Augmented, aug);
    default:
      /* TODO */
      break;
    }

#undef SELECT_CHORD_TYPE

#define SELECT_CHORD_ACC(uppercase, lowercase) \
  case ChordAccent::uppercase: \
    SELECT_CHILD (creator_accent_flowbox, creator_accent_##lowercase); \
    break

  /* set accent */
  switch (descr->accent_)
    {
      SELECT_CHORD_ACC (Seventh, 7);
      SELECT_CHORD_ACC (MajorSeventh, j7);
      SELECT_CHORD_ACC (FlatNinth, b9);
      SELECT_CHORD_ACC (Ninth, 9);
      SELECT_CHORD_ACC (SharpNinth, s9);
      SELECT_CHORD_ACC (Eleventh, 11);
      SELECT_CHORD_ACC (FlatFifthSharpEleventh, b5_s11);
      SELECT_CHORD_ACC (SharpFifthFlatThirteenth, s5_b13);
      SELECT_CHORD_ACC (SixthThirteenth, 6_13);
    default:
      break;
    }

#undef SELECT_CHORD_ACC

  /* set bass note */
  if (descr->has_bass_)
    {
      SELECT_CHILD (
        creator_bass_note_flowbox,
        creator_bass_notes[ENUM_VALUE_TO_INT (descr->bass_note_)]);
    }

#undef SELECT_CHILD

  gtk_widget_set_sensitive (
    GTK_WIDGET (self->creator_visibility_in_scale), self->scale != nullptr);

  gtk_flow_box_set_activate_on_single_click (self->creator_accent_flowbox, true);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->creator_root_note_flowbox), "selected-children-changed",
    G_CALLBACK (on_creator_root_note_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_type_flowbox), "selected-children-changed",
    G_CALLBACK (on_creator_type_selected_children_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_accent_flowbox), "child-activated",
    G_CALLBACK (on_creator_accent_child_activated), self);
  g_signal_connect (
    G_OBJECT (self->creator_bass_note_flowbox), "selected-children-changed",
    G_CALLBACK (on_creator_bass_note_selected_children_changed), self);
}

static void
setup_diatonic_tab (ChordSelectorWindowWidget * self)
{
  if (self->scale)
    {
      auto &scale = self->scale->scale_;
      /* major */
      if (scale.type_ == MusicalScale::Type::Ionian)
        {
          gtk_label_set_text (self->diatonic_i_lbl, "I");
          gtk_label_set_text (self->diatonic_ii_lbl, "ii");
          gtk_label_set_text (self->diatonic_iii_lbl, "iii");
          gtk_label_set_text (self->diatonic_iv_lbl, "IV");
          gtk_label_set_text (self->diatonic_v_lbl, "V");
          gtk_label_set_text (self->diatonic_vi_lbl, "vi");
          gtk_label_set_text (self->diatonic_vii_lbl, "vii\u00B0");
        }
      /* minor */
      else if (scale.type_ == MusicalScale::Type::Aeolian)
        {
          gtk_label_set_text (self->diatonic_i_lbl, "i");
          gtk_label_set_text (self->diatonic_ii_lbl, "ii\u00B0");
          gtk_label_set_text (self->diatonic_iii_lbl, "III");
          gtk_label_set_text (self->diatonic_iv_lbl, "iv");
          gtk_label_set_text (self->diatonic_v_lbl, "v");
          gtk_label_set_text (self->diatonic_vi_lbl, "VI");
          gtk_label_set_text (self->diatonic_vii_lbl, "VII");
        }
    }
  else
    {
      /* remove diatonic page */
      gtk_notebook_remove_page (self->notebook, 1);
    }
}

static gboolean
creator_filter (GtkFlowBoxChild * child, ChordSelectorWindowWidget * self)
{
#if 0
  z_debug (
    "scale %p, in scale active %d",
    self->scale,
    gtk_check_button_get_active (
      self->creator_visibility_in_scale));
#endif

  if (
    self->scale
    && gtk_check_button_get_active (self->creator_visibility_in_scale))
    {
      /* root notes */
      for (int i = 0; i < 12; i++)
        {
          if (child != self->creator_root_notes[i])
            continue;

          bool ret = self->scale->scale_.contains_note (
            ENUM_INT_TO_VALUE (MusicalNote, i));
          return ret;
        }

      /* bass notes */
      for (int i = 0; i < 12; i++)
        {
          if (child != self->creator_bass_notes[i])
            continue;

          return self->scale->scale_.contains_note (
            ENUM_INT_TO_VALUE (MusicalNote, i));
        }

      /* accents */
      for (unsigned int i = 0; i < ENUM_COUNT (ChordAccent) - 1; i++)
        {
          if (self->creator_accents[i] != child)
            continue;

          MusicalNote note =
            ENUM_INT_TO_VALUE (MusicalNote, get_selected_root_note (self));
          ChordType type =
            ENUM_INT_TO_VALUE (ChordType, get_selected_chord_type (self));

          if ((int) note == -1 || (int) type == -1)
            return 0;

          bool ret = self->scale->scale_.is_accent_in_scale (
            note, type, ENUM_INT_TO_VALUE (ChordAccent, i + 1));
          return ret;
        }

      /* type */
      for (
        unsigned int i = ENUM_VALUE_TO_INT (ChordType::Major);
        i <= ENUM_VALUE_TO_INT (ChordType::Augmented); i++)
        {
          if (
            self->creator_types[i - ENUM_VALUE_TO_INT (ChordType::Major)]
            != child)
            continue;

          MusicalNote note =
            ENUM_INT_TO_VALUE (MusicalNote, get_selected_root_note (self));
          if ((int) note == -1)
            return true;

          ChordDescriptor chord (
            note, 0, ENUM_INT_TO_VALUE (MusicalNote, 0),
            ENUM_INT_TO_VALUE (ChordType, i), ChordAccent::None, 0);

          int ret = self->scale->scale_.contains_chord (chord);

          return ret;
        }

      return false;
    }
  else
    {
      return true;
    }
}

static void
on_group_changed (GtkCheckButton * check_btn, ChordSelectorWindowWidget * self)
{
  if (gtk_check_button_get_active (check_btn))
    {
      z_debug ("GROUP CHANGED");
      gtk_flow_box_invalidate_filter (self->creator_root_note_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_type_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_accent_flowbox);
      gtk_flow_box_invalidate_filter (self->creator_bass_note_flowbox);
    }
}

void
chord_selector_window_widget_present (const int chord_idx, GtkWidget * parent)
{
  auto * self = static_cast<ChordSelectorWindowWidget *> (
    g_object_new (CHORD_SELECTOR_WINDOW_WIDGET_TYPE, nullptr));

  self->chord_idx = chord_idx;
  const auto &descr = CHORD_EDITOR->chords_[chord_idx];
  self->descr_clone = new ChordDescriptor (descr);

#if 0
  ArrangerObject * region_obj =
    (ArrangerObject *)
    CLIP_EDITOR->get_region ();
  z_return_val_if_fail (region_obj, nullptr);

  self->scale =
    chord_track_get_scale_at_pos (
      P_CHORD_TRACK,
      &region_obj->pos);
#endif
  self->scale = P_CHORD_TRACK->get_scale_at_pos (PLAYHEAD);

  adw_dialog_present (ADW_DIALOG (self), parent);

  setup_creator_tab (self);
  setup_diatonic_tab (self);
}

static void
chord_selector_window_widget_finalize (GObject * obj)
{
  auto self = Z_CHORD_SELECTOR_WINDOW_WIDGET (obj);

  if (self->descr_clone)
    delete self->descr_clone;

  G_OBJECT_CLASS (chord_selector_window_widget_parent_class)->finalize (obj);
}

static void
chord_selector_window_widget_class_init (ChordSelectorWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "chord_selector_window.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child (klass, ChordSelectorWindowWidget, child)

  BIND_CHILD (notebook);
  BIND_CHILD (diatonic_flowbox);
  BIND_CHILD (diatonic_i);
  BIND_CHILD (diatonic_ii);
  BIND_CHILD (diatonic_iii);
  BIND_CHILD (diatonic_iv);
  BIND_CHILD (diatonic_v);
  BIND_CHILD (diatonic_vi);
  BIND_CHILD (diatonic_vii);
  BIND_CHILD (diatonic_i_lbl);
  BIND_CHILD (diatonic_ii_lbl);
  BIND_CHILD (diatonic_iii_lbl);
  BIND_CHILD (diatonic_iv_lbl);
  BIND_CHILD (diatonic_v_lbl);
  BIND_CHILD (diatonic_vi_lbl);
  BIND_CHILD (diatonic_vii_lbl);
  BIND_CHILD (creator_root_note_flowbox);
  BIND_CHILD (creator_root_note_c);
  BIND_CHILD (creator_root_note_cs);
  BIND_CHILD (creator_root_note_d);
  BIND_CHILD (creator_root_note_ds);
  BIND_CHILD (creator_root_note_e);
  BIND_CHILD (creator_root_note_f);
  BIND_CHILD (creator_root_note_fs);
  BIND_CHILD (creator_root_note_g);
  BIND_CHILD (creator_root_note_gs);
  BIND_CHILD (creator_root_note_a);
  BIND_CHILD (creator_root_note_as);
  BIND_CHILD (creator_root_note_b);
  BIND_CHILD (creator_type_flowbox);
  BIND_CHILD (creator_type_maj);
  BIND_CHILD (creator_type_min);
  BIND_CHILD (creator_type_dim);
  BIND_CHILD (creator_type_sus4);
  BIND_CHILD (creator_type_sus2);
  BIND_CHILD (creator_type_aug);
  BIND_CHILD (creator_accent_flowbox);
  BIND_CHILD (creator_accent_7);
  BIND_CHILD (creator_accent_j7);
  BIND_CHILD (creator_accent_b9);
  BIND_CHILD (creator_accent_9);
  BIND_CHILD (creator_accent_s9);
  BIND_CHILD (creator_accent_11);
  BIND_CHILD (creator_accent_b5_s11);
  BIND_CHILD (creator_accent_s5_b13);
  BIND_CHILD (creator_accent_6_13);
  BIND_CHILD (creator_bass_note_flowbox);
  BIND_CHILD (creator_bass_note_c);
  BIND_CHILD (creator_bass_note_cs);
  BIND_CHILD (creator_bass_note_d);
  BIND_CHILD (creator_bass_note_ds);
  BIND_CHILD (creator_bass_note_e);
  BIND_CHILD (creator_bass_note_f);
  BIND_CHILD (creator_bass_note_fs);
  BIND_CHILD (creator_bass_note_g);
  BIND_CHILD (creator_bass_note_gs);
  BIND_CHILD (creator_bass_note_a);
  BIND_CHILD (creator_bass_note_as);
  BIND_CHILD (creator_bass_note_b);
  BIND_CHILD (creator_visibility_all);
  BIND_CHILD (creator_visibility_in_scale);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = chord_selector_window_widget_finalize;
}

static void
chord_selector_window_widget_init (ChordSelectorWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_check_button_set_group (
    self->creator_visibility_in_scale, self->creator_visibility_all);
  gtk_check_button_set_active (self->creator_visibility_all, true);

  self->creator_root_notes[0] = self->creator_root_note_c;
  self->creator_root_notes[1] = self->creator_root_note_cs;
  self->creator_root_notes[2] = self->creator_root_note_d;
  self->creator_root_notes[3] = self->creator_root_note_ds;
  self->creator_root_notes[4] = self->creator_root_note_e;
  self->creator_root_notes[5] = self->creator_root_note_f;
  self->creator_root_notes[6] = self->creator_root_note_fs;
  self->creator_root_notes[7] = self->creator_root_note_g;
  self->creator_root_notes[8] = self->creator_root_note_gs;
  self->creator_root_notes[9] = self->creator_root_note_a;
  self->creator_root_notes[10] = self->creator_root_note_as;
  self->creator_root_notes[11] = self->creator_root_note_b;

  self->creator_bass_notes[0] = self->creator_bass_note_c;
  self->creator_bass_notes[1] = self->creator_bass_note_cs;
  self->creator_bass_notes[2] = self->creator_bass_note_d;
  self->creator_bass_notes[3] = self->creator_bass_note_ds;
  self->creator_bass_notes[4] = self->creator_bass_note_e;
  self->creator_bass_notes[5] = self->creator_bass_note_f;
  self->creator_bass_notes[6] = self->creator_bass_note_fs;
  self->creator_bass_notes[7] = self->creator_bass_note_g;
  self->creator_bass_notes[8] = self->creator_bass_note_gs;
  self->creator_bass_notes[9] = self->creator_bass_note_a;
  self->creator_bass_notes[10] = self->creator_bass_note_as;
  self->creator_bass_notes[11] = self->creator_bass_note_b;

  self->creator_types[0] = self->creator_type_maj;
  self->creator_types[1] = self->creator_type_min;
  self->creator_types[2] = self->creator_type_dim;
  self->creator_types[3] = self->creator_type_sus4;
  self->creator_types[4] = self->creator_type_sus2;
  self->creator_types[5] = self->creator_type_aug;

  self->creator_accents[0] = self->creator_accent_7;
  self->creator_accents[1] = self->creator_accent_j7;
  self->creator_accents[2] = self->creator_accent_b9;
  self->creator_accents[3] = self->creator_accent_9;
  self->creator_accents[4] = self->creator_accent_s9;
  self->creator_accents[5] = self->creator_accent_11;
  self->creator_accents[6] = self->creator_accent_b5_s11;
  self->creator_accents[7] = self->creator_accent_s5_b13;
  self->creator_accents[8] = self->creator_accent_6_13;

  gtk_flow_box_set_min_children_per_line (self->creator_root_note_flowbox, 12);
  gtk_flow_box_set_min_children_per_line (self->creator_type_flowbox, 12);
  gtk_flow_box_set_min_children_per_line (self->creator_accent_flowbox, 12);
  gtk_flow_box_set_min_children_per_line (self->creator_bass_note_flowbox, 12);

  /* set filter functions */
  gtk_flow_box_set_filter_func (
    self->creator_root_note_flowbox, (GtkFlowBoxFilterFunc) creator_filter,
    self, nullptr);
  gtk_flow_box_set_filter_func (
    self->creator_type_flowbox, (GtkFlowBoxFilterFunc) creator_filter, self,
    nullptr);
  gtk_flow_box_set_filter_func (
    self->creator_accent_flowbox, (GtkFlowBoxFilterFunc) creator_filter, self,
    nullptr);
  gtk_flow_box_set_filter_func (
    self->creator_bass_note_flowbox, (GtkFlowBoxFilterFunc) creator_filter,
    self, nullptr);

  /* set signals */
  g_signal_connect (
    G_OBJECT (self->creator_visibility_all), "toggled",
    G_CALLBACK (on_group_changed), self);
  g_signal_connect (
    G_OBJECT (self->creator_visibility_in_scale), "toggled",
    G_CALLBACK (on_group_changed), self);
  g_signal_connect (G_OBJECT (self), "closed", G_CALLBACK (on_closed), self);
}
