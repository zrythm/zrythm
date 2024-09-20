// SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "actions/midi_mapping_action.h"
#include "actions/port_action.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/meter.h"
#include "dsp/midi_mapping.h"
#include "dsp/port.h"
#include "gui/widgets/bar_slider.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/dialogs/port_info.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/popovers/port_connections_popover.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorPortWidget, inspector_port_widget, GTK_TYPE_WIDGET)

#if HAVE_JACK
static void
on_jack_toggled (GtkWidget * widget, InspectorPortWidget * self)
{
  self->port->set_expose_to_backend (
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}
#endif

/**
 * Gets the label for the bar slider (port string).
 *
 * @return if the string was filled in or not.
 */
static bool
get_port_str (InspectorPortWidget * self, Port * port, char * buf)
{
  if (
    port->id_.owner_type_ == PortIdentifier::OwnerType::Plugin
    || port->id_.owner_type_ == PortIdentifier::OwnerType::Fader
    || port->id_.owner_type_ == PortIdentifier::OwnerType::Track)
    {
      int num_midi_mappings = MIDI_MAPPINGS->get_for_port (*port, nullptr);

      const char * star = (num_midi_mappings > 0 ? "*" : "");
      char * port_label = g_markup_escape_text (port->id_.label_.c_str (), -1);
      auto   color_prefix =
        fmt::format ("<span foreground=\"{}\">", self->hex_color);
      char color_suffix[40] = "</span>";
      if (port->id_.flow_ == PortFlow::Input)
        {
          int num_unlocked_srcs = port->get_num_unlocked_srcs ();
          sprintf (
            buf,
            "%s <small><sup>"
            "%s%d%s%s"
            "</sup></small>",
            port_label, color_prefix.c_str (), num_unlocked_srcs, star,
            color_suffix);
          self->last_num_connections = num_unlocked_srcs;
          return true;
        }
      else if (port->id_.flow_ == PortFlow::Output)
        {
          int num_unlocked_dests = port->get_num_unlocked_dests ();
          sprintf (
            buf,
            "%s <small><sup>"
            "%s%d%s%s"
            "</sup></small>",
            port_label, color_prefix.c_str (), num_unlocked_dests, star,
            color_suffix);
          self->last_num_connections = num_unlocked_dests;
          return true;
        }
      g_free (port_label);
    }
  else
    {
      z_return_val_if_reached (false);
    }
  return false;
}

static void
show_context_menu (InspectorPortWidget * self, gdouble x, gdouble y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];

  if (self->port->id_.type_ == PortType::Control)
    {
      sprintf (tmp, "app.reset-control::%p", self->port);
      menuitem = z_gtk_create_menu_item (_ ("Reset"), nullptr, tmp);
      g_menu_append_item (menu, menuitem);

      sprintf (tmp, "app.bind-midi-cc::%p", self->port);
      menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
      g_menu_append_item (menu, menuitem);
    }

  sprintf (tmp, "app.port-view-info::%p", self->port);
  menuitem = z_gtk_create_menu_item (_ ("View info"), nullptr, tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  InspectorPortWidget * self)
{
  if (n_press != 1)
    return;

  z_info ("right click");

  show_context_menu (self, x, y);
}

static void
on_popover_closed (GtkPopover * popover, InspectorPortWidget * self)
{
  get_port_str (self, self->port, self->bar_slider->prefix);
}

static void
on_double_click (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  InspectorPortWidget * self)
{
  if (n_press != 2)
    return;

  z_info ("double click");

  /* need to ref here because gtk unrefs
   * internally */
  g_object_ref (self->connections_popover);

  port_connections_popover_widget_refresh (
    self->connections_popover, self->port);
  gtk_popover_popup (GTK_POPOVER (self->connections_popover));

  g_signal_connect (
    G_OBJECT (self->connections_popover), "closed",
    G_CALLBACK (on_popover_closed), self);
}

/** 250 ms */
/*static const float MAX_TIME = 250000.f;*/

static float
get_port_value (InspectorPortWidget * self)
{
  Port * port = self->port;
  switch (port->id_.type_)
    {
    case PortType::Audio:
    case PortType::Event:
      {
        float val, max;
        self->meter->get_value (AudioValueFormat::Fader, &val, &max);
        return val;
      }
      break;
    case PortType::CV:
      {
        return port->buf_[0];
      }
      break;
    case PortType::Control:
      {
        auto ctrl = dynamic_cast<ControlPort *> (port);
        return ctrl->real_val_to_normalized (ctrl->unsnapped_control_);
      }
      break;
    default:
      break;
    }
  return 0.f;
}

static float
get_snapped_port_value (InspectorPortWidget * self)
{
  Port * port = self->port;
  if (port->get_type () == PortType::Control)
    {
      auto control_port = dynamic_cast<ControlPort *> (port);
      /* optimization */
      if (
        G_LIKELY (self->last_port_val_set)
        && math_floats_equal (self->last_real_val, control_port->control_))
        {
          return self->last_normalized_val;
        }

      self->last_real_val = control_port->control_;
      self->last_normalized_val =
        control_port->real_val_to_normalized (control_port->control_);
      self->last_port_val_set = true;
      return self->last_normalized_val;
    }
  else
    {
      return get_port_value (self);
    }
}

static void
set_port_value (InspectorPortWidget * self, float val)
{
  auto ctrl = dynamic_cast<ControlPort *> (self->port);
  ctrl->set_control_value (
    ctrl->normalized_val_to_real (val), F_NOT_NORMALIZED, true);
}

static void
set_init_port_value (InspectorPortWidget * self, float val)
{
  /*z_info (*/
  /*"val change started: {:f}", (double) val);*/
  self->normalized_init_port_val = val;
}

static void
val_change_finished (InspectorPortWidget * self, float val)
{
  /*z_info (*/
  /*"val change finished: {:f}", (double) val);*/
  if (!math_floats_equal (val, self->normalized_init_port_val))
    {
      auto ctrl = dynamic_cast<ControlPort *> (self->port);
      /* set port to previous val */
      ctrl->set_control_value (
        ctrl->normalized_val_to_real (self->normalized_init_port_val),
        F_NOT_NORMALIZED, false);

      try
        {
          UNDO_MANAGER->perform (std::make_unique<PortAction> (
            PortAction::Type::SetControlValue, self->port->id_, val, true));
        }
      catch (const ZrythmException &e)
        {
          e.handle (format_str (
            _ ("Failed to set control {} to {:f}"), self->port->id_.label_,
            (double) val));
        }
    }
}

static gboolean
bar_slider_tick_cb (
  GtkWidget *           widget,
  GdkFrameClock *       frame_clock,
  InspectorPortWidget * self)
{
  /* update bar slider label if num connections
   * changed */
  Port * port = self->port;
  int    num_connections = 0;
  if (port->get_flow () == PortFlow::Input)
    {
      num_connections = port->get_num_unlocked_srcs ();
    }
  else if (port->get_flow () == PortFlow::Output)
    {
      num_connections = port->get_num_unlocked_dests ();
    }
  if (num_connections != self->last_num_connections)
    {
      get_port_str (self, self->port, self->bar_slider->prefix);
    }

  /* if enough time passed, try to update the
   * tooltip */
  gint64 now = g_get_monotonic_time ();
  if (now - self->last_tooltip_change > 100000)
    {
      char   str[2000];
      auto   full_designation = self->port->get_full_designation ();
      char * full_designation_escaped =
        g_markup_escape_text (full_designation.c_str (), -1);
      char * comment_escaped = NULL;
      if (!self->port->id_.comment_.empty ())
        {
          comment_escaped = g_markup_printf_escaped (
            "<i>%s</i>\n", self->port->id_.comment_.c_str ());
        }
      const char * src_or_dest_str =
        self->port->id_.flow_ == PortFlow::Input
          ? _ ("Incoming signals")
          : _ ("Outgoing signals");
      sprintf (
        str,
        "<b>%s</b>\n"
        "%s"
        "%s: <b>%zu</b>\n"
        "%s: <b>%f</b>\n"
        "%s: <b>%f</b> | "
        "%s: <b>%f</b>",
        full_designation_escaped, comment_escaped ? comment_escaped : "",
        src_or_dest_str,
        self->port->id_.flow_ == PortFlow::Input
          ? self->port->srcs_.size ()
          : self->port->dests_.size (),
        _ ("Current val"), (double) get_port_value (self), _ ("Min"),
        (double) self->minf, _ ("Max"), (double) self->maxf);
      g_free (full_designation_escaped);
      g_free_and_null (comment_escaped);

      /* if the tooltip changed, update it */
      const char * cur_tooltip = gtk_widget_get_tooltip_markup (widget);
      if (!string_is_equal (cur_tooltip, str))
        {
          /* FIXME reenable when GTK bug is fixed */
          /*gtk_widget_set_tooltip_markup (*/
          /*widget, str);*/
        }

      /* remember time */
      self->last_tooltip_change = now;
    }
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

InspectorPortWidget *
inspector_port_widget_new (Port * port)
{
  InspectorPortWidget * self = static_cast<InspectorPortWidget *> (
    g_object_new (INSPECTOR_PORT_WIDGET_TYPE, nullptr));

  self->port = port;
  self->meter = std::make_unique<Meter> (*port);

  char str[200];
  int  has_str = 0;
  if (!port->has_label ())
    {
      z_warning ("No port label");
      goto inspector_port_new_end;
    }

  has_str = get_port_str (self, port, str);

  if (has_str)
    {
      float minf = port->minf_;
      float maxf = port->maxf_;
      if (port->get_type () == PortType::Audio)
        {
          /* use fader val for audio */
          maxf = 1.f;
        }
      float zerof_ = port->zerof_;
      int   editable = 0;
      int   is_control = 0;
      if (port->get_type () == PortType::Control)
        {
          editable = 1;
          is_control = 1;
        }
      self->bar_slider = _bar_slider_widget_new (
        BarSliderType::BAR_SLIDER_TYPE_NORMAL,
        [self] () { return get_port_value (self); },
        [self] (float val) { set_port_value (self, val); }, self,
        /* use normalized vals for controls */
        is_control ? 0.f : minf, is_control ? 1.f : maxf, -1, 20,
        is_control ? 0.f : zerof_, 0, 2, UiDragMode::UI_DRAG_MODE_CURSOR, str,
        "");
      self->bar_slider->snapped_getter = [self] () {
        return get_snapped_port_value (self);
      };
      self->bar_slider->show_value = 0;
      self->bar_slider->editable = editable;
      self->bar_slider->init_setter = [self] (auto val) {
        set_init_port_value (self, val);
      };
      self->bar_slider->end_setter = [self] (auto val) {
        val_change_finished (self, val);
      };
      gtk_overlay_set_child (self->overlay, GTK_WIDGET (self->bar_slider));
      self->minf = minf;
      self->maxf = maxf;
      self->zerof_ = zerof_;
      self->port_str = str;

      /* keep drawing the bar slider */
      gtk_widget_add_tick_callback (
        GTK_WIDGET (self->bar_slider), (GtkTickCallback) bar_slider_tick_cb,
        self, nullptr);
    }

    /* jack button */
#if HAVE_JACK
  if (AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      if (
        port->get_type () == PortType::Audio
        || port->get_type () == PortType::Event)
        {
          self->jack = z_gtk_toggle_button_new_with_icon ("expose-to-jack");
          gtk_widget_set_halign (GTK_WIDGET (self->jack), GTK_ALIGN_START);
          gtk_widget_set_valign (GTK_WIDGET (self->jack), GTK_ALIGN_CENTER);
          gtk_widget_set_margin_start (GTK_WIDGET (self->jack), 2);
          gtk_widget_add_css_class (GTK_WIDGET (self->jack), "mini-button");
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->jack), _ ("Expose port to JACK"));
          gtk_overlay_add_overlay (self->overlay, GTK_WIDGET (self->jack));
          if (
            port->data_ && port->internal_type_ == Port::InternalType::JackPort)
            {
              gtk_toggle_button_set_active (self->jack, 1);
            }
          g_signal_connect (
            G_OBJECT (self->jack), "toggled", G_CALLBACK (on_jack_toggled),
            self);

          /* add some margin to clearly show the jack
           * button */
          /*GtkWidget * label =*/
          /*gtk_bin_get_child (*/
          /*GTK_BIN (self->menu_button));*/
          /*gtk_widget_set_margin_start (*/
          /*label, 12);*/
        }
    }
#endif

  self->double_click_gesture = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->double_click_gesture), "pressed",
    G_CALLBACK (on_double_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->bar_slider),
    GTK_EVENT_CONTROLLER (self->double_click_gesture));

  self->right_click_gesture = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_click_gesture), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (self->right_click_gesture), "pressed",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->bar_slider),
    GTK_EVENT_CONTROLLER (self->right_click_gesture));

inspector_port_new_end:

  return self;
}

static void
finalize (InspectorPortWidget * self)
{
  std::destroy_at (&self->port_str);
  std::destroy_at (&self->hex_color);
  std::destroy_at (&self->meter);

  G_OBJECT_CLASS (inspector_port_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
dispose (InspectorPortWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->overlay));
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

#define GET_REFCOUNT (((ZGObjectImpl *) self->connections_popover)->ref_count)

  int refcount = (int) GET_REFCOUNT;
  /*z_debug ("refcount: {}", refcount);*/
  gtk_widget_unparent (GTK_WIDGET (self->connections_popover));
  refcount--;

  /* note: this is a hack until GTK bug #4599 is
   * resolved */
  while (refcount > 0)
    {
      refcount = (int) GET_REFCOUNT;
      z_debug ("unrefing... refcount: {}", refcount);
      g_object_unref (self->connections_popover);
      refcount--;
    }

#undef GET_REFCOUNT

  G_OBJECT_CLASS (inspector_port_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
inspector_port_widget_class_init (InspectorPortWidgetClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;

  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
}

static void
inspector_port_widget_init (InspectorPortWidget * self)
{
  std::construct_at (&self->port_str);
  std::construct_at (&self->hex_color);
  std::construct_at (&self->meter);

  self->overlay = GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_parent (GTK_WIDGET (self->overlay), GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->connections_popover =
    port_connections_popover_widget_new (GTK_WIDGET (self));
  gtk_widget_set_parent (
    GTK_WIDGET (self->connections_popover), GTK_WIDGET (self));
  /*g_object_ref_sink (self->connections_popover);*/
  /*g_object_ref (self->connections_popover);*/

  self->hex_color = UI_COLORS->bright_orange.to_hex ();
}
