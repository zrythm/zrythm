/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

/**
 * \file
 * \brief CYAML functions for handling utf8 text.
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "utf8.h"

/**
 * Get expected byte-length of UTF8 character.
 *
 * Finds the number of bytes expected for the UTF8 sequence starting with
 * the given byte.
 *
 * \param[in]  b  First byte of UTF8 sequence.
 * \return the byte width of the character or 0 if invalid.
 */
static inline unsigned cyaml_utf8_char_len(uint8_t b)
{
	if (!(b & 0x80)) {
		return 1;
	} else switch (b >> 3) {
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
			return 2;
		case 0x1c:
		case 0x1d:
			return 3;
		case 0x1e:
			return 4;
	}
	return 0; /* Invalid */
}

/* Exported function, documented in utf8.h. */
unsigned cyaml_utf8_get_codepoint(
		const uint8_t *s,
		unsigned *len)
{
	unsigned c = 0;
	bool sf = false;

	if (*len == 1) {
		return s[0];
	} else if ((*len > 1) && (*len <= 4)) {
		/* Compose first byte into codepoint. */
		c |= (s[0] & ((1 << (7 - *len)) - 1)) << ((*len - 1) * 6);

		/* Handle continuation bytes. */
		for (unsigned i = 1; i < *len; i++) {
			/* Check continuation byte begins with 0b10xxxxxx. */
			if ((s[i] & 0xc0) != 0x80) {
				/* Need to shorten length so we don't consume
				 * this byte with this invalid character. */
				*len -= i;
				goto invalid;
			}

			/* Compose continuation byte into codepoint. */
			c |= (0x3f & s[i]) << ((*len - i - 1) * 6);
		}
	}

	/* Non-shortest forms are forbidden.
	 *
	 * The following table shows the bits available for each
	 * byte sequence length, as well as the bit-delta to the
	 * shorter sequence.
	 *
	 * | Bytes | Bits | Bit delta | Data                    |
	 * | ----- | ---- | --------- | ----------------------- |
	 * | 1     |  7   | N/A       | 0xxxxxxx                |
	 * | 2     |  11  | 4         | 110xxxxx + 10xxxxxx x 1 |
	 * | 3     |  16  | 5         | 1110xxxx + 10xxxxxx x 2 |
	 * | 4     |  21  | 5         | 11110xxx + 10xxxxxx x 3 |
	 *
	 * So here we check that the top "bit-delta" bits are not all
	 * clear for the byte length,
	 */
	switch (*len) {
	case 2: sf = (c & (((1 << 4) - 1) << (11 - 4))) != 0; break;
	case 3: sf = (c & (((1 << 5) - 1) << (16 - 5))) != 0; break;
	case 4: sf = (c & (((1 << 5) - 1) << (21 - 5))) != 0; break;
	default: goto invalid;
	}

	if (!sf) {
		/* Codepoint representation was not shortest-form. */
		goto invalid;
	}

	return c;

invalid:
	return 0xfffd; /* REPLACEMENT CHARACTER */
}

/**
 * Convert a Unicode codepoint to lower case.
 *
 * \note This only handles some of the Unicode blocks.
 *       (Currently the Latin ones.)
 *
 * \param[in]  c  Codepoint to convert to lower-case, if applicable.
 * \return the lower-cased codepoint.
 */
static unsigned cyaml_utf8_to_lower(unsigned c)
{
	if (((c >= 0x0041) && (c <= 0x005a)) /* Basic Latin */        ||
	    ((c >= 0x00c0) && (c <= 0x00d6)) /* Latin-1 Supplement */ ||
	    ((c >= 0x00d8) && (c <= 0x00de)) /* Latin-1 Supplement */ ) {
		return c + 32;

	} else if (((c >= 0x0100) && (c <= 0x012f)) /* Latin Extended-A */ ||
	           ((c >= 0x0132) && (c <= 0x0137)) /* Latin Extended-A */ ||
	           ((c >= 0x014a) && (c <= 0x0177)) /* Latin Extended-A */ ||
	           ((c >= 0x0182) && (c <= 0x0185)) /* Latin Extended-B */ ||
	           ((c >= 0x01a0) && (c <= 0x01a5)) /* Latin Extended-B */ ||
	           ((c >= 0x01de) && (c <= 0x01ef)) /* Latin Extended-B */ ||
	           ((c >= 0x01f8) && (c <= 0x021f)) /* Latin Extended-B */ ||
	           ((c >= 0x0222) && (c <= 0x0233)) /* Latin Extended-B */ ||
	           ((c >= 0x0246) && (c <= 0x024f)) /* Latin Extended-B */ ) {
		return c & ~0x1;

	} else if (((c >= 0x0139) && (c <= 0x0148) /* Latin Extended-A */) ||
	           ((c >= 0x0179) && (c <= 0x017e) /* Latin Extended-A */) ||
	           ((c >= 0x01b3) && (c <= 0x01b6) /* Latin Extended-B */) ||
	           ((c >= 0x01cd) && (c <= 0x01dc) /* Latin Extended-B */)) {
		return (c + 1) & ~0x1;

	} else switch (c) {
		case 0x0178: return 0x00ff; /* Latin Extended-A */
		case 0x0187: return 0x0188; /* Latin Extended-B */
		case 0x018b: return 0x018c; /* Latin Extended-B */
		case 0x018e: return 0x01dd; /* Latin Extended-B */
		case 0x0191: return 0x0192; /* Latin Extended-B */
		case 0x0198: return 0x0199; /* Latin Extended-B */
		case 0x01a7: return 0x01a8; /* Latin Extended-B */
		case 0x01ac: return 0x01ad; /* Latin Extended-B */
		case 0x01af: return 0x01b0; /* Latin Extended-B */
		case 0x01b7: return 0x0292; /* Latin Extended-B */
		case 0x01b8: return 0x01b9; /* Latin Extended-B */
		case 0x01bc: return 0x01bd; /* Latin Extended-B */
		case 0x01c4: return 0x01c6; /* Latin Extended-B */
		case 0x01c5: return 0x01c6; /* Latin Extended-B */
		case 0x01c7: return 0x01c9; /* Latin Extended-B */
		case 0x01c8: return 0x01c9; /* Latin Extended-B */
		case 0x01ca: return 0x01cc; /* Latin Extended-B */
		case 0x01cb: return 0x01cc; /* Latin Extended-B */
		case 0x01f1: return 0x01f3; /* Latin Extended-B */
		case 0x01f2: return 0x01f3; /* Latin Extended-B */
		case 0x01f4: return 0x01f5; /* Latin Extended-B */
		case 0x01f7: return 0x01bf; /* Latin Extended-B */
		case 0x0220: return 0x019e; /* Latin Extended-B */
		case 0x023b: return 0x023c; /* Latin Extended-B */
		case 0x023d: return 0x019a; /* Latin Extended-B */
		case 0x0241: return 0x0242; /* Latin Extended-B */
		case 0x0243: return 0x0180; /* Latin Extended-B */
	}

	return c;
}

/* Exported function, documented in utf8.h. */
int cyaml_utf8_casecmp(
		const void * const str1,
		const void * const str2)
{
	const uint8_t *s1 = str1;
	const uint8_t *s2 = str2;

	while (true) {
		unsigned len1;
		unsigned len2;
		int cmp1;
		int cmp2;

		/* Check for end of strings. */
		if ((*s1 == 0) && (*s2 == 0)) {
			return 0; /* Both strings ended; match. */

		} else if (*s1 == 0) {
			return 1; /* String 1 has ended. */

		} else if (*s2 == 0) {
			return -1;/* String 2 has ended. */
		}

		/* Get byte lengths of these characters. */
		len1 = cyaml_utf8_char_len(*s1);
		len2 = cyaml_utf8_char_len(*s2);

		/* Compare values. */
		if ((len1 == 1) && (len2 == 1)) {
			/* Common case: Both strings have ASCII values. */
			if (*s1 != *s2) {
				/* They're different; need to lower case. */
				cmp1 = ((*s1 >= 'A') && (*s1 <= 'Z')) ?
						(*s1 + 'a' - 'A') : *s1;
				cmp2 = ((*s2 >= 'A') && (*s2 <= 'Z')) ?
						(*s2 + 'a' - 'A') : *s2;
				if (cmp1 != cmp2) {
					return cmp1 - cmp2;
				}
			}
		} else if ((len1 != 0) && (len2 != 0)) {
			/* Neither string has invalid sequence;
			 * convert to UCS4 for comparison. */
			cmp1 = cyaml_utf8_get_codepoint(s1, &len1);
			cmp2 = cyaml_utf8_get_codepoint(s2, &len2);

			if (cmp1 != cmp2) {
				/* They're different; need to lower case. */
				cmp1 = cyaml_utf8_to_lower(cmp1);
				cmp2 = cyaml_utf8_to_lower(cmp2);
				if (cmp1 != cmp2) {
					return cmp1 - cmp2;
				}
			}
		} else {
			if (len1 | len2) {
				/* One of the strings has invalid sequence. */
				return ((int)len1) - ((int)len2);
			} else {
				/* Both strings have an invalid sequence. */
				len1 = len2 = 1;
			}
		}

		/* Advance each string by their current character length. */
		s1 += len1;
		s2 += len2;
	}
}
