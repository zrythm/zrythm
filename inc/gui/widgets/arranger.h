// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Arranger base widget.
 */

#ifndef __GUI_WIDGETS_ARRANGER_H__
#define __GUI_WIDGETS_ARRANGER_H__

#include "dsp/position.h"
#include "dsp/transport.h"
#include "gui/widgets/main_window.h"
#include "utils/ui.h"

#include "gtk_wrapper.h"

#define ARRANGER_WIDGET_TYPE (arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (ArrangerWidget, arranger_widget, Z, ARRANGER_WIDGET, GtkWidget)

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote          MidiNote;
typedef struct SnapGrid          SnapGrid;
typedef struct AutomationPoint   AutomationPoint;

typedef struct _GtkEventControllerMotion GtkEventControllerMotion;
typedef struct ArrangerObject            ArrangerObject;
typedef struct ArrangerSelections        ArrangerSelections;
typedef struct EditorSettings            EditorSettings;
typedef struct ObjectPool                ObjectPool;
typedef struct _RulerWidget              RulerWidget;
enum class ArrangerObjectType;
enum class TransportDisplay;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define ARRANGER_WIDGET_GET_ACTION(arr, actn) \
  (arr->action == UI_OVERLAY_ACTION_##actn)

enum class ArrangerCursor
{
  /** Invalid cursor. */
  ARRANGER_CURSOR_NONE,
  ARRANGER_CURSOR_SELECT,
  ARRANGER_CURSOR_SELECT_STRETCH,
  ARRANGER_CURSOR_EDIT,
  ARRANGER_CURSOR_AUTOFILL,
  ARRANGER_CURSOR_CUT,
  ARRANGER_CURSOR_ERASER,
  ARRANGER_CURSOR_AUDITION,
  ARRANGER_CURSOR_RAMP,
  ARRANGER_CURSOR_GRAB,
  ARRANGER_CURSOR_GRABBING,
  ARRANGER_CURSOR_RESIZING_L,
  ARRANGER_CURSOR_RESIZING_L_FADE,
  ARRANGER_CURSOR_STRETCHING_L,
  ARRANGER_CURSOR_RESIZING_L_LOOP,
  ARRANGER_CURSOR_RESIZING_R,
  ARRANGER_CURSOR_RESIZING_R_FADE,
  ARRANGER_CURSOR_STRETCHING_R,
  ARRANGER_CURSOR_RESIZING_R_LOOP,
  ARRANGER_CURSOR_RESIZING_UP,
  ARRANGER_CURSOR_RESIZING_UP_FADE_IN,
  ARRANGER_CURSOR_RESIZING_UP_FADE_OUT,
  ARRANGER_CURSOR_GRABBING_COPY,
  ARRANGER_CURSOR_GRABBING_LINK,
  ARRANGER_CURSOR_RANGE,
  ARRANGER_CURSOR_FADE_IN,
  ARRANGER_CURSOR_FADE_OUT,
  ARRANGER_CURSOR_RENAME,
  ARRANGER_CURSOR_PANNING,
};

/**
 * Type of arranger.
 */
enum class ArrangerWidgetType
{
  ARRANGER_WIDGET_TYPE_TIMELINE,
  ARRANGER_WIDGET_TYPE_MIDI,
  ARRANGER_WIDGET_TYPE_MIDI_MODIFIER,
  ARRANGER_WIDGET_TYPE_AUDIO,
  ARRANGER_WIDGET_TYPE_CHORD,
  ARRANGER_WIDGET_TYPE_AUTOMATION,
};

#if 0
typedef enum ArrangerWidgetHoverType
{
  ARRANGER_WIDGET_HOVER_TYPE_NONE,
  ARRANGER_WIDGET_HOVER_TYPE_TRACK,
  ARRANGER_WIDGET_HOVER_TYPE_TRACK_LANE,
  ARRANGER_WIDGET_HOVER_TYPE_AUTOMATION_TRACK,
} ArrangerWidgetHoverType;
#endif

/**
 * The arranger widget is a canvas that draws all
 * the arranger objects it contains.
 */
typedef struct _ArrangerWidget
{
  GtkWidget parent_instance;

  /** Type of arranger this is. */
  ArrangerWidgetType type;

  GtkGestureDrag *           drag;
  GtkGestureClick *          click;
  GtkGestureClick *          right_click;
  GtkEventControllerMotion * motion_controller;

  /** Used when dragging. */
  double last_offset_x;
  double last_offset_y;

  /** Whether there is an offset from a user scroll that should
   * be added to the offset while dragging.
   * FIXME: This is probably not needed - see ruler implementation on_motion()
   * on how to avoid these. */
  double offset_x_from_scroll;
  double offset_y_from_scroll;

  UiOverlayAction action;

  /** X-axis coordinate at start of drag. */
  double start_x;

  /** Y-axis coordinate at start of drag. */
  double start_y;

  /** X-axis coordinate at the start of the drag,
   * in pixels. */
  double start_pos_px;

  /**
   * Whether a drag update operation started.
   *
   * drag_update will be skipped unless this is
   * true or gtk_drag_check_threshold() returns
   * true.
   */
  bool drag_update_started;

  /** Whether an object exists, so we can use the
   * earliest_obj_start_pos. */
  bool earliest_obj_exists;

  /** Start Position of the earliest object
   * at the start of the drag. */
  Position earliest_obj_start_pos;

  /**
   * Fade in/out position at start.
   *
   * Used when moving fade in/out points.
   */
  Position fade_pos_at_start;

  /**
   * The object that was clicked in this drag cycle, if any.
   *
   * This is the ArrangerObject that was clicked, even though there could be
   * more selected.
   *
   * This is also used when changing values via the event viewer.
   *
   * FIXME this sometimes stores project objects (that should
   * not be free'd) and sometimes clones (eg,
   * arranger_object_edit_begin/finish()). Only allow one type.
   */
  ArrangerObject * start_object;

  /** Object currently hovered. */
  ArrangerObject * hovered_object;

  /** Whether the start object was selected before
   * drag_begin. */
  int start_object_was_selected;

  /**
   * A clone of the ArrangerSelections on drag begin.
   *
   * When autofilling velocities, this is used to store the affected objects
   * before editing.
   *
   * This must contain clones only.
   */
  ArrangerSelections * sel_at_start;

  /**
   * Region on drag begin, if editing automation.
   */
  Region * region_at_start;

  /** Selections to delete, used with the eraser
   * tool. */
  ArrangerSelections * sel_to_delete;

  /** Start Position of the earliest object
   * currently. */
  // Position             earliest_obj_pos;

  /** The absolute (not snapped) Position at the
   * start of a drag, translated from start_x. */
  Position start_pos;

  /** Whether playback was paused during drag
   * begin. */
  bool was_paused;

  /** Playhead position at start of drag. */
  Position playhead_pos_at_start;

  /** The absolute (not snapped) current diff in
   * ticks from the curr_pos to the start_pos. */
  double curr_ticks_diff_from_start;

  /** The adjusted diff in ticks to use for moving
   * objects starting from their cached start
   * positions. */
  double adj_ticks_diff;

  /** adj_ticks_diff in last cycle. */
  double last_adj_ticks_diff;

  /** The absolute (not snapped) Position as of the
   * current action. */
  Position curr_pos;

  Position end_pos; ///< for moving regions
  gboolean key_is_pressed;

  /** Current hovering positions (absolute). */
  double hover_x;
  double hover_y;

  bool hovered;

  /** Number of clicks in current action. */
  int n_press;

  /** Associated SnapGrid. */
  SnapGrid * snap_grid;

  /** Whether shift button is held down. */
  int shift_held;

  /** Whether Ctrl button is held down. */
  int ctrl_held;

  /** Whether Alt is currently held down. */
  int alt_held;

  gint64 last_frame_time;

  /* ----- TIMELINE ------ */

  /** The number of visible tracks moved during a moving
   * operation between tracks up to the last cycle. */
  int visible_track_diff;

  /** The number of lanes moved during a moving operation
   * between lanes, up to the last cycle. */
  int lane_diff;

  /** The number of visible automation t racks moved during a
   * moving operation between automation tracks up to the
   * last cycle. */
  int visible_at_diff;

  /** Whether this TimelineArrangerWidget is for
   * the PinnedTracklist or not. */
  int is_pinned;

  /**
   * 1 if resizing range.
   */
  int resizing_range;

  /**
   * 1 if this is the first call to resize the range,
   * so range1 can be set.
   */
  int resizing_range_start;

  AutomationTrack * hovered_at;
  TrackLane *       hovered_lane;
  Track *           hovered_track;

  /* textures used as region icons */
  GdkTexture * symbolic_link_texture;
  GdkTexture * music_note_16th_texture;
  GdkTexture * fork_awesome_snowflake_texture;
  GdkTexture * media_playlist_repeat_texture;

  /** Size of above textures. */
  int region_icon_texture_size;

  /** Cached nodes for region loop lines. */
  GskRenderNode * loop_line_node;
  GskRenderNode * clip_start_line_node;
  GskRenderNode * cut_line_node;

  /* ----- END TIMELINE ----- */

  /* ------ MIDI (PIANO ROLL) ---- */

  /** The note currently hovering over */
  int hovered_note;

  /* ------ END MIDI (PIANO ROLL) ---- */

  /* ------ MIDI MODIFIER ---- */

  /** 1-127. */
  int start_vel_val;

  /**
   * Maximum Velocity diff applied in this action.
   *
   * Used in drag_end to create an UndableAction.
   * This can have any value, even greater than 127
   * and it will be clamped when applying it to
   * a Velocity.
   */
  int vel_diff;

  /* ------ END MIDI MODIFIER ---- */

  /* ------- CHORD ------- */

  /** Index of the chord being hovered on. */
  int hovered_chord_index;

  /* ------- END CHORD ------- */

  /* --- AUDIO --- */

  /**
   * Float value at start.
   *
   * Used when changing the audio region gain.
   */
  float fval_at_start;

  double dval_at_start;

  /* --- END AUDIO --- */

  /** Px the playhead was last drawn at, so we can
   * redraw this and the new px only when the
   * playhead changes position. */
  int last_playhead_px;

  /** Set to 1 to redraw. */
  bool redraw;

  // cairo_t *      cached_cr;

  // cairo_surface_t * cached_surface;

  /** Rectangle in the last call. */
  graphene_rect_t last_rect;

  /**
   * Whether the current selections can link
   * (ie, only regions are selected).
   *
   * To be set on drag begin.
   */
  bool can_link;

  /** Whether a rectangle is highlighted for DND. */
  bool is_highlighted;

  /** The rectangle to highlight. */
  GdkRectangle highlight_rect;
  // GdkRectangle   prev_highlight_rect;
  //

  /** Last selection rectangle, used to redraw the
   * union of the new selection and this. */
  GdkRectangle last_selection_rect;

  /**
   * Drag start button (primary, secondary, etc.).
   *
   * Can be tested against GDK_BUTTON_SECONDARY and
   * GDK_BUTTON_PRIMARY.
   */
  guint drag_start_btn;

  /**
   * Whether this is the first time the widget is
   * drawn.
   *
   * This is used for loading back the scroll
   * positions saved in the project.
   */
  bool first_draw;

  /** New temporary hadjustment value used when zooming
   * in/out. */
  double new_hadj_val;

  /** Cached setting. */
  TransportDisplay ruler_display;

  /**
   * Layout for drawing velocity text.
   *
   * TODO move to Velocity if parallel
   * processing is needed - no need now.
   */
  PangoLayout * vel_layout;

  /**
   * Layout for drawing automation point text.
   *
   * TODO move to AutomationPoint if parallel
   * processing is needed - no need now.
   */
  PangoLayout * ap_layout;

  /**
   * Layout for drawing audio editor text.
   */
  PangoLayout * audio_layout;

  /** Layout for debug text. */
  PangoLayout * debug_layout;

  /**
   * Cached playhead x to draw.
   *
   * This is used to avoid queuing drawing at x and
   * then drawing after it (if playhead moved). The
   * playhead will be drawn at the location it
   * was when the draw was queued.
   */
  int queued_playhead_px;

  /**
   * Array of objects to draw.
   *
   * To be reused in snapshot().
   */
  GPtrArray * hit_objs_to_draw;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} ArrangerWidget;

const char *
arranger_widget_get_type_str (ArrangerWidgetType type);

/**
 * Returns if the arranger can scroll vertically.
 */
bool
arranger_widget_can_scroll_vertically (ArrangerWidget * self);

/**
 * Creates a timeline widget using the given
 * timeline data.
 */
void
arranger_widget_setup (
  ArrangerWidget *   self,
  ArrangerWidgetType type,
  SnapGrid *         snap_grid);

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (ArrangerWidget * self, ArrangerCursor cursor);

/**
 * Wrapper of the UI functions based on the arranger
 * type.
 */
int
arranger_widget_pos_to_px (ArrangerWidget * self, Position * pos, int use_padding);

/**
 * Gets the cursor based on the current hover
 * position.
 */
ArrangerCursor
arranger_widget_get_cursor (ArrangerWidget * self);

/**
 * Figures out which cursor should be used based
 * on the current state and then sets it.
 */
void
arranger_widget_refresh_cursor (ArrangerWidget * self);

/**
 * Get all objects currently present in the arranger.
 */
void
arranger_widget_get_all_objects (ArrangerWidget * self, GPtrArray * objs_arr);

/**
 * Wrapper for ui_px_to_pos depending on the
 * arranger type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  bool             has_padding);

/**
 * Returns the current visible rectangle.
 *
 * @param rect The rectangle to fill in.
 */
void
arranger_widget_get_visible_rect (ArrangerWidget * self, GdkRectangle * rect);

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
void
arranger_widget_get_hit_objects_at_point (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y,
  GPtrArray *        arr);

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 */
void
arranger_widget_get_hit_objects_in_rect (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  GPtrArray *        arr);

/**
 * Returns the ArrangerObject of the given type
 * at (x,y).
 *
 * @param type The arranger object type, or -1 to
 *   search for all types.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  const double       x,
  const double       y);

void
arranger_widget_select_all (ArrangerWidget * self, bool select, bool fire_events);

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
NONNULL bool
arranger_widget_is_in_moving_operation (ArrangerWidget * self);

/**
 * Returns the ArrangerSelections for this
 * ArrangerWidget.
 */
RETURNS_NONNULL
ArrangerSelections *
arranger_widget_get_selections (ArrangerWidget * self);

/**
 * Only redraws the playhead part.
 */
void
arranger_widget_redraw_playhead (ArrangerWidget * self);

SnapGrid *
arranger_widget_get_snap_grid (ArrangerWidget * self);

/**
 * Called from MainWindowWidget because some
 * events don't reach here.
 */
gboolean
arranger_widget_on_key_press (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self);

void
arranger_widget_on_key_release (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self);

/**
 * Scroll until the given object is visible.
 *
 * @param horizontal 1 for horizontal, 2 for
 *   vertical.
 * @param up Whether scrolling up or down.
 * @param padding Padding pixels.
 */
NONNULL void
arranger_widget_scroll_until_obj (
  ArrangerWidget * self,
  ArrangerObject * obj,
  int              horizontal,
  int              up,
  int              left,
  int              padding);

/**
 * Toggles the mute status of the selection, based
 * on the mute status of the selected object.
 *
 * This creates an undoable action and executes it.
 */
void
arranger_widget_toggle_selections_muted (
  ArrangerWidget * self,
  ArrangerObject * clicked_object);

/**
 * Returns the earliest possible position allowed
 * in this arranger (eg, 1.1.0.0 for timeline).
 */
void
arranger_widget_get_min_possible_position (ArrangerWidget * self, Position * pos);

/**
 * Sets the highlight rectangle.
 *
 * @param rect The rectangle with absolute positions, or NULL
 *   to unset/unhighlight.
 */
void
arranger_widget_set_highlight_rect (ArrangerWidget * self, GdkRectangle * rect);

/**
 * Returns the EditorSettings corresponding to
 * the given arranger.
 */
EditorSettings *
arranger_widget_get_editor_settings (ArrangerWidget * self);

/**
 * Get just the values, adjusted properly for special cases
 * (like pinned timeline).
 */
EditorSettings
arranger_widget_get_editor_setting_values (ArrangerWidget * self);

bool
arranger_widget_is_playhead_visible (ArrangerWidget * self);

NONNULL void
arranger_widget_handle_playhead_auto_scroll (ArrangerWidget * self, bool force);

typedef void (*ArrangerWidgetForeachFunc) (ArrangerWidget * arranger);

/**
 * Runs the given function for each arranger.
 */
NONNULL void
arranger_widget_foreach (ArrangerWidgetForeachFunc func);

NONNULL RulerWidget *
arranger_widget_get_ruler (ArrangerWidget * self);

/**
 * Returns whether any arranger is in the middle
 * of an action.
 */
bool
arranger_widget_any_doing_action (void);

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 */
int
arranger_widget_get_playhead_px (ArrangerWidget * self);

#define arranger_widget_print_action(self) \
  g_debug ("action: %s", ui_overlay_strings[self->action])

/**
 * Returns true if MIDI arranger and track mode
 * is enabled.
 */
bool
arranger_widget_get_drum_mode_enabled (ArrangerWidget * self);

/**
 * Called when an item needs to be created at the
 * given position.
 *
 * @param autofilling Whether this is part of an
 *   autofill action.
 */
NONNULL void
arranger_widget_create_item (
  ArrangerWidget * self,
  double           start_x,
  double           start_y,
  bool             autofilling);

/**
 * To be called after using arranger_widget_create_item() in
 * an action (ie, not from click + drag interaction with the
 * arranger) to finish the action.
 *
 * @return Whether an action was performed.
 */
NONNULL bool
arranger_widget_finish_creating_item_from_action (
  ArrangerWidget * self,
  double           x,
  double           y);

/**
 * Returns the total height (including off-screen).
 */
int
arranger_widget_get_total_height (ArrangerWidget * self);

/**
 * @}
 */

#endif
