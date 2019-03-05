/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML functions for handling utf8 text.
 */

#ifndef CYAML_UTF8_H
#define CYAML_UTF8_H

/**
 * Get a codepoint from the input string.
 *
 * Caller must provide the expected length given the first input byte.
 *
 * If a multi-byte character contains an invalid continuation byte, the
 * character length will be updated on exit to the number of bytes consumed,
 * and the replacement character, U+FFFD will be returned.
 *
 * \param[in]      s    String to read first codepoint from.
 * \param[in,out]  len  Expected length of first character, updated on exit.
 * \return The codepoint or `0xfffd` if character is invalid.
 */
unsigned cyaml_utf8_get_codepoint(
		const uint8_t *s,
		unsigned *len);

/**
 * Case insensitive comparason.
 *
 * \note This has some limitations and only performs case insensitive
 *       comparason over some sectons of unicode.
 *
 * \param[in]  str1  First string to be compared.
 * \param[in]  str2  Second string to be compared.
 * \return 0 if and only if strings are equal.
 */
int cyaml_utf8_casecmp(
		const void * const str1,
		const void * const str2);

#endif
