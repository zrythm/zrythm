/*
  Copyright 2011-2016 David Robillard <http://drobilla.net>

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/ampatree.h"
#include "zix/bitset.h"
#include "zix/common.h"
#include "zix/trie_util.h"

#define ZIX_AMPATREE_BITS  256
#define ZIX_AMPATREE_ELEMS (ZIX_BITSET_ELEMS(ZIX_AMPATREE_BITS))

typedef size_t n_edges_t;

typedef struct ZixAMPatreeNode {
	const uint8_t* label_first;  /* Start of incoming label */
	const uint8_t* label_last;   /* End of incoming label */
	const uint8_t* str;          /* String stored at this node */
	n_edges_t      n_children;   /* Number of outgoing edges */

	ZixBitsetTally tally[ZIX_AMPATREE_ELEMS];
	ZixBitset      bits[ZIX_AMPATREE_ELEMS];

	struct ZixAMPatreeNode* children[];
} ZixAMPatreeNode;

struct _ZixAMPatree {
	ZixAMPatreeNode* root;  /* Root of the tree */
};

ZIX_PRIVATE ZixAMPatreeNode**
zix_ampatree_get_child_ptr(ZixAMPatreeNode* node, n_edges_t index)
{
	return node->children + index;
}

ZIX_PRIVATE void
zix_ampatree_print_rec(ZixAMPatreeNode* node, FILE* fd)
{
	if (node->label_first) {
		size_t edge_label_len = node->label_last - node->label_first + 1;
		char*  edge_label     = (char*)calloc(1, edge_label_len + 1);
		strncpy(edge_label, node->label_first, edge_label_len);
		fprintf(fd, "\t\"%p\" [label=<"
		        "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">"
		        "<TR><TD>%s</TD></TR>", (void*)node, edge_label);
		free(edge_label);
		if (node->str) {
			fprintf(fd, "<TR><TD>\"%s\"</TD></TR>", node->str);
		}
		fprintf(fd, "</TABLE>>,shape=\"plaintext\"] ;\n");
	} else {
		fprintf(fd, "\t\"%p\" [label=\"\"] ;\n", (void*)node);
	}

	for (n_edges_t i = 0; i < node->n_children; ++i) {
		ZixAMPatreeNode* const child = *zix_ampatree_get_child_ptr(node, i);
		zix_ampatree_print_rec(child, fd);
		fprintf(fd, "\t\"%p\" -> \"%p\" ;\n", (void*)node, (void*)child);
	}
}

ZIX_API
void
zix_ampatree_print_dot(const ZixAMPatree* t, FILE* fd)
{
	fprintf(fd, "digraph Patree { \n");
	zix_ampatree_print_rec(t->root, fd);
	fprintf(fd, "}\n");
}

ZIX_PRIVATE size_t
zix_ampatree_node_size(n_edges_t n_children)
{
	return sizeof(ZixAMPatreeNode) + (n_children * sizeof(ZixAMPatreeNode*));
}

ZIX_PRIVATE ZixAMPatreeNode*
realloc_node(ZixAMPatreeNode* n, int n_children)
{
	return (ZixAMPatreeNode*)realloc(n, zix_ampatree_node_size(n_children));
}

ZIX_API ZixAMPatree*
zix_ampatree_new(void)
{
	ZixAMPatree* t = (ZixAMPatree*)calloc(1, sizeof(ZixAMPatree));

	t->root = calloc(1, sizeof(ZixAMPatreeNode));

	return t;
}

ZIX_PRIVATE void
zix_ampatree_free_rec(ZixAMPatreeNode* n)
{
	if (n) {
		for (n_edges_t i = 0; i < n->n_children; ++i) {
			zix_ampatree_free_rec(*zix_ampatree_get_child_ptr(n, i));
		}
		free(n);
	}
}

ZIX_API void
zix_ampatree_free(ZixAMPatree* t)
{
	zix_ampatree_free_rec(t->root);
	free(t);
}

ZIX_PRIVATE inline bool
patree_is_leaf(const ZixAMPatreeNode* n)
{
	return n->n_children == 0;
}

ZIX_PRIVATE size_t
ampatree_find_edge(const ZixAMPatreeNode* n, const uint8_t c)
{
	return zix_bitset_count_up_to_if(n->bits, n->tally, c);
}

#ifndef NDEBUG
ZIX_PRIVATE bool
zix_ampatree_node_check(ZixAMPatreeNode* const n)
{
	for (n_edges_t i = 0; i < n->n_children; ++i) {
		assert(*zix_ampatree_get_child_ptr(n, i) != NULL);
	}
	return true;
}
#endif

ZIX_PRIVATE void
patree_add_edge(ZixAMPatreeNode** n_ptr,
                const uint8_t*    str,
                const uint8_t*    first,
                const uint8_t*    last)
{
	ZixAMPatreeNode* n = *n_ptr;

	assert(last >= first);

	ZixAMPatreeNode* const child = realloc_node(NULL, 0);
	zix_bitset_clear(child->bits, child->tally, ZIX_AMPATREE_BITS);
	child->label_first  = first;
	child->label_last   = last;
	child->str          = str;
	child->n_children = 0;

 	n = realloc_node(n, n->n_children + 1);
	ZixAMPatreeNode* m = malloc(zix_ampatree_node_size(n->n_children + 1));
	m->label_first  = n->label_first;
	m->label_last   = n->label_last;
	m->str          = n->str;
	m->n_children = n->n_children + 1;

	memcpy(m->bits, n->bits, ZIX_AMPATREE_ELEMS * sizeof(ZixBitset));
	memcpy(m->tally, n->tally, ZIX_AMPATREE_ELEMS * sizeof(ZixBitsetTally));
	zix_bitset_set(m->bits, m->tally, first[0]);

	const n_edges_t l = ampatree_find_edge(m, first[0]);
	memcpy(zix_ampatree_get_child_ptr(m, 0),
	       zix_ampatree_get_child_ptr(n, 0),
	       l * sizeof(ZixAMPatreeNode*));
	*zix_ampatree_get_child_ptr(m, l) = child;
	memcpy(zix_ampatree_get_child_ptr(m, l + 1),
	       zix_ampatree_get_child_ptr(n, l),
	       (n->n_children - l) * sizeof(ZixAMPatreeNode*));
	free(n);
	n = m;

	*n_ptr = n;

	assert(zix_ampatree_node_check(n));
	assert(zix_ampatree_node_check(child));
}

/* Split an outgoing edge (to a child) at the given index */
ZIX_PRIVATE void
patree_split_edge(ZixAMPatreeNode** child_ptr,
                  size_t            index)
{
	ZixAMPatreeNode* child = *child_ptr;

	const size_t grandchild_size = zix_ampatree_node_size(child->n_children);

	ZixAMPatreeNode* const grandchild = realloc_node(NULL, child->n_children);
	memcpy(grandchild, child, grandchild_size);
	grandchild->label_first = child->label_first + index;

	child                                 = realloc_node(child, 1);
	child->n_children                     = 1;
	child->label_last                     = child->label_first + (index - 1);
	child->str                            = NULL;
	*zix_ampatree_get_child_ptr(child, 0) = grandchild;
	zix_bitset_clear(child->bits, child->tally, ZIX_AMPATREE_BITS);
	zix_bitset_set(child->bits, child->tally, grandchild->label_first[0]);

	*child_ptr = child;

	assert(zix_ampatree_node_check(child));
	assert(zix_ampatree_node_check(grandchild));
}

ZIX_PRIVATE ZixStatus
patree_insert_internal(ZixAMPatreeNode** n_ptr,
                       const uint8_t*    str,
                       const uint8_t*    first,
                       const size_t      len)
{
	ZixAMPatreeNode* n       = *n_ptr;
	const n_edges_t  child_i = ampatree_find_edge(n, *first);
	if (child_i != (n_edges_t)-1) {
		ZixAMPatreeNode**    child_ptr   = zix_ampatree_get_child_ptr(n, child_i);
		ZixAMPatreeNode*     child       = *child_ptr;
		const uint8_t* const label_first = child->label_first;
		const uint8_t* const label_last  = child->label_last;
		const size_t         label_len   = label_last - label_first + 1;
		const size_t         split_i     = zix_trie_change_index(first, label_first, label_len);

		if (first[split_i]) {
			if (split_i == label_len) {
				return patree_insert_internal(child_ptr, str, first + label_len, len);
			} else {
				patree_split_edge(child_ptr, split_i);
				return patree_insert_internal(child_ptr, str, first + split_i, len);
			}
		} else if (label_len != split_i) {
			patree_split_edge(child_ptr, split_i);
			if (first[split_i]) {
				patree_add_edge(child_ptr, str, first + split_i, str + len - 1);
			} else {
				assert(!(*child_ptr)->str);
				(*child_ptr)->str = str;
			}
			return ZIX_STATUS_SUCCESS;
		} else if (label_first[split_i] && !child->str) {
			child->str = str;
		} else {
			return ZIX_STATUS_EXISTS;
		}
	} else {
		patree_add_edge(n_ptr, str, first, str + len - 1);
	}

	return ZIX_STATUS_SUCCESS;
}

ZIX_API ZixStatus
zix_ampatree_insert(ZixAMPatree* t, const char* str, size_t len)
{
	assert(strlen(str) == len);

	const uint8_t* ustr = (const uint8_t*)str;
	return patree_insert_internal(&t->root, ustr, ustr, len);
}

ZIX_API ZixStatus
zix_ampatree_find(const ZixAMPatree* t, const char* const str, const char** match)
{
	ZixAMPatreeNode* n = t->root;
	const uint8_t*   p = str;
	n_edges_t        child_i;
	while ((child_i = ampatree_find_edge(n, p[0])) != (n_edges_t)-1) {
		assert(child_i <= n->n_children);
		ZixAMPatreeNode* const child = *zix_ampatree_get_child_ptr(n, child_i);

		/* Prefix compare search string and label */
		const uint8_t* const l            = child->label_first;
		const size_t         len          = child->label_last - l + 1;
		const int            change_index = zix_trie_change_index(p, l, len);

		p += change_index;

		if (!*p) {
			/* Reached end of search string */
			if (l + change_index == child->label_last + 1) {
				/* Reached end of label string as well, match */
				*match = child->str;
				return *match ? ZIX_STATUS_SUCCESS : ZIX_STATUS_NOT_FOUND;
			} else {
				/* Label string is longer, no match (prefix match) */
				return ZIX_STATUS_NOT_FOUND;
			}
		} else {
			/* Didn't reach end of search string */
			if (patree_is_leaf(n)) {
				/* Hit leaf early, no match */
				return ZIX_STATUS_NOT_FOUND;
			} else {
				/* Match at this node, continue search downwards (or match) */
				n = child;
			}
		}
	}

	return ZIX_STATUS_NOT_FOUND;
}
