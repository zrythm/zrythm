/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

/******************************************************************************
 * C data structure for storing a project plan.
 *
 * This is what we want to load the YAML into.
 ******************************************************************************/

/* Structure for storing numerical sequence */
struct numbers {
	char *name;
	int *data;
	unsigned data_count;
};


/******************************************************************************
 * CYAML schema to tell libcyaml about both expected YAML and data structure.
 *
 * (Our CYAML schema is just a bunch of static const data.)
 ******************************************************************************/

/* CYAML value schema for entries of the data sequence. */
static const cyaml_schema_value_t data_entry = {
	CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
};

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

/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t top_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			struct numbers, top_mapping_schema),
};


/******************************************************************************
 * Actual code to load and save YAML doc using libcyaml.
 ******************************************************************************/

/* Our CYAML config.
 *
 * If you want to change it between calls, don't make it const.
 *
 * Here we have a very basic config.
 */
static const cyaml_config_t config = {
	.log_level = CYAML_LOG_WARNING, /* Logging errors and warnings only. */
	.log_fn = cyaml_log,            /* Use the default logging function. */
	.mem_fn = cyaml_mem,            /* Use the default memory allocator. */
};

/* Main entry point from OS. */
int main(int argc, char *argv[])
{
	cyaml_err_t err;
	struct numbers *n;
	enum {
		ARG_PROG_NAME,
		ARG_PATH_IN,
		ARG__COUNT,
	};

	/* Handle args */
	if (argc != ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s <INPUT>\n", argv[ARG_PROG_NAME]);
		return EXIT_FAILURE;
	}

	/* Load input file. */
	err = cyaml_load_file(argv[ARG_PATH_IN], &config,
			&top_schema, (cyaml_data_t **)&n, NULL);
	if (err != CYAML_OK) {
		fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
		return EXIT_FAILURE;
	}

	/* Use the data. */
	printf("%s:\n", n->name);
	for (unsigned i = 0; i < n->data_count; i++) {
		printf("  - %i\n", n->data[i]);
	}

	/* Free the data */
	cyaml_free(&config, &top_schema, n, 0);

	return EXIT_SUCCESS;
}
