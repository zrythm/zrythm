/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

#include "ttest.h"

/**
 * Unit test context data.
 */
typedef struct test_data {
	cyaml_data_t **data;
	unsigned *seq_count;
	const struct cyaml_config *config;
	const struct cyaml_schema_value *schema;
} test_data_t;

/**
 * Common clean up function to free data loaded by tests.
 *
 * \param[in]  data  The unit test context data.
 */
static void cyaml_cleanup(void *data)
{
	struct test_data *td = data;
	unsigned seq_count = 0;

	if (td->seq_count != NULL) {
		seq_count = *(td->seq_count);
	}

	if (td->data != NULL) {
		cyaml_free(td->config, td->schema, *(td->data), seq_count);
	}
}

/**
 * Test loading a non-existent file.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_file_load_bad_path(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		char *cakes;
	} *data_tgt = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_file("/cyaml/path/shouldn't/exist.yaml",
			config, &top_schema, (cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_FILE_OPEN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading a non-existent file.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_file_save_bad_path(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct target_struct {
		char *cakes;
	} *data = NULL;
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = NULL,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_file("/cyaml/path/shouldn't/exist.yaml",
			config, &top_schema, data, 0);
	if (err != CYAML_ERR_FILE_OPEN) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading the basic YAML file.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_file_load_basic(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct animal {
		char *kind;
		char **sounds;
		unsigned sounds_count;
	};
	struct target_struct {
		struct animal *animals;
		unsigned animals_count;
		char **cakes;
		unsigned cakes_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sounds_entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field animal_mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("sounds", CYAML_FLAG_POINTER,
				struct animal, sounds,
				&sounds_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animals_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct animal, animal_mapping_schema),
	};
	static const struct cyaml_schema_value cakes_entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animals,
				&animals_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("cakes", CYAML_FLAG_POINTER,
				struct target_struct, cakes,
				&cakes_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_file("test/data/basic.yaml", config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading and then saving the basic YAML file.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_file_load_save_basic(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct animal {
		char *kind;
		char **sounds;
		unsigned sounds_count;
	};
	struct target_struct {
		struct animal *animals;
		unsigned animals_count;
		char **cakes;
		unsigned cakes_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sounds_entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field animal_mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("sounds", CYAML_FLAG_POINTER,
				struct animal, sounds,
				&sounds_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animals_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct animal, animal_mapping_schema),
	};
	static const struct cyaml_schema_value cakes_entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animals,
				&animals_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("cakes", CYAML_FLAG_POINTER,
				struct target_struct, cakes,
				&cakes_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_file("test/data/basic.yaml", config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	err = cyaml_save_file("build/load_save.yaml", config, &top_schema,
				data_tgt, 0);
	if (err != CYAML_OK) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test loading the basic YAML file, with a mismatching schema.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_file_load_basic_invalid(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	struct animal {
		char *kind;
		int *sounds;
		unsigned sounds_count;
	};
	struct target_struct {
		struct animal *animals;
		unsigned animals_count;
		char **cakes;
		unsigned cakes_count;
	} *data_tgt = NULL;
	static const struct cyaml_schema_value sounds_entry_schema = {
		/* The data has a string, but we're expecting int here. */
		CYAML_VALUE_INT(CYAML_FLAG_DEFAULT, int),
	};
	static const struct cyaml_schema_field animal_mapping_schema[] = {
		CYAML_FIELD_STRING_PTR("kind", CYAML_FLAG_POINTER,
				struct animal, kind, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("sounds", CYAML_FLAG_POINTER,
				struct animal, sounds,
				&sounds_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value animals_entry_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
				struct animal, animal_mapping_schema),
	};
	static const struct cyaml_schema_value cakes_entry_schema = {
		CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		CYAML_FIELD_SEQUENCE("animals", CYAML_FLAG_POINTER,
				struct target_struct, animals,
				&animals_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_SEQUENCE("cakes", CYAML_FLAG_POINTER,
				struct target_struct, cakes,
				&cakes_entry_schema, 0, CYAML_UNLIMITED),
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.data = (cyaml_data_t **) &data_tgt,
		.config = config,
		.schema = &top_schema,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_load_file("test/data/basic.yaml", config, &top_schema,
			(cyaml_data_t **) &data_tgt, NULL);
	if (err != CYAML_ERR_INVALID_VALUE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Test saving to a file when an erro occurs.
 *
 * \param[in]  report  The test report context.
 * \param[in]  config  The CYAML config to use for the test.
 * \return true if test passes, false otherwise.
 */
static bool test_file_save_basic_invalid(
		ttest_report_ctx_t *report,
		const cyaml_config_t *config)
{
	static const struct target_struct {
		int value;
	} data = {
		.value = 9,
	};
	static const struct cyaml_schema_field mapping_schema[] = {
		{
			.key = "key",
			.value = {
				.type = CYAML_INT,
				.flags = CYAML_FLAG_DEFAULT,
				.data_size = 0,
			},
			.data_offset = offsetof(struct target_struct, value),
		},
		CYAML_FIELD_END
	};
	static const struct cyaml_schema_value top_schema = {
		CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
				struct target_struct, mapping_schema),
	};
	test_data_t td = {
		.config = config,
	};
	cyaml_err_t err;

	ttest_ctx_t tc = ttest_start(report, __func__, cyaml_cleanup, &td);

	err = cyaml_save_file("build/save.yaml", config, &top_schema, &data, 0);
	if (err != CYAML_ERR_INVALID_DATA_SIZE) {
		return ttest_fail(&tc, cyaml_strerror(err));
	}

	return ttest_pass(&tc);
}

/**
 * Run the YAML file tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool file_tests(
		ttest_report_ctx_t *rc,
		cyaml_log_t log_level,
		cyaml_log_fn_t log_fn)
{
	bool pass = true;
	cyaml_config_t config = {
		.log_fn = log_fn,
		.mem_fn = cyaml_mem,
		.log_level = log_level,
		.flags = CYAML_CFG_DEFAULT,
	};

	ttest_heading(rc, "File loading tests");

	pass &= test_file_load_basic(rc, &config);
	pass &= test_file_load_save_basic(rc, &config);

	/* Since we expect loads of error logging for these tests,
	 * suppress log output if required log level is greater
	 * than \ref CYAML_LOG_INFO.
	 */
	if (log_level > CYAML_LOG_INFO) {
		config.log_fn = NULL;
	}

	pass &= test_file_load_bad_path(rc, &config);
	pass &= test_file_save_bad_path(rc, &config);
	pass &= test_file_load_basic_invalid(rc, &config);
	pass &= test_file_save_basic_invalid(rc, &config);

	return pass;
}
