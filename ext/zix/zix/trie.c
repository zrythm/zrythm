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

// #define ZIX_TRIE_LINEAR_SEARCH 1

#include "zix/common.h"
#include "zix/trie.h"
#include "zix/trie_util.h"

//typedef uint_fast8_t n_edges_t;
typedef uintptr_t n_edges_t;

typedef struct {
	const uint8_t* str;           /* String stored at this node */
	n_edges_t      num_children;  /* Number of outgoing edges */

	/** Array of first child characters, followed by parallel array of pointers
	    to ZixTrieNode pointers. */
	uint8_t children[];
} ZixTrieNode;

struct _ZixTrie {
	ZixTrieNode* root;  /* Root of the tree */
};

/** Round `size` up to the next multiple of the word size. */
ZIX_PRIVATE uint32_t
zix_trie_pad(uint32_t size)
{
	return (size + sizeof(void*) - 1) & (~(sizeof(void*) - 1));
}

ZIX_PRIVATE ZixTrieNode**
zix_trie_get_children(ZixTrieNode* node)
{
	return (ZixTrieNode**)(node->children +
	                       zix_trie_pad(node->num_children));
}

ZIX_PRIVATE ZixTrieNode**
zix_trie_get_child_ptr(ZixTrieNode* node, n_edges_t index)
{
	return zix_trie_get_children(node) + index;
}

ZIX_PRIVATE void
zix_trie_print_rec(ZixTrieNode* node, FILE* fd)
{
	if (node->str) {
		fprintf(fd, "\t\"%p\" [label=\"%s\"] ;\n", (void*)node, node->str);
	} else {
		fprintf(fd, "\t\"%p\" [label=\"\"] ;\n", (void*)node);
	}

	for (n_edges_t i = 0; i < node->num_children; ++i) {
		ZixTrieNode* const child = *zix_trie_get_child_ptr(node, i);
		zix_trie_print_rec(child, fd);
		fprintf(fd, "\t\"%p\" -> \"%p\" [ label = \"%c\" ];\n",
		        (void*)node, (void*)child, node->children[i]);
	}
}

ZIX_API
void
zix_trie_print_dot(const ZixTrie* t, FILE* fd)
{
	fprintf(fd, "digraph Trie { \n");
	zix_trie_print_rec(t->root, fd);
	fprintf(fd, "}\n");
}

ZIX_PRIVATE size_t
zix_trie_node_size(n_edges_t num_children)
{
	return (sizeof(ZixTrieNode) +
	        zix_trie_pad(num_children) +
	        (num_children * sizeof(ZixTrieNode*)));
}

ZIX_PRIVATE ZixTrieNode*
realloc_node(ZixTrieNode* n, int num_children)
{
	return (ZixTrieNode*)realloc(n, zix_trie_node_size(num_children));
}

ZIX_API ZixTrie*
zix_trie_new(void)
{
	ZixTrie* t = (ZixTrie*)calloc(1, sizeof(ZixTrie));

	t->root = calloc(1, sizeof(ZixTrieNode));

	return t;
}

ZIX_PRIVATE void
zix_trie_free_rec(ZixTrieNode* n)
{
	if (n) {
		for (n_edges_t i = 0; i < n->num_children; ++i) {
			zix_trie_free_rec(*zix_trie_get_child_ptr(n, i));
		}
		free(n);
	}
}

ZIX_API void
zix_trie_free(ZixTrie* t)
{
	zix_trie_free_rec(t->root);
	free(t);
}

ZIX_PRIVATE inline bool
trie_is_leaf(const ZixTrieNode* n)
{
	return n->num_children == 0;
}

ZIX_PRIVATE bool
trie_find_edge(const ZixTrieNode* n, const uint8_t c, n_edges_t* const index)
{
	*index = zix_trie_find_key(n->children, n->num_children, c);
	return *index < n->num_children && n->children[*index] == c;
}

#ifndef NDEBUG
ZIX_PRIVATE bool
zix_trie_node_check(ZixTrieNode* const n)
{
	for (n_edges_t i = 0; i < n->num_children; ++i) {
		assert(n->children[i] != '\0');
		assert(*zix_trie_get_child_ptr(n, i) != NULL);
	}
	return true;
}
#endif

ZIX_PRIVATE size_t
zix_trie_add_child(ZixTrieNode** n_ptr,
                   ZixTrieNode*  child,
                   const uint8_t first)
{
	ZixTrieNode* n = *n_ptr;

	n = realloc_node(n, n->num_children + 1);
#ifdef ZIX_TRIE_LINEAR_SEARCH
	const n_edges_t l = n->num_children;
	memmove(n->children + zix_trie_pad(n->num_children + 1),
	        n->children + zix_trie_pad(n->num_children),
	        n->num_children * sizeof(ZixTrieNode*));
	++n->num_children;
	n->children[n->num_children - 1]                = first;
	*zix_trie_get_child_ptr(n, n->num_children - 1) = child;
#else
	const n_edges_t l = zix_trie_find_key(n->children, n->num_children, first);
	ZixTrieNode*    m = malloc(zix_trie_node_size(n->num_children + 1));
	m->str          = n->str;
	m->num_children = n->num_children + 1;

	memcpy(m->children, n->children, l);
	m->children[l] = first;
	memcpy(m->children + l + 1, n->children + l, n->num_children - l);

	memcpy(zix_trie_get_child_ptr(m, 0),
	       zix_trie_get_child_ptr(n, 0),
	       l * sizeof(ZixTrieNode*));
	*zix_trie_get_child_ptr(m, l) = child;
	memcpy(zix_trie_get_child_ptr(m, l + 1),
	       zix_trie_get_child_ptr(n, l),
	       (n->num_children - l) * sizeof(ZixTrieNode*));
	free(n);
	n = m;
#endif

	*n_ptr = n;

	return l;

	assert(zix_trie_node_check(n));
	//assert(zix_trie_node_check(child));
}

ZIX_PRIVATE ZixTrieNode**
trie_insert_tail(ZixTrieNode**  n_ptr,
                 const uint8_t* str,
                 const uint8_t* first,
                 const size_t   len)
{
	assert(first[0]);
	ZixTrieNode* c = NULL;
	while (first[0]) {
		assert(zix_trie_node_check(*n_ptr));
		
		c               = realloc_node(NULL, 1);
		c->str          = NULL;
		c->num_children = 0;

		const size_t l = zix_trie_add_child(n_ptr, c, first[0]);
		n_ptr = zix_trie_get_child_ptr(*n_ptr, l);
		assert(*n_ptr == c);

		++first;
	}
	c->num_children = 0;
	c->str = str;
	assert(zix_trie_node_check(c));
	assert(*n_ptr == c);
	return n_ptr;
}

ZIX_PRIVATE ZixStatus
trie_insert_internal(ZixTrieNode**  n_ptr,
                     const uint8_t* str,
                     const uint8_t* first,
                     const size_t   len)
{
	assert(zix_trie_node_check(*n_ptr));
	ZixTrieNode* n = *n_ptr;
	n_edges_t    child_i;
	while (trie_find_edge(n, first[0], &child_i)) {
		assert(child_i <= n->num_children);
		ZixTrieNode** const child_ptr = zix_trie_get_child_ptr(n, child_i);
		ZixTrieNode*  const child     = *child_ptr;
		assert(child_ptr);

		assert(zix_trie_node_check(child));

		if (!first[0]) {
			/* Reached end of search string */
			if (child->str) {
				/* Found exact string in trie. */
				return ZIX_STATUS_EXISTS;
			} else {
				/* Set node value to inserted string */
				child->str = str;
				return ZIX_STATUS_SUCCESS;
			}
		} else {
			/* Didn't reach end of search string */
			if (trie_is_leaf(n)) {
				/* Hit leaf early, insert underneath */
				ZixTrieNode** c = trie_insert_tail(n_ptr, str, first, len);
				assert(zix_trie_node_check(*n_ptr));
				assert(zix_trie_node_check(*c));
				return ZIX_STATUS_SUCCESS;
			} else {
				/* Match at this node, continue search downwards (or match) */
				n_ptr = child_ptr;
				assert(*n_ptr);
				n = *n_ptr;
				assert(n == child);
				++first;
			}
		}
	}

	if (!first[0]) {
		if ((*n_ptr)->str) {
			return ZIX_STATUS_EXISTS;
		} else {
			(*n_ptr)->str = str;
			return ZIX_STATUS_SUCCESS;
		}
	}

	/* Hit leaf early, insert underneath */
	assert(zix_trie_node_check(*n_ptr));
	ZixTrieNode** c = trie_insert_tail(n_ptr, str, first, len);
	assert(zix_trie_node_check(*n_ptr));
	assert(zix_trie_node_check(*c));
	return ZIX_STATUS_SUCCESS;
}

ZIX_API ZixStatus
zix_trie_insert(ZixTrie* t, const char* str, size_t len)
{
	assert(strlen(str) == len);

	const uint8_t* ustr = (const uint8_t*)str;
	return trie_insert_internal(&t->root, ustr, ustr, len);
}

ZIX_API ZixStatus
zix_trie_find(const ZixTrie* t, const char* const str, const char** match)
{
	ZixTrieNode* n = t->root;
	const char*  p = str;
	n_edges_t    child_i;

	while (trie_find_edge(n, p[0], &child_i)) {
		assert(child_i <= n->num_children);
		ZixTrieNode* const child = *zix_trie_get_child_ptr(n, child_i);

		++p;

		if (!*p) {
			/* Reached end of search string */
			if (child->str) {
				/* Found exact string in trie. */
				*match = (const char*)child->str;
				return ZIX_STATUS_SUCCESS;
			} else {
				/* Prefix match, but exact string not present. */
				return ZIX_STATUS_NOT_FOUND;
			}
		} else {
			/* Didn't reach end of search string */
			if (trie_is_leaf(n)) {
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
