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

# Specify menu for GtkMenuButton in XML

Specify the id of the menu, like:

```xml
<property name="menu">my_menu_id</property>
```

 ----

Copyright (C) 2019-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification, are permitted in any medium without royalty provided the copyright notice and this notice are preserved. This file is offered as-is, without any warranty.
