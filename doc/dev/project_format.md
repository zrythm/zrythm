Project Format
==============

# Schemas

Use `*_create_from_v1()` to get an instance of the current
struct with no changes from the struct image under inc/schemas.
This is used because we can't simply cast (the current instance
might have more members that are not serialized) and because
the headers under inc/schemas should be standalone (should not
include currently used structs from e.g. under inc/dsp).

## Upgrading
Use `*_upgrade_from_v1()` to upgrade an old instance (with
different members) to the next version. If the next version is
the current version, then it should return the normal struct
(e.g., `Project *` not `Project_v2`).

This means that when the
current struct changes, the previous `upgrade_from` function
should be edited as well.

The caller is responsible for freeing the old struct argument
(e.g., with `cyaml_free()`).
