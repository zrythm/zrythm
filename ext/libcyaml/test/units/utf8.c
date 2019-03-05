/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

#include "../../src/utf8.h"

#include "ttest.h"

/** Helper macro to squash unused variable warnings. */
#define UNUSED(_x) ((void)(_x))

/** Helper macro to get the length of string string literals. */
#define SLEN(_s) (CYAML_ARRAY_LEN(_s) - 1)

/**
 * Test utf-8 decoding.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_utf8_get_codepoint(
		ttest_report_ctx_t *report)
{
	static const struct tests {
		unsigned c;
		const char *s;
		unsigned l;
	} t[] = {
		{ 0xfffd, "\ufffd", SLEN("\ufffd") },
		{ 0xfffd, "\xC1\x9C", SLEN("\xC1\x9C") },
		{ 0x1f638, u8"😸", SLEN(u8"😸") },
		{ 0xfffd, u8"😸", 0 },
		{ 0xfffd, u8"😸", 5 },
	};
	bool pass = true;

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(t); i++) {
		unsigned l;
		unsigned c;
		ttest_ctx_t tc;
		char name[sizeof(__func__) + 32];
		sprintf(name, "%s_%u", __func__, i);

		tc = ttest_start(report, name, NULL, NULL);

		l = t[i].l;
		c = cyaml_utf8_get_codepoint((uint8_t *)t[i].s, &l);
		if (c != t[i].c) {
			pass &= ttest_fail(&tc, "Incorrect codepoint for %s "
					"(expecting %4.4x, got %4.4x)",
					t[i].s, t[i].c, c);
			continue;
		}

		pass &= ttest_pass(&tc);
	}

	return pass;
}

/**
 * Test comparing the same strings.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_utf8_strcmp_same(
		ttest_report_ctx_t *report)
{
	const char *strings[] = {
		"Simple",
		"test",
		"This is a LONGER string, if you see what I mean.",
		"29087   lsdkfj  </,.{}'#\"|@>",
		u8"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß",
		u8"àáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ",
		u8"¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿",
		"\xc3\0",
		u8"αβγδε",
		u8"¯\\_(ツ)_/¯",
		"\xfa",
		u8"😸",
	};
	bool pass = true;

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(strings); i++) {
		ttest_ctx_t tc;
		char name[sizeof(__func__) + 32];
		sprintf(name, "%s_%u", __func__, i);

		tc = ttest_start(report, name, NULL, NULL);

		if (cyaml_utf8_casecmp(strings[i], strings[i]) != 0) {
			pass &= ttest_fail(&tc, "Failed to match: %s",
					strings[i]);
			continue;
		}

		pass &= ttest_pass(&tc);
	}

	return pass;
}

/**
 * Test comparing strings that match.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_utf8_strcmp_matches(
		ttest_report_ctx_t *report)
{
	static const struct string_pairs {
		const char *a;
		const char *b;
	} pairs[] = {
		{ "", "" },
		{ "This is a TEST", "this is A test" },
		{ u8"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ", u8"àáâãäåæçèéêëìíîïðñòóôõö" },
		{ u8"ĀĂĄĆĈĊČĎĐĒĔĖĘĚĜĞ", u8"āăąćĉċčďđēĕėęěĝğ" },
		{ u8"ĳĵķĸĺļľ", u8"ĲĴĶĸĹĻĽ" },
		{ u8"ŊŌŎŐŒŔŖŘŚŜŞŠŢŤŦŨŪŬŮŰŲŴŶ", u8"ŋōŏőœŕŗřśŝşšţťŧũūŭůűųŵŷ" },
		{ u8"űųŵŷŸźżž", u8"űųŵŷÿŹŻŽ" },
		{ u8"ƂƄ ơƣƥ", u8"ƃƅ ƠƢƤ" },
		{ u8"ǞǠǢǤǦǨǪǬǮ", u8"ǟǡǣǥǧǩǫǭǯ" },
		{ u8"ǸǺǼǾȀȂȄȆȈȊȌȎȐȒȔȖȘȚȜȞ", u8"ǹǻǽǿȁȃȅȇȉȋȍȏȑȓȕȗșțȝȟ" },
		{ u8"ȢȤȦȨȪȬȮȰȲ", u8"ȣȥȧȩȫȭȯȱȳ" },
		{ u8"ɇɉɋɍɏ", u8"ɆɈɊɌɎ" },
		{ u8"ƯźżžƳƵ", u8"ưŹŻŽƴƶ" },
		{ u8"ǍǏǑǓǕǗǙǛ", u8"ǎǐǒǔǖǘǚǜ" },
		{ u8"\u0178", u8"\u00ff" },
		{ u8"\u0187", u8"\u0188" },
		{ u8"\u018b", u8"\u018c" },
		{ u8"\u018e", u8"\u01dd" },
		{ u8"\u0191", u8"\u0192" },
		{ u8"\u0198", u8"\u0199" },
		{ u8"\u01a7", u8"\u01a8" },
		{ u8"\u01ac", u8"\u01ad" },
		{ u8"\u01af", u8"\u01b0" },
		{ u8"\u01b7", u8"\u0292" },
		{ u8"\u01b8", u8"\u01b9" },
		{ u8"\u01bc", u8"\u01bd" },
		{ u8"\u01c4", u8"\u01c6" },
		{ u8"\u01c5", u8"\u01c6" },
		{ u8"\u01c7", u8"\u01c9" },
		{ u8"\u01c8", u8"\u01c9" },
		{ u8"\u01ca", u8"\u01cc" },
		{ u8"\u01cb", u8"\u01cc" },
		{ u8"\u01f1", u8"\u01f3" },
		{ u8"\u01f2", u8"\u01f3" },
		{ u8"\u01f4", u8"\u01f5" },
		{ u8"\u01f7", u8"\u01bf" },
		{ u8"\u0220", u8"\u019e" },
		{ u8"\u023b", u8"\u023c" },
		{ u8"\u023d", u8"\u019a" },
		{ u8"\u0241", u8"\u0242" },
		{ u8"\u0243", u8"\u0180" },
		{ "\xF0\x9F\x98\xB8", "\xF0\x9F\x98\xB8" },
		{ "\xF0\x00\x98\xB8", "\xF0\x00\x98\xB8" },
		{ "\xF0\x9F\x00\xB8", "\xF0\x9F\x00\xB8" },
		{ "\xF0\x9F\x98\x00", "\xF0\x9F\x98\x00" },
		{ "\xE2\x9F\x9A", "\xE2\x9F\x9A" },
		{ "\xE2\x00\x9A", "\xE2\x00\x9A" },
		{ "\xE2\x9F\x00", "\xE2\x9F\x00" },
		{ "A\xc2""C", "A\xc2""C" },
		{ "A\xc2""C", u8"A\ufffdC" },
		{ u8"A\ufffdC", "A\xc2""C" },
	};
	bool pass = true;

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(pairs); i++) {
		ttest_ctx_t tc;
		char name[sizeof(__func__) + 32];
		sprintf(name, "%s_%u", __func__, i);

		tc = ttest_start(report, name, NULL, NULL);

		if (cyaml_utf8_casecmp(pairs[i].a, pairs[i].b) != 0) {
			pass &= ttest_fail(&tc, "Failed to match strings: "
					"%s and %s", pairs[i].a, pairs[i].b);
			continue;
		}

		pass &= ttest_pass(&tc);
	}

	return pass;
}

/**
 * Test comparing strings that match.
 *
 * \param[in]  report  The test report context.
 * \return true if test passes, false otherwise.
 */
static bool test_utf8_strcmp_mismatches(
		ttest_report_ctx_t *report)
{
	static const struct string_pairs {
		const char *a;
		const char *b;
	} pairs[] = {
		{ "Invalid", "\xfa" },
		{ "Cat", u8"😸" },
		{ "cat", u8"😸" },
		{ "1 cat", u8"😸" },
		{ "[cat]", u8"😸" },
		{ "Ü cat", u8"😸" },
		{ "Ü cat", u8"😸" },
		{ "\\", "\xC1\x9C" },
	};
	bool pass = true;

	for (unsigned i = 0; i < CYAML_ARRAY_LEN(pairs); i++) {
		ttest_ctx_t tc;
		char name[sizeof(__func__) + 32];
		sprintf(name, "%s_%u", __func__, i);

		tc = ttest_start(report, name, NULL, NULL);

		if (cyaml_utf8_casecmp(pairs[i].a, pairs[i].b) == 0) {
			pass &= ttest_fail(&tc, "Failed to detect mismatch: "
					"%s and %s", pairs[i].a, pairs[i].b);
			continue;
		}

		pass &= ttest_pass(&tc);
	}

	return pass;
}

/**
 * Run the CYAML util unit tests.
 *
 * \param[in]  rc         The ttest report context.
 * \param[in]  log_level  CYAML log level.
 * \param[in]  log_fn     CYAML logging function, or NULL.
 * \return true iff all unit tests pass, otherwise false.
 */
bool utf8_tests(
		ttest_report_ctx_t *rc,
		cyaml_log_t log_level,
		cyaml_log_fn_t log_fn)
{
	bool pass = true;

	UNUSED(log_level);
	UNUSED(log_fn);

	ttest_heading(rc, "UTF-8 tests: Codepoint composition");

	pass &= test_utf8_get_codepoint(rc);

	ttest_heading(rc, "UTF-8 tests: String comparison");

	pass &= test_utf8_strcmp_same(rc);
	pass &= test_utf8_strcmp_matches(rc);
	pass &= test_utf8_strcmp_mismatches(rc);

	return pass;
}
