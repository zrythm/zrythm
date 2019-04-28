/*
   Copyright 2011 David Robillard <http://drobilla.net>

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/common.h"
#include "zix/strindex.h"

typedef struct _ZixStrindexNode {
	struct _ZixStrindexNode* children;      /* Children of this node */
	size_t                   num_children;  /* Number of outgoing edges */
	char*                    first;         /* Start of this suffix */
	char*                    label_first;   /* Start of incoming label */
	char*                    label_last;    /* End of incoming label */
} ZixStrindexNode;

struct _ZixStrindex {
	char*            s;     /* String contained in this tree */
	ZixStrindexNode* root;  /* Root of the tree */
};

static ZixStatus
strindex_insert(ZixStrindexNode* n,
                char*            suffix_first,
                char*            first,
                char*            last);

ZIX_API
ZixStrindex*
zix_strindex_new(const char* s)
{
	ZixStrindex* t = (ZixStrindex*)malloc(sizeof(ZixStrindex));
	memset(t, '\0', sizeof(struct _ZixStrindex));
	t->s    = strdup(s);
	t->root = (ZixStrindexNode*)malloc(sizeof(ZixStrindexNode));
	memset(t->root, '\0', sizeof(ZixStrindexNode));
	t->root->num_children = 0;
	t->root->first        = t->s;

	const size_t len = strlen(t->s);
	for (size_t i = 0; i < len; ++i) {
		strindex_insert(t->root, t->s + i, t->s + i, t->s + (len - 1));
	}

	return t;
}

static void
zix_strindex_free_rec(ZixStrindexNode* n)
{
	if (n) {
		for (size_t i = 0; i < n->num_children; ++i) {
			zix_strindex_free_rec(&n->children[i]);
		}
		free(n->children);
	}
}

ZIX_API
void
zix_strindex_free(ZixStrindex* t)
{
	zix_strindex_free_rec(t->root);
	free(t->s);
	free(t->root);
	free(t);
}

static inline int
strindex_is_leaf(ZixStrindexNode* n)
{
	return n->num_children == 0;
}

static int
strindex_find_edge(ZixStrindexNode* n, char c, size_t* index)
{
	for (size_t i = 0; i < n->num_children; ++i) {
		if (n->children[i].label_first[0] == c) {
			*index = i;
			return 1;
		}
	}
	return 0;
}

static void
strindex_add_edge(ZixStrindexNode* n,
                  char*            suffix_first,
                  char*            first,
                  char*            last)
{
	assert(last > first);
	n->children = (ZixStrindexNode*)realloc(
		n->children, (n->num_children + 1) * sizeof(ZixStrindexNode));

	memset(&n->children[n->num_children], '\0', sizeof(ZixStrindexNode));

	n->children[n->num_children].first       = suffix_first;
	n->children[n->num_children].label_first = first;
	n->children[n->num_children].label_last  = last;
	++n->num_children;
}

/* Split an outgoing edge (to a child) at the given index */
static void
strindex_split_edge(ZixStrindexNode* child,
                    size_t           index)
{
	ZixStrindexNode* children     = child->children;
	size_t           num_children = child->num_children;

	char* first = child->label_first + index;
	char* last  = child->label_last;
	assert(last > first);
	assert(child->first);

	child->children = (ZixStrindexNode*)malloc(sizeof(ZixStrindexNode));

	child->children[0].children     = children;
	child->children[0].num_children = num_children;
	child->children[0].first        = child->first;
	child->children[0].label_first  = first;
	child->children[0].label_last   = last;

	child->label_last = child->label_first + (index - 1);  // chop end

	child->num_children = 1;
}

static ZixStatus
strindex_insert(ZixStrindexNode* n, char* suffix_first, char* first, char* last)
{
	size_t           child_i;
	ZixStrindexNode* child;
	assert(first <= last);

	if (strindex_find_edge(n, *first, &child_i)) {
		char*        label_first = n->children[child_i].label_first;
		char*        label_last  = n->children[child_i].label_last;
		size_t       split_i     = 0;
		const size_t label_len   = label_last - label_first + 1;
		const size_t s_len       = last - first + 1;
		const size_t max_len     = (s_len < label_len) ? s_len : label_len;

		while (split_i < max_len && first[split_i] == label_first[split_i]) {
			++split_i;
		}
		child = n->children + child_i;

		if (label_len < s_len) {
			if (split_i == label_len) {
				strindex_insert(child, suffix_first, first + label_len, last);
			} else {
				strindex_split_edge(child, split_i);
				strindex_insert(child, suffix_first, first + split_i, last);
			}
		} else if ((label_len != split_i) && (label_len != s_len)) {
			strindex_split_edge(child, split_i);
			if (split_i != s_len) {
				strindex_add_edge(child, suffix_first, first + split_i, last);
			}
		}
	} else {
		strindex_add_edge(n, suffix_first, first, last);
	}

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
ZixStatus
zix_strindex_find(ZixStrindex* t, const char* p, char** match)
{
#ifndef NDEBUG
	const char* orig_p = p;
#endif

	ZixStrindexNode* n = t->root;
	size_t           child_i;

	*match = NULL;

	while (*p != '\0') {
		size_t p_len = strlen(p);
		if (strindex_find_edge(n, p[0], &child_i)) {
			char*  label_first = n->children[child_i].label_first;
			char*  label_last  = n->children[child_i].label_last;
			size_t label_len   = label_last - label_first + 1;
			size_t max_len     = (p_len < label_len) ? p_len : label_len;
			assert(child_i <= n->num_children);
			assert(max_len > 0);

			/* Set match to the start of substring
			   (but may not actually be a complete match yet)
			 */
			if (*match == NULL) {
				assert(p[0] == orig_p[0]);
				assert(orig_p[0] == label_first[0]);
				*match = n->children[child_i].first;
				assert((*match)[0] == orig_p[0]);
			}

			if (strncmp(p, label_first, max_len)) {
				/* no match */
				*match = NULL;
				return ZIX_STATUS_NOT_FOUND;
			}

			if (p_len <= label_len) {
				/* At the last node, match */
				*match = n->children[child_i].first;
				assert(!strncmp(*match, orig_p, strlen(orig_p)));
				return ZIX_STATUS_SUCCESS;
			} else if (strindex_is_leaf(n)) {
				/* Hit leaf early, no match */
				return ZIX_STATUS_NOT_FOUND;
			} else {
				/* Match at this node, continue search downwards (or match) */
				p += label_len;
				n  = &n->children[child_i];
				if (label_len >= p_len) {
					assert(strstr(t->s, orig_p) != NULL);
					assert(strncmp(orig_p, *match, max_len));
					return ZIX_STATUS_SUCCESS;
				}
			}

		} else {
			assert(strstr(t->s, orig_p) == NULL);
			return ZIX_STATUS_NOT_FOUND;
		}
	}
	return ZIX_STATUS_NOT_FOUND;
}
