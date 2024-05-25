// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/custom_image.h"
#include "utils/gtk.h"

G_DEFINE_TYPE (CustomImageWidget, custom_image_widget, GTK_TYPE_WIDGET)

static void
custom_image_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  CustomImageWidget * self = Z_CUSTOM_IMAGE_WIDGET (widget);

  {
    graphene_rect_t tmp_r = GRAPHENE_RECT_INIT (
      0.f, 0.f, (float) gdk_texture_get_width (self->texture),
      (float) gdk_texture_get_height (self->texture));
    gtk_snapshot_append_texture (snapshot, self->texture, &tmp_r);
  }
}

/**
 * Sets the texture.
 *
 * Takes ownership of texture.
 */
void
custom_image_widget_set_texture (CustomImageWidget * self, GdkTexture * texture)
{
  self->texture = texture;

  gtk_widget_set_size_request (
    GTK_WIDGET (self), gdk_texture_get_width (texture),
    gdk_texture_get_height (texture));

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
custom_image_widget_init (CustomImageWidget * self)
{
}

static void
custom_image_widget_class_init (CustomImageWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  klass->snapshot = custom_image_snapshot;
  gtk_widget_class_set_css_name (klass, "custom-image");

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_PRESENTATION);
}
