GTK Tips
========

# dispose() vs finalize() vs destroy()

According to ebassi from GTK:
Rule of thumb: use dispose()
for releasing references to objects you acquired
from the outside, and finalize() to release
memory/references you allocated internally.

There's basically no reason to
override `destroy()`; always use `dispose`/`finalize`.
The "destroy" signal is for
other code, using your widget, to release
references they might have.

# map() vs realize()

Alexander Mikhaylenko:
```
map - if you put it into a stack and switch pages,
it will map/unmap every time, but will still be
realized the whole time
realized == when it's in a realized window iirc
but basically you can access the window and you have access to rendering stuff
mapped == when it's actually gonna be drawn, yes
```

# Popovers
Thanks to ebassi from GTK:
always make sure that the popover opens inside
the window; make the window request a minimum
size that allows the popover to be fully
contained; or make the contents of the popover
be small enough to fit into a small top level
parent window

# Specify menu for GtkMenuButton in XML

Specify the id of the menu, like:

```xml
<property name="menu">my_menu_id</property>
```

# Set max height on GtkScrolledWindow

Set `propagate-natural-height` to true and set
max height with `max-content-height`.

# GDK key defines and accel strings

- `include/gtk-3.0/gdk/gdkkeysyms.h`
- `gdk_keyval_name()`
- `gdk_keyval_from_name()`

# Showing popups on startup

First need to set style provider in
`zrythm_app_startup()` otherwise we get segfaults.

# Custom widget text node styling

mclasen:
```
if you call append_layout, you provide the pango layout that specifies what fonts to use
if it is a pango layout created with gtk_widget_create_pango_layout then yes, css may influence what font is used
```

<!---
SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->
