LibCYAML: Tutorial
==================

This document is intended for C developers wishing to make use of
[LibCYAML](https://github.com/tlsa/libcyaml).

Overview
--------

If you want to use LibCYAML you'll need to have two things:

1. A consistent structure to the sort of YAML you want to load/save.
2. Some C data structure to load/save to/from.

LibCYAML's aim is to make this as simple as possible for the programmer.
However, LibCYAML knows nothing about either your data structure or the
"shape" of the YAML you want to load.  You provide this information by
defining "schemas", and passing them to LibCYAML.

> **Note**: If you need to handle arbitrary "free-form" YAML (e.g. for a tool
> to convert between YAML and JSON), then LibCYAML would not be much help.
> In such a case, I'd recommend using [libyaml](https://github.com/yaml/libyaml)
> directly.

A simple example: loading YAML
------------------------------

Let's say you want to load the following YAML document:

```yaml
name: Fibonacci
data:
  - 1
  - 1
  - 2
  - 3
  - 5
  - 8
```

And you want to load it into the following C data structure:

```c
struct numbers {
	char *name;
	int *data;
	unsigned data_count;
};
```

Then we need to define a CYAML schema to describe these to LibCYAML.

> **Note**: Use the doxygen API documentation, or else the documentation in
> [cyaml.h](https://github.com/tlsa/libcyaml/blob/master/include/cyaml/cyaml.h)
> in conjunction with this guide.

At the top level of the YAML is a mapping with two fields, "name" and
"data".  The the first field is just a simple scalar value (it's neither
a mapping nor a sequence).  The second field has a sequence value.

We'll start by defining the CYAML schema for the "data" sequence,
since since that's the "deepest" non-scalar type.  The reason for
starting here will become clear later.

```c
/* CYAML value schema for entries of the data sequence. */
static const cyaml_schema_value_t data_entry = {
	CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
};
```

Here we're making a `cyaml_schema_value_t` for the entries in the
sequence.  There are various `CYAML_VALUE_{TYPE}` macros to assist with
this.  Here we're using `CYAML_VALUE_INT`, because the value is a signed
integer.  The parameters passed to the macro are `enum cyaml_flag`, and
the C data type of the value.

Now we can write the schema for the mapping.  First we'll construct
an array of `cyaml_schema_field_t` entries that describe each
field in the mapping.

```c
/* CYAML mapping schema fields array for the top level mapping. */
static const cyaml_schema_field_t top_mapping_schema[] = {
	CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER,
			struct numbers, name,
			0, CYAML_UNLIMITED),
	CYAML_FIELD_SEQUENCE("data", CYAML_FLAG_POINTER,
			struct numbers, data, &data_entry,
			0, CYAML_UNLIMITED),
	CYAML_FIELD_END
};
```

There are `CYAML_FIELD_{TYPE}` helper macros to construct the mapping field
entries.  The array must be terminated by a `CYAML_FIELD_END` entry.
The helper macro parameters are specific to each `CYAML_FIELD_{TYPE}` macro.

The entry for the name field is of type string pointer.  You can consult the
documentation for the `CYAML_FIELD_{TYPE}` macros to see what the parameters
mean.

> **Note**: The field for the sequence takes a pointer to the sequence entry
> data type that we defined earlier as `data_entry`.

Finally we can define the schema for the top level value that gets passed to
the LibCYAML.

```c
/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t top_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			struct numbers, top_mapping_schema),
};
```

In this case our top level value is a mapping type.  One of the parameters
needed for mappings is the array of field definitions.  In this case we're
passing the `top_mapping_schema` that we defined above.

```c
/* Create our CYAML configuration. */
static const cyaml_config_t config = {
	.log_level = CYAML_LOG_WARNING, /* Logging errors and warnings only. */
	.log_fn = cyaml_log,            /* Use the default logging function. */
	.mem_fn = cyaml_mem,            /* Use the default memory allocator. */
};

/* Where to store the loaded data */
struct numbers *n;

/* Load the file into p */
cyaml_err_t err = cyaml_load_file(argv[ARG_PATH_IN], &config,
		&top_schema, (cyaml_data_t **)&n, NULL);
if (err != CYAML_OK) {
	/* Handle error */
}

/* Use the data. */
printf("%s:\n", n->name);
for (unsigned i = 0; i < n->data_count; i++) {
	printf("  - %i\n", n->data[i]);
}

/* Free the data */
err = cyaml_free(&config, &top_schema, n, 0);
if (err != CYAML_OK) {
	/* Handle error */
}
```

And that's it, the YAML is loaded into the custom C data structure.

You can find the code for this in the project's [examples](../examples)
directory.
