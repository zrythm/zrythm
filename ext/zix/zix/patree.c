/*
  Copyright 2011-2014 David Robillard <http://drobilla.net>

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

// #define ZIX_TRIE_LINEAR_SEARCH 1

#include "zix/common.h"
#include "zix/patree.h"
#include "zix/trie_util.h"

typedef size_t n_edges_t;

typedef struct {
	const uint8_t* label_first;  /* Start of incoming label */
	const uint8_t* label_last;   /* End of incoming label */
	const uint8_t* str;          /* String stored at this node */
	n_edges_t      n_children;   /* Number of outgoing edges */

	/** Array of first child characters, followed by parallel array of pointers
	    to ZixPatreeNode pointers. */
	uint8_t children[];
} ZixPatreeNode;

struct _ZixPatree {
	ZixPatreeNode* root;  /* Root of the tree */
};

/** Round `size` up to the next multiple of the word size. */
ZIX_PRIVATE uint32_t
zix_patree_pad(uint32_t size)
{
	return (size + sizeof(void*) - 1) & (~(sizeof(void*) - 1));
}

ZIX_PRIVATE ZixPatreeNode**
zix_patree_get_children(ZixPatreeNode* node)
{
	return (ZixPatreeNode**)(node->children +
	                         zix_patree_pad(node->n_children));
}

ZIX_PRIVATE ZixPatreeNode**
zix_patree_get_child_ptr(ZixPatreeNode* node, n_edges_t index)
{
	return zix_patree_get_children(node) + index;
}

ZIX_PRIVATE void
zix_patree_print_rec(ZixPatreeNode* node, FILE* fd)
{
	if (node->label_first) {
		size_t edge_label_len = node->label_last - node->label_first + 1;
		char*  edge_label     = (char*)calloc(1, edge_label_len + 1);
		strncpy(edge_label, (const char*)node->label_first, edge_label_len);
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
		ZixPatreeNode* const child = *zix_patree_get_child_ptr(node, i);
		zix_patree_print_rec(child, fd);
		fprintf(fd, "\t\"%p\" -> \"%p\" ;\n", (void*)node, (void*)child);
	}
}

ZIX_API
void
zix_patree_print_dot(const ZixPatree* t, FILE* fd)
{
	fprintf(fd, "digraph Patree { \n");
	zix_patree_print_rec(t->root, fd);
	fprintf(fd, "}\n");
}

ZIX_PRIVATE size_t
zix_patree_node_size(n_edges_t n_children)
{
	return (sizeof(ZixPatreeNode) +
	        zix_patree_pad(n_children) +
	        (n_children * sizeof(ZixPatreeNode*)));
}

ZIX_PRIVATE ZixPatreeNode*
realloc_node(ZixPatreeNode* n, int n_children)
{
	return (ZixPatreeNode*)realloc(n, zix_patree_node_size(n_children));
}

ZIX_API ZixPatree*
zix_patree_new(void)
{
	ZixPatree* t = (ZixPatree*)calloc(1, sizeof(ZixPatree));

	t->root = calloc(1, sizeof(ZixPatreeNode));

	return t;
}

ZIX_PRIVATE void
zix_patree_free_rec(ZixPatreeNode* n)
{
	if (n) {
		for (n_edges_t i = 0; i < n->n_children; ++i) {
			zix_patree_free_rec(*zix_patree_get_child_ptr(n, i));
		}
		free(n);
	}
}

ZIX_API void
zix_patree_free(ZixPatree* t)
{
	zix_patree_free_rec(t->root);
	free(t);
}

ZIX_PRIVATE inline bool
patree_is_leaf(const ZixPatreeNode* n)
{
	return n->n_children == 0;
}

ZIX_PRIVATE bool
patree_find_edge(const ZixPatreeNode* n, const uint8_t c, n_edges_t* const index)
{
	*index = zix_trie_find_key(n->children, n->n_children, c);
	return *index < n->n_children && n->children[*index] == c;
}

#ifndef NDEBUG
ZIX_PRIVATE bool
zix_patree_node_check(ZixPatreeNode* const n)
{
	for (n_edges_t i = 0; i < n->n_children; ++i) {
		assert(n->children[i] != '\0');
		assert(*zix_patree_get_child_ptr(n, i) != NULL);
	}
	return true;
}
#endif

ZIX_PRIVATE void
patree_add_edge(ZixPatreeNode** n_ptr,
                const uint8_t*  str,
                const uint8_t*  first,
                const uint8_t*  last)
{
	ZixPatreeNode* n = *n_ptr;

	assert(last >= first);

	ZixPatreeNode* const child = realloc_node(NULL, 0);
	child->label_first  = first;
	child->label_last   = last;
	child->str          = str;
	child->n_children = 0;

	n = realloc_node(n, n->n_children + 1);
#ifdef ZIX_TRIE_LINEAR_SEARCH
	memmove(n->children + zix_patree_pad(n->n_children + 1),
	        n->children + zix_patree_pad(n->n_children),
	        n->n_children * sizeof(ZixPatreeNode*));
	++n->n_children;
	n->children[n->n_children - 1]                  = first[0];
	*zix_patree_get_child_ptr(n, n->n_children - 1) = child;
#else
	const n_edges_t l = zix_trie_find_key(n->children, n->n_children, first[0]);
	ZixPatreeNode*  m = malloc(zix_patree_node_size(n->n_children + 1));
	m->label_first  = n->label_first;
	m->label_last   = n->label_last;
	m->str          = n->str;
	m->n_children = n->n_children + 1;

	memcpy(m->children, n->children, l);
	m->children[l] = first[0];
	memcpy(m->children + l + 1, n->children + l, n->n_children - l);

	memcpy(zix_patree_get_child_ptr(m, 0),
	       zix_patree_get_child_ptr(n, 0),
	       l * sizeof(ZixPatreeNode*));
	*zix_patree_get_child_ptr(m, l) = child;
	memcpy(zix_patree_get_child_ptr(m, l + 1),
	       zix_patree_get_child_ptr(n, l),
	       (n->n_children - l) * sizeof(ZixPatreeNode*));
	free(n);
	n = m;
#endif

	*n_ptr = n;

	assert(zix_patree_node_check(n));
	assert(zix_patree_node_check(child));
}

/* Split an outgoing edge (to a child) at the given index */
ZIX_PRIVATE void
patree_split_edge(ZixPatreeNode** child_ptr,
                  size_t          index)
{
	ZixPatreeNode* child = *child_ptr;

	const size_t grandchild_size = zix_patree_node_size(child->n_children);

	ZixPatreeNode* const grandchild = realloc_node(NULL, child->n_children);
	memcpy(grandchild, child, grandchild_size);
	grandchild->label_first = child->label_first + index;

	child                               = realloc_node(child, 1);
	child->n_children                 = 1;
	child->children[0]                  = grandchild->label_first[0];
	*zix_patree_get_child_ptr(child, 0) = grandchild;
	child->label_last                   = child->label_first + (index - 1);
	child->str                          = NULL;

	*child_ptr = child;

	assert(zix_patree_node_check(child));
	assert(zix_patree_node_check(grandchild));
}

ZIX_PRIVATE ZixStatus
patree_insert_internal(ZixPatreeNode** n_ptr,
                       const uint8_t*  str,
                       const uint8_t*  first,
                       const size_t    len)
{
	ZixPatreeNode* n = *n_ptr;
	n_edges_t      child_i;
	if (patree_find_edge(n, *first, &child_i)) {
		ZixPatreeNode**      child_ptr   = zix_patree_get_child_ptr(n, child_i);
		ZixPatreeNode*       child       = *child_ptr;
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
zix_patree_insert(ZixPatree* t, const char* str, size_t len)
{
	assert(strlen(str) == len);

	const uint8_t* ustr = (const uint8_t*)str;
	return patree_insert_internal(&t->root, ustr, ustr, len);
}

ZIX_API ZixStatus
zix_patree_find(const ZixPatree* t, const char* const str, const char** match)
{
	ZixPatreeNode* n = t->root;
	n_edges_t      child_i;

	const uint8_t* p = (const uint8_t*)str;

	while (patree_find_edge(n, p[0], &child_i)) {
		assert(child_i <= n->n_children);
		ZixPatreeNode* const child = *zix_patree_get_child_ptr(n, child_i);

		/* Prefix compare search string and label */
		const uint8_t* const l            = child->label_first;
		const size_t         len          = child->label_last - l + 1;
		const size_t         change_index = zix_trie_change_index(p, l, len);

		p += change_index;

		if (!*p) {
			/* Reached end of search string */
			if (l + change_index == child->label_last + 1) {
				/* Reached end of label string as well, match */
				*match = (const char*)child->str;
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
