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

/* Enumeration of months of the year */
enum months {
	MONTH_JAN = 1,
	MONTH_FEB,
	MONTH_MAR,
	MONTH_APR,
	MONTH_MAY,
	MONTH_JUN,
	MONTH_JUL,
	MONTH_AUG,
	MONTH_SEP,
	MONTH_OCT,
	MONTH_NOV,
	MONTH_DEC
};

/* Structure for storing dates */
struct date {
	uint8_t day;
	enum months month;
	uint16_t year;
};

/* Structure for storing durations */
struct duration {
	uint8_t hours;
	uint16_t days;
	uint16_t weeks;
	uint16_t years;
};

/* Enumeration of task flags */
enum task_flags {
	FLAGS_NONE          = 0,
	FLAGS_IMPORTANT     = (1 << 0),
	FLAGS_ENGINEERING   = (1 << 1),
	FLAGS_DOCUMENTATION = (1 << 2),
	FLAGS_MANAGEMENT    = (1 << 3),
};

/* Structure for storing a task */
struct task {
	const char *name;
	enum task_flags flags;
	struct duration estimate;

	const char **depends;
	unsigned depends_count;

	const char **people;
	unsigned n_people;
};

/* Top-level structure for storing a plan */
struct plan {
	const char *name;
	struct date *start;

	const char **people;
	unsigned n_people;

	struct task *tasks;
	uint64_t tasks_count;
};


/******************************************************************************
 * CYAML schema to tell libcyaml about both expected YAML and data structure.
 *
 * (Our CYAML schema is just a bunch of static const data.)
 ******************************************************************************/

/* Mapping from "task_flags" strings to enum values for schema. */
static const cyaml_strval_t task_flags_strings[] = {
	{ "None",          FLAGS_NONE          },
	{ "Important",     FLAGS_IMPORTANT     },
	{ "Engineering",   FLAGS_ENGINEERING   },
	{ "Documentation", FLAGS_DOCUMENTATION },
	{ "Management",    FLAGS_MANAGEMENT    },
};

/* Mapping from "month" strings to flag values for schema. */
static const cyaml_strval_t month_strings[] = {
	{ "January",   MONTH_JAN },
	{ "February",  MONTH_FEB },
	{ "March",     MONTH_MAR },
	{ "April",     MONTH_APR },
	{ "May",       MONTH_MAY },
	{ "June",      MONTH_JUN },
	{ "July",      MONTH_JUL },
	{ "August",    MONTH_AUG },
	{ "September", MONTH_SEP },
	{ "October",   MONTH_OCT },
	{ "November",  MONTH_NOV },
	{ "December",  MONTH_DEC },
};

/* Schema for string pointer values (used in sequences of strings). */
static const cyaml_schema_value_t string_ptr_schema = {
	CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
};

/* The duration mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, `struct duration`.
 */
static const cyaml_schema_field_t duration_fields_schema[] = {
	/* The fields here are all optional unsigned integers.
	 *
	 * Note that if an optional field is unset in the YAML, its value
	 * will be zero in the C data structure.
	 *
	 * In all cases here, the YAML mapping key name (first parameter to
	 * the macros) matches the name of the associated member of the
	 * `duration` structure (fourth parameter).
	 */
	CYAML_FIELD_UINT(
			"hours", CYAML_FLAG_OPTIONAL,
			struct duration, hours),
	CYAML_FIELD_UINT(
			"days", CYAML_FLAG_OPTIONAL,
			struct duration, days),
	CYAML_FIELD_UINT(
			"weeks", CYAML_FLAG_OPTIONAL,
			struct duration, weeks),
	CYAML_FIELD_UINT(
			"years", CYAML_FLAG_OPTIONAL,
			struct duration, years),

	/* The field array must be terminated by an entry with a NULL key.
	 * Here we use the CYAML_FIELD_END macro for that. */
	CYAML_FIELD_END
};

/* The date mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, `struct date`.
 */
static const cyaml_schema_field_t date_fields_schema[] = {
	/* The "day" and "year" fields are just normal UNIT CYAML types.
	 * See the comments for duration_fields_schema for details.
	 * The only difference is neither of these are optional.
	 * Note: The order of the fields in this array doesn't matter.
	 */
	CYAML_FIELD_UINT(
			"day", CYAML_FLAG_DEFAULT,
			struct date, day),

	CYAML_FIELD_UINT(
			"year", CYAML_FLAG_DEFAULT,
			struct date, year),

	/* The month field is an enum.
	 *
	 * YAML key: "month".
	 * C structure member for this key: "month".
	 * Array mapping strings to values: month_strings
	 *
	 * Its CYAML type is ENUM, so an array of cyaml_strval_t must be
	 * provided to map from string to values.
	 * Note that we're not setting the strict flag here so both strings and
	 * numbers are accepted in the YAML.  (For example, both "4" and "April"
	 * would be accepted.)
	 */
	CYAML_FIELD_ENUM(
			"month", CYAML_FLAG_DEFAULT,
			struct date, month, month_strings,
			CYAML_ARRAY_LEN(month_strings)),

	/* The field array must be terminated by an entry with a NULL key.
	 * Here we use the CYAML_FIELD_END macro for that. */
	CYAML_FIELD_END
};

/* The task mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, `struct task`.
 */
static const cyaml_schema_field_t task_fields_schema[] = {
	/* The first field in the mapping is a task name.
	 *
	 * YAML key: "name".
	 * C structure member for this key: "name".
	 *
	 * Its CYAML type is string pointer, and we have no minimum or maximum
	 * string length constraints.
	 */
	CYAML_FIELD_STRING_PTR(
			"name", CYAML_FLAG_POINTER,
			struct task, name, 0, CYAML_UNLIMITED),

	/* The flags field is a flags value.
	 *
	 * YAML key: "flags".
	 * C structure member for this key: "flags".
	 * Array mapping strings to values: task_flags_strings
	 *
	 * In the YAML a CYAML flags value should be a sequence of scalars.
	 * The values of each set scalar is looked up the in array of
	 * string/value mappings, and the values are bitwise ORed together.
	 *
	 * Note that we're setting the strict flag here so only strings
	 * present in task_flags_strings are allowed, and numbers are not.
	 *
	 * We make the field optional so when there are no flags set, the field
	 * can be omitted from the YAML.
	 */
	CYAML_FIELD_FLAGS(
			"flags", CYAML_FLAG_OPTIONAL | CYAML_FLAG_STRICT,
			struct task, flags, task_flags_strings,
			CYAML_ARRAY_LEN(task_flags_strings)),

	/* The next field is the task estimate.
	 *
	 * YAML key: "estimate".
	 * C structure member for this key: "estimate".
	 *
	 * Its CYAML type is a mapping.
	 *
	 * Since it's a mapping type, the schema for its mapping's fields must
	 * be provided too.  In this case, it's `duration_fields_schema`.
	 */
	CYAML_FIELD_MAPPING(
			"estimate", CYAML_FLAG_DEFAULT,
			struct task, estimate, duration_fields_schema),

	/* The next field is the tasks that this task depends on.
	 *
	 * YAML key: "depends".
	 * C structure member for this key: "depends".
	 *
	 * Its CYAML type is a sequence.
	 *
	 * Since it's a sequence type, the value schema for its entries must
	 * be provided too.  In this case, it's string_ptr_schema.
	 *
	 * Since it's not a sequence of a fixed-length, we must tell CYAML
	 * where the sequence entry count is to be stored.  In this case, it
	 * goes in the "depends_count" C structure member in `struct task`.
	 * Since this is "depends" with the "_count" postfix, we can use
	 * the following macro, which assumes a postfix of "_count" in the
	 * struct member name.
	 */
	CYAML_FIELD_SEQUENCE(
			"depends", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct task, depends,
			&string_ptr_schema, 0, CYAML_UNLIMITED),

	/* The next field is the task people.
	 *
	 * YAML key: "people".
	 * C structure member for this key: "people".
	 *
	 * Its CYAML type is a sequence.
	 *
	 * Since it's a sequence type, the value schema for its entries must
	 * be provided too.  In this case, it's string_ptr_schema.
	 *
	 * Since it's not a sequence of a fixed-length, we must tell CYAML
	 * where the sequence entry count is to be stored.  In this case, it
	 * goes in the "n_people" C structure member in `struct plan`.
	 */
	CYAML_FIELD_SEQUENCE_COUNT(
			"people", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
			struct task, people, n_people,
			&string_ptr_schema, 0, CYAML_UNLIMITED),

	/* The field array must be terminated by an entry with a NULL key.
	 * Here we use the CYAML_FIELD_END macro for that. */
	CYAML_FIELD_END
};

/* The value for a task is a mapping.
 *
 * Its fields are defined in task_fields_schema.
 */
static const cyaml_schema_value_t task_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
			struct task, task_fields_schema),
};

/* The plan mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, `struct plan`.
 */
static const cyaml_schema_field_t plan_fields_schema[] = {
	/* The first field in the mapping is a project name.
	 *
	 * YAML key: "project".
	 * C structure member for this key: "name".
	 *
	 * Its CYAML type is string pointer, and we have no minimum or maximum
	 * string length constraints.
	 */
	CYAML_FIELD_STRING_PTR(
			"project", CYAML_FLAG_POINTER,
			struct plan, name, 0, CYAML_UNLIMITED),

	/* The next field is the project start date.
	 *
	 * YAML key: "start".
	 * C structure member for this key: "start".
	 *
	 * Its CYAML type is a mapping pointer.
	 *
	 * Since it's a mapping type, the schema for its mapping's fields must
	 * be provided too.  In this case, it's `date_fields_schema`.
	 */
	CYAML_FIELD_MAPPING_PTR(
			"start", CYAML_FLAG_POINTER,
			struct plan, start, date_fields_schema),

	/* The next field is the project people.
	 *
	 * YAML key: "people".
	 * C structure member for this key: "people".
	 *
	 * Its CYAML type is a sequence.
	 *
	 * Since it's a sequence type, the value schema for its entries must
	 * be provided too.  In this case, it's string_ptr_schema.
	 *
	 * Since it's not a sequence of a fixed-length, we must tell CYAML
	 * where the sequence entry count is to be stored.  In this case, it
	 * goes in the "n_people" C structure member in `struct plan`.
	 */
	CYAML_FIELD_SEQUENCE_COUNT(
			"people", CYAML_FLAG_POINTER,
			struct plan, people, n_people,
			&string_ptr_schema, 0, CYAML_UNLIMITED),

	/* The next field is the project tasks.
	 *
	 * YAML key: "tasks".
	 * C structure member for this key: "tasks".
	 *
	 * Its CYAML type is a sequence.
	 *
	 * Since it's a sequence type, the value schema for its entries must
	 * be provided too.  In this case, it's task_schema.
	 *
	 * Since it's not a sequence of a fixed-length, we must tell CYAML
	 * where the sequence entry count is to be stored.  In this case, it
	 * goes in the "tasks_count" C structure member in `struct plan`.
	 * Since this is "tasks" with the "_count" postfix, we can use
	 * the following macro, which assumes a postfix of "_count" in the
	 * struct member name.
	 */
	CYAML_FIELD_SEQUENCE(
			"tasks", CYAML_FLAG_POINTER,
			struct plan, tasks,
			&task_schema, 0, CYAML_UNLIMITED),

	/* If the YAML contains a field that our program is not interested in
	 * we can ignore it, so the value of the field will not be loaded.
	 *
	 * Note that unless the OPTIONAL flag is set, the ignored field must
	 * be present.
	 *
	 * Another way of handling this would be to use the config flag
	 * to ignore unknown keys.  This config is passed to libcyaml
	 * separately from the schema.
	 */
	CYAML_FIELD_IGNORE("irrelevant", CYAML_FLAG_OPTIONAL),

	/* The field array must be terminated by an entry with a NULL key.
	 * Here we use the CYAML_FIELD_END macro for that. */
	CYAML_FIELD_END
};

/* Top-level schema.  The top level value for the plan is a mapping.
 *
 * Its fields are defined in plan_fields_schema.
 */
static const cyaml_schema_value_t plan_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			struct plan, plan_fields_schema),
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
	struct plan *plan;
	enum {
		ARG_PROG_NAME,
		ARG_PATH_IN,
		ARG_PATH_OUT,
		ARG__COUNT,
	};

	/* Handle args */
	if (argc != ARG__COUNT) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s <INPUT> <OUTPUT>\n", argv[ARG_PROG_NAME]);
		return EXIT_FAILURE;
	}

	/* Load input file. */
	err = cyaml_load_file(argv[ARG_PATH_IN], &config,
			&plan_schema, (void **) &plan, NULL);
	if (err != CYAML_OK) {
		fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
		return EXIT_FAILURE;
	}

	/* Use the data. */
	printf("Project: %s\n", plan->name);
	for (unsigned i = 0; i < plan->tasks_count; i++) {
		printf("%u. %s\n", i + 1, plan->tasks[i].name);
	}

	/* Modify the data */
	plan->tasks[0].estimate.days += 3;
	plan->tasks[0].estimate.weeks += 1;

	/* Save data to new YAML file. */
	err = cyaml_save_file(argv[ARG_PATH_OUT], &config,
			&plan_schema, plan, 0);
	if (err != CYAML_OK) {
		fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
		cyaml_free(&config, &plan_schema, plan, 0);
		return EXIT_FAILURE;
	}

	/* Free the data */
	cyaml_free(&config, &plan_schema, plan, 0);

	return EXIT_SUCCESS;
}
