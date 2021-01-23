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

 ----

Copyright (C) 2019-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification, are permitted in any medium without royalty provided the copyright notice and this notice are preserved. This file is offered as-is, without any warranty.
